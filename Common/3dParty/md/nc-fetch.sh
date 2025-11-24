#!/bin/bash

install_dir=$PWD/md4c
if [ ! -d $install_dir ]
then
    echo "Fetching md4c"
    git clone https://github.com/mity/md4c.git
    cd md4c
    git checkout 481fbfbdf72daab2912380d62bb5f2187d438408

    cd ..
    echo "md4c ready!"
else
    echo "Skipping md4c (done already)."
fi
