#!/bin/bash

work_dir="$1"
install_dir="$2"
target_platform="$3" # linux | windows-crosscompile
keep_work=${4:-"no"}
mingw_llvm_bin_path=${5:-""}

script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
patches_dir="$script_dir"

abort_op()
{
    rm -rf "$work_dir"
    rm -rf "$install_dir"
    echo "OpenSSL aborted: $1" >&2
    exit 1
}

configure_linux()
{
    echo "Configuring OpenSSL (linux)"
    ./config enable-md2 no-shared no-asm --prefix=$install_dir --openssldir=$install_dir \
    || abort_op "Configuration failed!"
}

configure_windows_crosscompile()
{
    echo "Configuring OpenSSL (windows-crosscompile)"

    export CC=gcc
    export AR=ar
    export RANLIB=ranlib
    export WINDRES=windres  # for resource compilation
    export CROSS_COMPILE="$mingw_llvm_bin_path/x86_64-w64-mingw32-"   # optional, helps OpenSSL detect tools

    if [ ! -e $CROSS_COMPILE$CC ]
    then
        abort_op "Configuration failed: cross-compiler gcc not exectuable: $CROSS_COMPILE$CC"
    fi

    perl Configure mingw64 \
        enable-md2 \
        no-shared \
        no-asm \
        --prefix=$install_dir --openssldir=$install_dir \
    || abort_op "Configuration failed!"

    echo "Patching OpenSSL for llvm-mingw"
    git apply "$patches_dir/openssl_win.patch" || abort_op "Failed to apply openssl_win.patch"
}

if [ $# -lt 3 ]
then
    echo "Needs at least 3 arguments: work_dir_path install_dir_path target_platform [\"keep-workdir\"] [mingw_llvm_bin_path]" >&2
    echo "  target_platform     : linux | windows-crosscompile" >&2
    echo "  mingw_llvm_bin_path : LLVM MinGW cross-compiler's bin path" >&2
    exit 1
fi

if [ -d $install_dir ]
then
    echo "Skipping OpenSSL (done already)."
    exit 0
else
    mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"
fi

if [ -d "$work_dir" ]
then
    rm -rf $work_dir
fi
mkdir -p "$work_dir" || abort_op "Failed to create work dir: [$work_dir]"

echo "Fetching OpenSSL repo into: [$work_dir]"
git clone --depth=1 --branch OpenSSL_1_1_1f https://github.com/openssl/openssl.git "$work_dir" \
    || abort_op "Git clone failed!"

cd "$work_dir"
if [ "$target_platform" == "linux" ]
then
    configure_linux
elif [ "$target_platform" == "windows-crosscompile" ]
then
    configure_windows_crosscompile
else
    abort_op "OpenSSL failed: no valid platform specified!"
fi

echo "Building OpenSSL"
make -j$(nproc) || abort_op "Build failed!"

echo "Installing OpenSSL to: [$install_dir]"
make install || abort_op "Install failed!"

if [ "$keep_work" != "keep-workdir" ] && [ "$keep_work" != "keep_workdir" ]; then
    echo "OpenSSL ready! (work dir will be removed)"
    rm -rf "$work_dir"
else
    echo "OpenSSL ready! (work dir will be kept)"
fi

exit 0
