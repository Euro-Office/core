#!/bin/bash

install_dir=$PWD/katana-parser
patches_dir=$PWD
if [ ! -d $install_dir ]
then
    echo "Fetching Katana parser"
    git clone https://github.com/jasenhuang/katana-parser.git
    cd katana-parser
    git checkout be6df458d4540eee375c513958dcb862a391cdd1
    
    echo "Patching Katana parser"
    git apply $patches_dir/katana.patch

    cd ..
    echo "Katana parser ready!"
else
    echo "Skipping Katana parser (done already)."
fi

install_dir=$PWD/gumbo-parser
patches_dir=$PWD
if [ ! -d $install_dir ]
then
    echo "Fetching gumbo-parser"
    git clone https://github.com/google/gumbo-parser.git
    cd gumbo-parser
    git checkout aa91b27b02c0c80c482e24348a457ed7c3c088e0
    
    echo "Patching gumbo-parser"
    git apply $patches_dir/gumbo.patch

    cd ..
    echo "gumbo-parser ready!"
else
    echo "Skipping gumbo-parser (done already)."
fi
