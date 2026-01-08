#!/bin/bash

install_dir="$1"

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
git clone https://github.com/google/brotli.git "$install_dir" \
    || abort_op "Failed to clone Brotli repo"
cd "$install_dir"
git checkout a47d7475063eb223c87632eed806c0070e70da29 || abort_op "Failed to check out a47d7475063eb223c87632eed806c0070e70da29"

echo "Brotli ready!"
