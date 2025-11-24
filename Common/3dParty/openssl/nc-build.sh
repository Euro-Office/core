#!/bin/bash

install_dir=$PWD/openssl_build_linux_64
if [ ! -d $install_dir ]
then
    echo "Fetching openssl"
    git clone --depth=1 --branch OpenSSL_1_1_1f https://github.com/openssl/openssl.git
    
    echo "Building openssl"
    mkdir openssl_build_linux_64

    cd openssl
    ./config enable-md2 no-shared no-asm --prefix=$install_dir --openssldir=$install_dir
    make -j10
    make install

    cd ..
    echo "Openssl ready!"
else
    echo "Skipping OpenSSL build (done already)."
fi
