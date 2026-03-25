#!/bin/bash

work_dir="$1"
install_dir="$2"
icu_major=$3
icu_minor=$4
operation_mode=$5 # fetch-only | full
target_plaform=$6 # linux | windows-crosscompile
mingw_llvm_bin_path=${7:-""}

abort_op()
{
    rm -rf "$work_dir"
    rm -rf "$install_dir"
    echo "ICU aborted: $1" >&2
    exit 1
}

configure_linux() # args: build_dir, install_dir
{
    build_dir="$1"
    linux_install_dir="$2"

    mkdir -p "$build_dir" || abort_op "Failed to create build dir"
    mkdir -p "$linux_install_dir" || abort_op "Failed to create install dir"
    cd "$build_dir"

    $work_dir/icu/source/configure \
    --prefix="$linux_install_dir" \
    --enable-rpath \
    CC=gcc \
    CXX=g++ \
    AR=ar \
    RANLIB=ranlib \
    CXXFLAGS="-static-libstdc++ -static-libgcc" \
    LDFLAGS='-Wl,-rpath,$$ORIGIN' \
    || abort_op "Configure failed"    
}

configure_windows_crosscompile() # args: build_dir, native_build_dir install_dir
{
    build_dir="$1"
    native_build_dir="$2"
    win_install_dir="$3"

    if [ ! -d "$native_build_dir" ]
    then
        abort_op "The provided native build dir doesn't exist: $native_build_dir"
    fi

    if [ ! -e $mingw_llvm_bin_path/x86_64-w64-mingw32-gcc ]
    then
        abort_op "Configuration failed: cross-compiler gcc not exectuable: $mingw_llvm_bin_path/x86_64-w64-mingw32-gcc"
    fi

    mkdir -p "$build_dir" || abort_op "Failed to create build dir"
    mkdir -p "$win_install_dir" || abort_op "Failed to create install dir"
    cd "$build_dir"

    $work_dir/icu/source/configure \
    --host=x86_64-w64-mingw32 \
    --with-cross-build="$native_build_dir" \
    --prefix="$win_install_dir" \
    --enable-shared \
    --disable-static \
    CC=$mingw_llvm_bin_path/x86_64-w64-mingw32-gcc \
    CXX=$mingw_llvm_bin_path/x86_64-w64-mingw32-g++ \
    AR=$mingw_llvm_bin_path/x86_64-w64-mingw32-ar \
    RANLIB=$mingw_llvm_bin_path/x86_64-w64-mingw32-ranlib \
    CXXFLAGS="-static-libstdc++ -static-libgcc" \
    || abort_op "Configure failed"    
}


if [ $# -lt 6 ]
then
    echo "Needs 6 arguments: work_dir_path install_dir_path major_ver minor_ver operation_mode target_platform [mingw_llvm_bin_path]" >&2
    echo "  operation_mode  : fetch_only | full" >&2
    echo "  target_platform : linux | windows-crosscompile" >&2
    echo "  mingw_llvm_bin_path : LLVM MinGW cross-compiler's bin path" >&2
    exit 1
fi

if [ -d $install_dir ]
then
    echo "Skipping ICU (done already)."
    exit 0
else
    mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"
fi

if [ -d "$work_dir" ]
then
    rm -rf $work_dir
fi
mkdir -p "$work_dir" || abort_op "Failed to create work dir: [$work_dir]"

echo "Fetching ICU into: [$work_dir]"
git clone --depth 1 --branch release-$icu_major-$icu_minor https://github.com/unicode-org/icu.git "$work_dir/icu2" \
    || abort_op "Git clone failed!"

cd "$work_dir"
cp -r icu2/icu4c ./icu
cp icu2/LICENSE ./
rm -rf icu2

echo "OPERATION: $operation_mode"
if [ "$operation_mode" == "fetch-only" ] || [ "$operation_mode" == "fetch_only" ]
then
    echo "ICU ready! (fetch only)"
    exit 0
fi

if [ "$target_plaform" == "linux" ]
then
    echo "Configuring icu (linux x86_64)"

    configure_linux "$work_dir/build_linux_x86_64" "$install_dir"
    make -j$(nproc) && make install || abort_op "Build failed"

elif [ "$target_plaform" == "windows-crosscompile" ]
then
    echo "Configuring icu (linux x86_64) for crosscompilation (windows x86_64)"

    # Need to build the linux native version for ICU's build tools to be present
    configure_linux "$work_dir/build_linux_x86_64" "$work_dir/install_linux_x86_64"
    cd "$work_dir/build_linux_x86_64"
    # make -j$(nproc) && make install || abort_op "Host build failed"
    make -j$(nproc) || abort_op "Host build failed"

    configure_windows_crosscompile "$work_dir/build_win_x86_64" "$work_dir/build_linux_x86_64" "$install_dir"
    cd "$work_dir/build_win_x86_64"
    make -j$(nproc) && make install || abort_op "Cross-build failed"

else
    abort_op "ICU failed: no valid platform specified!"

fi

echo "ICU ready! (work dir will be removed)"
rm -rf "$work_dir"

exit 0
