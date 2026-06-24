#!/bin/bash

install_dir="$1"

# If the install dir comes from wsl, convert that to cygwin path
if [[ $install_dir == /mnt/* ]]; then
    install_dir="/cygdrive/${install_dir#/mnt/}"
fi

export PATH="$PATH:/usr/bin"
export PYTHON=/usr/bin/python3

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