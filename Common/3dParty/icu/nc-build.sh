#!/bin/bash

invoke_dir=$PWD
install_dir=$PWD/install_linux_x64
if [ ! -d $install_dir ]
then
    echo "Fetching icu"
    icu_major="74"
    icu_minor="2"

    git clone --depth 1 --branch release-$icu_major-$icu_minor https://github.com/unicode-org/icu.git icu2
    cp -r icu2/icu4c ./icu
    cp icu2/LICENSE ./
    rm -rf icu2

    echo "Building icu"
    mkdir build_linux_x64
    mkdir install_linux_x64
    cd build_linux_x64
    $invoke_dir/icu/source/configure \
    --prefix=$invoke_dir/install_linux_x64 \
    --enable-rpath \
    CC=gcc \
    CXX=g++ \
    AR=ar \
    RANLIB=ranlib \
    CXXFLAGS="-static-libstdc++ -static-libgcc" \
    LDFLAGS='-Wl,-rpath,$$ORIGIN'

    make -j10 && make install
    cd ..
    
    echo "icu ready!"
else
    echo "Skipping icu (done already)."
fi   
