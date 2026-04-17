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

# Build and install
make -j$(nproc) || abort_op "Build failed"
make install || abort_op "Install failed"
