#!/bin/bash

install_dir="$1"

script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
patches_dir="$script_dir"

abort_op()
{
    rm -rf "$install_dir"
    echo "Apple aborted: $1" >&2
    exit 1
}

if [ $# -lt 1 ]
then
    echo "Needs 1 arguments: install_dir_path" >&2
    exit 1
fi

if [ -d $install_dir ]
then
    echo "Skipping Apple (done already)."
    exit 0
else
    mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"
fi

# glm
git clone https://github.com/g-truc/glm.git "$install_dir/glm" || abort_op "Failed to clone glm repo"
cd "$install_dir/glm"
git checkout 33b4a621a697a305bc3a7610d290677b96beb181 || abort_op "Failed to check out 33b4a621a697a305bc3a7610d290677b96beb181"

# mdds
git clone https://github.com/kohei-us/mdds.git "$install_dir/mdds" || abort_op "Failed to clone mdds repo"
cd "$install_dir/mdds"
git checkout 0783158939c6ce4b0b1b89e345ab983ccb0f0ad0 || abort_op "Failed to checkout 0783158939c6ce4b0b1b89e345ab983ccb0f0ad0"
git apply "$patches_dir/mdds.patch" || abort_op "Failed to apply mdds.patch"

# librevenge
git clone https://github.com/Distrotech/librevenge.git "$install_dir/librevenge" || abort_op "Failed to clone librevenge repo"
cd "$install_dir/librevenge"
git checkout becd044b519ab83893ad6398e3cbb499a7f0aaf4 || abort_op "Failed to check out becd044b519ab83893ad6398e3cbb499a7f0aaf4"
git apply "$patches_dir/librevenge.patch"

# libodfgen
git clone https://github.com/Distrotech/libodfgen.git "$install_dir/libodfgen" || abort_op "Failed to clone libodfgen repo"
cd "$install_dir/libodfgen"
git checkout 8ef8c171ebe3c5daebdce80ee422cf7bb96aa3bc || abort_op "Failed to check out 8ef8c171ebe3c5daebdce80ee422cf7bb96aa3bc"

# libetonyek
git clone https://github.com/LibreOffice/libetonyek.git "$install_dir/libetonyek" || abort_op "Failed to clone libetonyek repo"
cd "$install_dir/libetonyek"
git checkout cb396b4a9453a457469b62a740d8fb933c9442c3 || abort_op "Failed to check out cb396b4a9453a457469b62a740d8fb933c9442c3"
git apply "$patches_dir/libetonyek.patch" || abort_op "Failed to apply libetonyek.patch"

# fetch.py is called with use_gperf = False, so I won't bother with that now.
# see Common/3dParty/apple/fetch.py
