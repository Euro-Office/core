#!/bin/bash

install_dir=$PWD/hyphen
if [ ! -d $install_dir ]
then
    echo "Fetching hyphen"
    git clone https://github.com/hunspell/hyphen
    echo "hyphen ready!"
else
    echo "Skipping hyphen (done already)."
fi
