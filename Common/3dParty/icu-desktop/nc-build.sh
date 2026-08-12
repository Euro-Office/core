#!/bin/bash

work_dir="$1"
install_dir="$2"
icu_major=$3
icu_minor=$4
fetch_only=${5:-0}

abort_op()
{
    rm -rf "$work_dir"
    rm -rf "$install_dir"
    echo "ICU aborted: $1" >&2
    exit 1
}

if [ $# -lt 4 ]
then
    echo "Needs 4 arguments: work_dir_path install_dir_path major_ver minor_ver" >&2
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

echo "Building icu"
mkdir build_linux_x64 || abort_op "Failed to create build dir"
cd build_linux_x64
$work_dir/icu/source/configure \
--prefix="$install_dir" \
--enable-rpath \
CC=gcc \
CXX=g++ \
AR=ar \
RANLIB=ranlib \
CXXFLAGS="-static-libstdc++ -static-libgcc" \
LDFLAGS='-Wl,-rpath,$$ORIGIN' \
|| abort_op "Configure failed"

if [ "$fetch_only" -eq 0 ]; then
    make -j10 && make install || abort_op "Build failed"

    echo "ICU ready! (work dir will be removed)"
    rm -rf "$work_dir"
fi

exit 0
