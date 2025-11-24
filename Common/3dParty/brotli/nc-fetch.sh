#!/bin/bash

install_dir=$PWD/brotli
if [ ! -d $install_dir ]
then
    echo "Fetching Brotli"
    git clone https://github.com/google/brotli.git
    cd brotli
    git checkout a47d7475063eb223c87632eed806c0070e70da29
    
    cd ..
    echo "Brotli ready!"
else
    echo "Skipping Brotli (done already)."
fi
