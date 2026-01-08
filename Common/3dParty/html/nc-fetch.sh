#!/bin/bash

install_dir_katana=$1
install_dir_gumbo=$2

script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
patches_dir="$script_dir"

abort_op()
{
    rm -rf "$install_dir_katana"
    rm -rf "$install_dir_gumbo"
    echo "Katana & Gumbo aborted: $1" >&2
    exit 1
}

if [ $# -lt 2 ]
then
    echo "Needs 2 arguments: katana_install_dir_path gumbo_install_dir_path" >&2
    exit 1
fi

if [ -d $install_dir_katana ]
then
    echo "Skipping Katana (done already)."
    exit 0
else
    mkdir -p "$install_dir_katana" || abort_op "Failed to create install dir: [$install_dir_katana]"
fi

if [ -d $install_dir_gumbo ]
then
    echo "Skipping Gumbo (done already)."
    exit 0
else
    mkdir -p "$install_dir_gumbo" || abort_op "Failed to create install dir: [$install_dir_gumbo]"
fi


echo "Fetching Katana parser"
git clone https://github.com/jasenhuang/katana-parser.git "$install_dir_katana" \
    || abort_op "Failed to clone Katana repo"

cd "$install_dir_katana"
git checkout be6df458d4540eee375c513958dcb862a391cdd1 || abort_op "Failed to check out be6df458d4540eee375c513958dcb862a391cdd1"
git apply $patches_dir/katana.patch || abort_op "Failed to apply katana.patch"
echo "Katana ready!"

echo "Fetching Gumbo parser"
git clone https://github.com/google/gumbo-parser.git "$install_dir_gumbo" \
    || abort_op "Failed to clone Gumbo repo"
cd "$install_dir_gumbo"
git checkout aa91b27b02c0c80c482e24348a457ed7c3c088e0 || abort_op "Failed to check out aa91b27b02c0c80c482e24348a457ed7c3c088e0"
git apply $patches_dir/gumbo.patch || abort_op "Failed to apply gumbo.patch"

echo "Gumbo ready!"
