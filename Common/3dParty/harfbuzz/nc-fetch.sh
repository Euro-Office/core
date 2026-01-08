#!/bin/bash

install_dir="$1"

script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
patches_dir="$script_dir/patch"

abort_op()
{
    rm -rf "$install_dir"
    echo "Harfbuzz aborted: $1" >&2
    exit 1
}

if [ $# -lt 1 ]
then
    echo "Needs 1 arguments: install_dir_path" >&2
    exit 1
fi

if [ -d $install_dir ]
then
    echo "Skipping Harfbuzz (done already)."
    exit 0
else
    mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"
fi

echo "Fetching Harfbuzz"
git clone https://github.com/harfbuzz/harfbuzz.git "$install_dir" \
    || abort_op "Failed to clone Harfbuzz repo"
cd "$install_dir"
git checkout 894a1f72ee93a1fd8dc1d9218cb3fd8f048be29a || abort_op "Failed to check out 894a1f72ee93a1fd8dc1d9218cb3fd8f048be29a"
git apply $patches_dir/harfbuzz.patch || abort_op "Failed to apply harfbuzz.patch"

echo "Harfbuzz ready!"
