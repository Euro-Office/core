#!/bin/bash

install_dir=$PWD/socket.io-client-cpp
patches_dir=$PWD/patches/proper_patches
if [ ! -d $install_dir ]
then
    echo "Fetching socket-io"
    git clone https://github.com/socketio/socket.io-client-cpp.git
    cd socket.io-client-cpp
    git checkout da779141a7379cc30c870d48295033bc16a23c66
    git submodule update --init --recursive
    
    echo "Patching socket-io submodules"

    cd lib/asio
    git checkout 230c0d2ae035c5ce1292233fcab03cea0d341264
    cd ../..


    cd lib/websocketpp
    git checkout 56123c87598f8b1dd471be83ca841ceae07f95ba
    git apply $patches_dir/websocketpp.patch
    cd ../..


    cd src/internal
    git apply $patches_dir/sio_client_impl_fail.patch
    git apply $patches_dir/sio_client_impl_open.patch
    git apply $patches_dir/sio_client_impl_close_timeout.patch
    cd ../..


    cp -r src src_no_tls
    git apply $patches_dir/no_tls.patch
    

    cd ..
    echo "socket-io ready!"
else
    echo "Skipping socket-io (done already)."
fi
