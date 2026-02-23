#!/bin/bash

work_dir="$1"
install_dir="$2"
keep_work=${3:-0}

echo "Fetching CEF build script into: [$work_dir]"

mkdir -P "$work_dir"
wget -P "$work_dir" https://raw.githubusercontent.com/chromiumembedded/cef/refs/heads/master/tools/automate/automate-git.py

echo "Installing build dependencies"

export PIP_BREAK_SYSTEM_PACKAGES=1

mkdir ~/code
mkdir ~/code/automate
mkdir ~/code/chromium_git

cd ~/code
apt-get -y install curl file lsb-release procps python3 python3-pip
curl 'https://chromium.googlesource.com/chromium/src/+/main/build/install-build-deps.py?format=TEXT' | base64 -d > install-build-deps.py
python3 ./install-build-deps.py --no-arm --no-chromeos-fonts --no-nacl
python3 -m pip install dataclasses importlib_metadata


cd ~/code
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

export PATH=/root/code/depot_tools:$PATH

cd "$work_dir"
python3 automate/automate-git.py --download-dir=/root/code/chromium_git --depot-tools-dir=/root/code/depot_tools --no-distrib --no-build

cd ~/code/chromium_git
chmod 755 update.sh

./update.sh

cd "$work_dir"

echo "Building CEF"
locale-gen en_US.UTF-8
update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
export GN_DEFINES="is_official_build=true use_sysroot=false use_allocator=none symbol_level=1 is_cfi=false use_thin_lto=false use_vaapi=false"
export CEF_ARCHIVE_FORMAT=tar.bz2
python3 automate-git.py --download-dir=. --checkout=f1c41e4b1392ef8816171c4d51a2fca308db5501 --branch=5414 --minimal-distrib --client-distrib --force-clean --no-chromium-history --build-target=cefsimple --x64-build --with-pgo-profiles

exit 0
