#!/bin/bash

install_dir="$1"

# If the install dir comes from wsl, convert that to cygwin path
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

abort_op()
{
    echo "ICU-CygWin aborted: $1" >&2
    exit 1
}

if [ ! -f "./runConfigureICU" ]
then
  abort_op "This script has to be run from the icu/source directory (cannot find runConfigureICU)"
fi

if [ ! -d "$install_dir" ]
then
  mkdir -p "$install_dir" || abort_op "Failed to create install dir"
fi

"./runConfigureICU" Cygwin/MSVC \
  --prefix="$install_dir" \
  --enable-shared \
  --disable-static || abort_op "Configuration failed"


# Fix bug with older icu versions
mkdir -p data/out/tmp data/out/build


# Build and install
make -j$(nproc) || make -j1 || abort_op "Build failed"
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