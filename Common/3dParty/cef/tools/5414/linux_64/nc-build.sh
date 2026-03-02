#!/bin/bash


mkdir ~/code
mkdir ~/code/automate
mkdir ~/code/chromium_git

cd ~/code
apt-get -y install curl file lsb-release procps python3 python3-pip
curl 'https://chromium.googlesource.com/chromium/src/+/main/build/install-build-deps.py?format=TEXT' | base64 -d > install-build-deps.py
python3 ./install-build-deps.py --no-arm --no-chromeos-fonts --no-nacl
python3 -m pip install dataclasses importlib_metadata

cd ~/code/automate
wget https://raw.githubusercontent.com/chromiumembedded/cef/master/tools/automate/automate-git.py

locale-gen en_US.UTF-8
update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8
export GN_DEFINES="is_official_build=true use_sysroot=false use_allocator=none symbol_level=1 is_cfi=false use_thin_lto=false use_vaapi=false"
export CEF_ARCHIVE_FORMAT=tar.bz2

# --- HELPER FUNCTION: Retry Logic ---
function run_gclient_recovery() {
    echo "!!! automate-git.py failed. Attempting to recover via gclient sync..."

    # CEF's automate-git.py downloads depot_tools inside the download-dir. 
    # We must add it to PATH so we can use 'gclient'.
    export PATH=$PATH:~/code/chromium_git/depot_tools

    # Go to the directory where .gclient file is located
    cd ~/code/chromium_git/chromium

    # Loop until gclient succeeds
    local RETRY_COUNT=0
    local MAX_RETRIES=20
    
    until gclient sync --force --reset --with_branch_heads --with_tags; do
        RETRY_COUNT=$((RETRY_COUNT+1))
        if [ $RETRY_COUNT -gt $MAX_RETRIES ]; then
             echo "Failed to sync after $MAX_RETRIES attempts. Exiting."
             exit 1
        fi
        echo "gclient sync failed (Attempt $RETRY_COUNT/$MAX_RETRIES). Retrying in 10 seconds..."
        sleep 10
    done

    echo "!!! gclient sync succeeded. Resuming build..."
    
    # Return to automate dir to re-run the main script
    cd ~/code/chromium_git
    rm -r cef
    rm -r depot_tools

    cd ~/code/automate
}

# --- MAIN BUILD EXECUTION ---

# We wrap the build command in a loop. 
# If it fails, we run recovery, then try the build command again.
BUILD_SUCCESS=false
COUNT=0
MAX_TRIES=3
while [ "$BUILD_SUCCESS" = false ]; do
    python3 ../automate/automate-git.py \
        --download-dir=/root/code/chromium_git \
        --checkout=f1c41e4b1392ef8816171c4d51a2fca308db5501 \
        --branch=5414 \
        --minimal-distrib \
        --client-distrib \
        --force-clean \
        --no-chromium-history \
        --build-target=cefsimple \
        --x64-build \
        --with-pgo-profiles

    if [ $? -eq 0 ]; then
        BUILD_SUCCESS=true
        echo "Build completed successfully!"
    else
        run_gclient_recovery
        COUNT=$((COUNT+1))
        if [ $COUNT -gt $MAX_TRIES ]; then
            cd ~/code/chromium_git
            rm -r chromium
            COUNT=0
            cd ~/code/automate
        fi
    fi
done


apt-get update && apt-get install -y \
        lsb-release \
        ninja-build \
        software-properties-common \
        gnupg \
        && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
        && echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
        && apt-get update \
        && apt-get install -y cmake \
        && apt-get clean && rm -rf /var/lib/apt/lists/*

cd ~/code/chromium_git/chromium/src/cef/binary_distrib/cef_binary_*_linux64
mkdir dll-wrapper-release
cd dll-wrapper-release
cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target libcef_dll_wrapper

cd ~/code/chromium_git/chromium/src/cef/binary_distrib/cef_binary_*_linux64
mkdir dll-wrapper-debug
cd dll-wrapper-debug
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --target libcef_dll_wrapper

cd ~/code/chromium_git/chromium/src/cef/binary_distrib

tar -cjf /cef_binary.tar.bz2 cef_binary_*_linux64

exit 0
