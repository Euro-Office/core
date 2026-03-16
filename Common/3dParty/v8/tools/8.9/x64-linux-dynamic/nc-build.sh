#!/bin/bash

work_dir="$1"
install_dir="$2"

script_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
patches_dir="$script_dir"

abort_op()
{
    rm -rf "$work_dir"
    rm -rf "$install_dir"
    echo "v8 aborted: $1" >&2
    exit 1
}

if [ $# -lt 2 ]
then
    echo "Needs 2 arguments: work_dir install_dir_path" >&2
    exit 1
fi

if [ -f "$install_dir/libv8_monolith.a" ]
then
    echo "Skipping v8 (done already)."
    exit 0
fi

if [ -d "$install_dir" ]
then
    rm -rf "$install_dir"
fi
mkdir -p "$install_dir" || abort_op "Failed to create install dir: [$install_dir]"

if [ -d "$work_dir" ]
then
    rm -rf "$work_dir"
fi
mkdir -p "$work_dir" || abort_op "Failed to create work dir: [$work_dir]"

git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git "$work_dir/depot_tools" \
    || abort_op "Failed to clone depot_tools repo"
cd "$work_dir/depot_tools"
git apply "$patches_dir/bootstrap.patch" || abort_op "Failed to apply bootstrap.patch"

export PATH=$PATH:"$work_dir/depot_tools"
cd "$work_dir"
./depot_tools/fetch v8 || abort_op "Depot tools failed to fetch v8"

cp -r ./v8/third_party ./v8/third_party_new

v8_branch_version="remotes/branch-heads/8.9"
./depot_tools/gclient sync -r $v8_branch_version
gclient sync --force
cp -r ./v8/third_party_new/ninja ./v8/third_party/ninja

cd "$work_dir/depot_tools"
git apply "$patches_dir/gclient_paths.patch" || abort_op "Failed to apply gclient_paths.patch"

cd "$work_dir/v8/third_party/jinja2"
git apply "$patches_dir/jinja2.patch" || abort_op "Failed to apply jijnja2.patch"

cd "$work_dir/v8"
git apply "$patches_dir/buildgn.patch" || abort_op "Failed to apply buildgn.patch"

: "${PLEASE_PRELOAD_LIBSTDCPP:=false}"
if [ -f /.dockerenv ] || [ "$PLEASE_PRELOAD_LIBSTDCPP" = "true" ]
then
    export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6
fi

gn gen out.gn/linux_64 --args='v8_static_library=true is_component_build=false v8_monolithic=true v8_use_external_startup_data=false treat_warnings_as_errors=false use_gold=false target_cpu="x64" v8_target_cpu="x64" is_debug=false use_sysroot=false use_custom_libcxx=false' \
    || abort_op "Gn gen failed"

ninja -C out.gn/linux_64 || abort_op "Ninja build failed"

cp "$work_dir/v8/out.gn/linux_64/obj/libv8_monolith.a" "$install_dir/libv8_monolith.a" \
    || abort_op "Install failed (libv8_monolith.a)"
mkdir -p "$install_dir/v8/include" || abort_op "Failed to create directory: [$install_dir/v8/include]"
mkdir -p "$install_dir/v8/src/base" || abort_op "Failed to create directory: [$install_dir/v8/src/base]"
cp -r "$work_dir/v8/include" "$install_dir/v8/" || abort_op "Install failed (v8/include)"

additional_base_headers_to_install="base-export.h compiler-specific.h sys-info.h"
for f in $additional_base_headers_to_install
do
    cp "$work_dir/v8/src/base/$f" "$install_dir/v8/src/base/$f" || abort_op "Failed to install ($f)"
done

echo "v8 ready! (work dir will be removed)"
rm -rf "$work_dir"
