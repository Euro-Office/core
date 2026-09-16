#!/bin/bash

install_dir="$1"
if [[ $install_dir == /mnt/* ]]; then
    install_dir="/cygdrive/${install_dir#/mnt/}"
fi

export PATH="$PATH:/usr/bin"

# Pick a working Python 3 for ICU's data/rules.mk generation. Prefer Cygwin's
# python3, but fall back to any python3/python on PATH (e.g. the native Windows
# one the build front-loads) when Cygwin's isn't installed. ICU's databuilder
# normalizes path separators to '/', so native Python produces correct makefile
# fragments too.
if [ -x /usr/bin/python3 ]; then
    export PYTHON=/usr/bin/python3
elif command -v python3 >/dev/null 2>&1; then
    export PYTHON="$( command -v python3 )"
elif command -v python >/dev/null 2>&1; then
    export PYTHON="$( command -v python )"
else
    echo "ERROR: no Python 3 found. Install Cygwin's python3, or ensure a native python3 is on PATH." >&2
    exit 1
fi
echo "Using PYTHON=$PYTHON"

# --- Make MSVC's link.exe win over Cygwin's /usr/bin/link (coreutils) --------
# ICU's configure runs `link --version` and aborts with "link.exe is not a
# valid linker" if it reports "GNU coreutils". Cygwin ships such a `link`, and
# depending on the inherited PATH order it can shadow MSVC's linker. cl is
# already on PATH (CC=cl) and MSVC's link.exe lives in the SAME directory, so
# front-load that directory; falls back to VCToolsInstallDir if cl isn't found.
msvc_bin=""
cl_path="$( command -v cl 2>/dev/null || true )"
if [ -n "$cl_path" ]; then
    msvc_bin="$( dirname "$cl_path" )"
elif [ -n "$VCToolsInstallDir" ]; then
    msvc_bin="$( cygpath -u "$VCToolsInstallDir" )/bin/Hostx64/x64"
fi
if [ -n "$msvc_bin" ] && [ -x "$msvc_bin/link.exe" ]; then
    export PATH="$msvc_bin:$PATH"
    echo "Front-loaded MSVC bin so link.exe resolves to the MS linker: $msvc_bin"
else
    echo "WARNING: could not locate MSVC bin dir; Cygwin's link may shadow link.exe" >&2
fi

# --- logging: write a clean UTF-8 log straight from bash ---------------------
# Done before any heavy work so the real output never passes through the
# parent's (mis-decoding) pipe. fd 3 keeps a handle to the original stderr so
# we can still surface a short "see log" line to the console/parent.
mkdir -p "$install_dir" || { echo "Failed to create install dir" >&2; exit 1; }
LOG="${install_dir%/}/icu-build.log"
exec 3>&2
exec >"$LOG" 2>&1
# -----------------------------------------------------------------------------

abort_op()
{
    echo "ICU-CygWin aborted: $1" >&2                 # full context in the log
    echo "ICU-CygWin aborted: $1 (see $LOG)" >&3      # short line to the console
    exit 1
}

if [ ! -f "./runConfigureICU" ]
then
  abort_op "This script has to be run from the icu/source directory (cannot find runConfigureICU)"
fi

"./runConfigureICU" Cygwin/MSVC \
  --prefix="$install_dir" \
  --enable-shared \
  --disable-static \
  --disable-renaming || abort_op "Configuration failed"

# Build and install
make -j$(nproc) || abort_op "Build failed"
make install || abort_op "Install failed"

# ---------------------------------------------------------------------------
# Replace symlinks with real copies of their targets.
#
# Cygwin's `make install` creates symlinks (e.g. lib/icu/Makefile.inc) as
# WSL-style reparse points. Cygwin can read those, but native Windows
# programs cannot even open them (CreateFile fails with "Invalid argument"),
# which breaks anything that walks the install tree afterwards — like the
# CI cache upload (Windows Python tarfile -> OSError errno 22).
# Dereferencing them here makes the install tree fully portable.
# ---------------------------------------------------------------------------
while IFS= read -r -d '' link
do
    target="$(readlink -f "$link")" || abort_op "Cannot resolve symlink: $link"
    [ -e "$target" ] || abort_op "Symlink target missing: $link -> $target"
    rm -f "$link" || abort_op "Cannot remove symlink: $link"
    cp -rL "$target" "$link" || abort_op "Cannot copy symlink target: $target -> $link"
done < <(find "$install_dir" -type l -print0)

remaining=$(find "$install_dir" -type l | wc -l)
[ "$remaining" -eq 0 ] || abort_op "Symlinks still present in install dir after dereferencing"
echo "Install tree contains no symlinks — safe for native Windows consumers."


# ---------------------------------------------------------------------------
# Emit unversioned ICU DLL aliases for Qt 6.10 (loads icuuc.dll by name).
# On Cygwin/MSVC the DLLs land in lib/ next to the import libs, not bin/, so
# locate each versioned DLL wherever it is rather than assuming a directory.
# [0-9] matches only versioned files (never the unversioned target → idempotent);
# -ef skips any same-file copy.
# ---------------------------------------------------------------------------
for base in icuuc icuin icudt; do
    src=$(find "$install_dir/bin" "$install_dir/lib" -maxdepth 1 \
              -name "${base}[0-9]*.dll" 2>/dev/null | sort | tail -n1)
    [ -n "$src" ] || abort_op "No versioned ${base}NN.dll under $install_dir (bin/ or lib/)"
    dst="$(dirname "$src")/${base}.dll"
    [ "$src" -ef "$dst" ] && continue
    cp -f "$src" "$dst" || abort_op "Cannot alias $src -> $dst"
    echo "Aliased $(basename "$src") -> ${base}.dll in $(dirname "$src")"
done
echo "Created unversioned ICU DLL aliases (icuuc.dll, icuin.dll, icudt.dll)."


# Persist renaming-off for downstream consumers: ICU built itself with
# -DU_DISABLE_RENAMING=1, but doesn't bake it into the installed headers, so
# consumers default to renaming-ON and emit _74 symbols. Force it in uconfig.h.
uconfig="$install_dir/include/unicode/uconfig.h"
if ! grep -q "define U_DISABLE_RENAMING 1" "$uconfig"; then
    sed -i 's/#define U_DISABLE_RENAMING 0/#define U_DISABLE_RENAMING 1/' "$uconfig"
fi
grep "U_DISABLE_RENAMING" "$uconfig"   # verify it now reads 1