#!/bin/bash

work_dir="$1"
install_dir="$2"
keep_work=${3:-""}

package_name="CryptoPP"
repo_url="https://github.com/weidai11/cryptopp"
git_tag="CRYPTOPP_8_9_0"

abort_op()
{
    rm -rf "$work_dir"
    rm -rf "$install_dir"
    echo "$package_name aborted: $1" >&2
    exit 1
}

if [ $# -lt 2 ]
then
    echo "Needs 2 arguments: work_dir_path install_dir_path" >&2
    exit 1
fi

if [ -d $install_dir ]
then
    echo "Skipping $package_name (done already)."
    exit 0
else
    mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"
fi

if [ -d "$work_dir" ]
then
    rm -rf $work_dir
fi
mkdir -p "$work_dir" || abort_op "Failed to create work dir: [$work_dir]"

echo "Fetching $package_name"
git clone --depth=1 --branch $git_tag $repo_url "$work_dir" \
    || abort_op "Failed to clone $package_name repo"

echo "Building $package_name"
cd "$work_dir"
make -j10 || abort_op "Build failed!"

echo "Installing $package_name to [$install_dir]"
make install-lib PREFIX="$install_dir" || abort_op "Install failed!"

echo "$package_name ready!"
if [ "$keep_work" != "keep-work-dir" ]
then
    echo "(work dir will be removed)"
    rm -rf "$work_dir"
fi

exit 0
