#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Configures and builds core on Linux and macOS, natively or for Android.
# The Dockerfiles in .docker/, the CI workflows and the downstream repos all
# build core through this script, so they get the same binaries. Windows:
# build.ps1.
#
# Usage: scripts/build.sh [options] [-- extra cmake arguments]
#   --build-dir DIR    CMake build directory (default: build)
#   --output DIR       build output (default: <build-dir>/package): the
#                      libraries and x2t in bin/, the tools in tools/, the
#                      static and import libraries in lib/ (a core package,
#                      see cmake/prebuilt-core.cmake); for Android the
#                      stripped libraries, flat
#   --tests            also build the unit tests (run them with ctest)
#   --ccache           compile through ccache
#   --android ABI      cross-compile x2t's libraries for an Android ABI
#                      (arm64-v8a, x86_64); needs ANDROID_NDK_ROOT
#   --android-api N    minimum Android API level (default: 26)
#
# Environment: VCPKG_ROOT (not for Android); CORE_PYTHON, the Python for
# the third-party builds (default: the one CMake finds).

set -euo pipefail

core="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir=build
output=
tests=OFF
ccache=
abi=
api=26

usage() { sed -n '/^# Usage/,/^# Environment/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit "$1"; }

while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir)   build_dir="$2"; shift 2 ;;
        --output)      output="$2"; shift 2 ;;
        --tests)       tests=ON; shift ;;
        --ccache)      ccache=1; shift ;;
        --android)     abi="$2"; shift 2 ;;
        --android-api) api="$2"; shift 2 ;;
        -h|--help)     usage 0 ;;
        --)            shift; break ;;
        *)             echo "unknown option: $1" >&2; usage 1 ;;
    esac
done

mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
output="${output:-$build_dir/package}"
mkdir -p "$output"
output="$(cd "$output" && pwd)"

args=( -G Ninja -S "$core" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release )
[ -n "${CORE_PYTHON:-}" ] && args+=( -DPYTHON_BIN="$CORE_PYTHON" )
if [ -n "$ccache" ]; then
    args+=( -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache )
    # Otherwise ccache skips every source of a target with precompiled headers
    # (about half of core): a fresh .gch is always newer than the compile
    export CCACHE_SLOPPINESS=pch_defines,time_macros,include_file_mtime,include_file_ctime
fi

if [ -n "$abi" ]; then
    : "${ANDROID_NDK_ROOT:?ANDROID_NDK_ROOT must point at the NDK}"
    args+=(
        -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake"
        -DANDROID_ABI="$abi"
        -DANDROID_PLATFORM="android-$api"
        -DANDROID_STL=c++_shared
        -DEO_CORE_OUTPUT_DIR="$build_dir/package"
    )
else
    : "${VCPKG_ROOT:?VCPKG_ROOT must point at vcpkg}"
    args+=(
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
        -DVCPKG_MANIFEST_MODE=ON
        -DVCPKG_MANIFEST_DIR="$core"
        -DEO_CORE_OUTPUT_DIR="$output/bin"
        -DEO_CORE_TOOLS_DIR="$output/tools"
        -DEO_CORE_LIB_DIR="$output/lib"
        -DEO_BUILD_TESTS="$tests"
    )
    [ "$tests" = ON ] && args+=( -DVCPKG_MANIFEST_FEATURES=tests )
    case "$(uname -s)-$(uname -m)" in
        # -O3 without -DNDEBUG, as the Linux release images always did
        Linux-*)        args+=( -DCMAKE_C_FLAGS_RELEASE=-O3 -DCMAKE_CXX_FLAGS_RELEASE=-O3 ) ;;
        Darwin-arm64)   args+=( -DVCPKG_TARGET_TRIPLET=arm64-osx ) ;;
        Darwin-x86_64)  args+=( -DVCPKG_TARGET_TRIPLET=x64-osx ) ;;
    esac
fi

cmake "${args[@]}" "$@"

if [ -z "$abi" ]; then
    cmake --build "$build_dir"
    exit 0
fi

# Android: only x2t and the libraries it and the editor engine load
cmake --build "$build_dir" --target x2tlib
cp "$build_dir"/package/*.so "$output"/
find "$build_dir" -name libx2t.so -exec cp {} "$output"/ \;
# The NDK's toolchain always adds -g: the build tree keeps the unstripped ones
"$ANDROID_NDK_ROOT"/toolchains/llvm/prebuilt/*/bin/llvm-strip --strip-unneeded "$output"/*.so
ls -1 "$output"
