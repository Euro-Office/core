#!/bin/bash

install_dir="$1"
git_tag="v1.2.0"

abort_op()
{
    rm -rf "$install_dir"
    echo "Brotli aborted: $1" >&2
    exit 1
}

if [ $# -lt 1 ]
then
    echo "Needs 1 arguments: install_dir_path" >&2
    exit 1
fi

if [ -d $install_dir ]
then
    echo "Skipping Brotli (done already)."
    exit 0
else
    mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"
fi

echo "Fetching Brotli"

git clone --depth=1 --branch $git_tag https://github.com/google/brotli.git "$install_dir" \
    || abort_op "Failed to clone brotli repo"

echo "Brotli ready!"
