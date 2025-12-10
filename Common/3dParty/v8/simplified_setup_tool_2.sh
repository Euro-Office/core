#!/bin/bash

# This script attempts to fetch and patch and setup v8 for linux x64 only.
# See v8_89.py in the build_tools repo.

if [ -f v8/out.gn/linux_64/obj/libv8_monolith.a ]
then
    echo "Skipping V8 setup, it's already done!"
    exit 0
fi

if [ ! -d depot_tools ]
then
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    cd depot_tools
    git apply ../bootstrap.patch
    cd ..
fi

export PATH=$PATH:$PWD/depot_tools

if [ ! -d ./depot_tools/v8 ]
then
    ./depot_tools/fetch v8
fi

cp -r ./v8/third_party ./v8/third_party_new

v8_branch_version="remotes/branch-heads/8.9"
./depot_tools/gclient sync -r $v8_branch_version
gclient sync --force
cp -r ./v8/third_party_new/ninja ./v8/third_party/ninja

cd depot_tools
git apply ../gclient_paths.patch
cd ..

cd v8/third_party/jinja2
git apply ../../../jinja2.patch
cd ../.. # should be in v8

git apply ../buildgn.patch

: "${PLEASE_PRELOAD_LIBSTDCPP:=false}"
if [ -f /.dockerenv ] || [ "$PLEASE_PRELOAD_LIBSTDCPP" = "true" ]
then
    export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6
fi

gn gen out.gn/linux_64 --args='v8_static_library=true is_component_build=false v8_monolithic=true v8_use_external_startup_data=false treat_warnings_as_errors=false use_gold=false target_cpu="x64" v8_target_cpu="x64" is_debug=false use_sysroot=false use_custom_libcxx=false'
# gn gen out.gn/linux_64 --args-file=../linux_64_args.gn
# gn gen out.gn/linux_64 --args='cflags_c=["-include","stdint.h"] cflags_cc=["-include","cstdint"] is_clang=false is_component_build=false is_debug=false target_cpu="x64" treat_warnings_as_errors=false use_custom_libcxx=false use_glib=true use_sysroot=false v8_monolithic=true v8_static_library=true v8_target_cpu="x64" v8_use_external_startup_data=false use_gold=false'
# gn gen out.gn/linux_64 --args='cflags_cc=["-Wno-template-id-cdtor"] is_clang=false is_component_build=false is_debug=false target_cpu="x64" treat_warnings_as_errors=false use_custom_libcxx=false use_glib=true use_sysroot=false v8_monolithic=true v8_static_library=true v8_target_cpu="x64" v8_use_external_startup_data=false use_gold=false'
# gn gen out.gn/linux_64 --args='cxx_std="c++17" is_clang=false is_component_build=false is_debug=false target_cpu="x64" treat_warnings_as_errors=false use_custom_libcxx=false use_glib=true use_sysroot=false v8_monolithic=true v8_static_library=true v8_target_cpu="x64" v8_use_external_startup_data=false use_gold=false'

ninja -C out.gn/linux_64

# cd out.gn/linux_64
# ar rcs obj/libv8_monolith.a $(find . -name "*.o")
# cd ../../..

