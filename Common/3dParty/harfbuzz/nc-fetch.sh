#!/bin/bash

install_dir=$PWD/harfbuzz
patches_dir=$PWD/patch
if [ ! -d $install_dir ]
then
    echo "Fetching Harfbuzz"
    git clone https://github.com/harfbuzz/harfbuzz.git
    cd harfbuzz
    git checkout 894a1f72ee93a1fd8dc1d9218cb3fd8f048be29a
    
    echo "Patching Harfbuzz"
    git apply $patches_dir/harfbuzz.patch

    cd ..
    echo "Harfbuzz ready!"
else
    echo "Skipping Harfbuzz (done already)."
fi
