#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

import sys
import os
import shutil
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "libaom",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "3"
)

libaom_src = nc.work_dir / "aom"

def cmake_generator_args() -> list[str]:
    return [ "-G", "NMake Makefiles" ] if nc.is_windows() else []

def cmake_build_args( cpu_count : int ) -> list[str]:
    return [] if nc.is_windows() else [ f"--parallel={cpu_count}" ]

def cmake_android_args() -> list[str]:
    # Cross-compile with the NDK's toolchain file for the target ABI
    if not nc.is_android():
        return []
    return [
        f"-DCMAKE_TOOLCHAIN_FILE={ nc.android_ndk_root() / 'build' / 'cmake' / 'android.toolchain.cmake' }",
        f"-DANDROID_ABI={ nc.android_abi() }",
        f"-DANDROID_PLATFORM=android-{ nc.ANDROID_API_LEVEL }",
    ]

def cmake_cpu_args() -> list[str]:
    # x86 SIMD needs yasm or nasm, otherwise libaom refuses to configure.
    # Without one, build plain C: speed hardly matters for decoding single
    # images. Arm uses compiler intrinsics.
    if nc.is_arm64() or shutil.which("nasm") or shutil.which("yasm"):
        return []
    return [ "-DAOM_TARGET_CPU=generic" ]

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "v3.15.3",
            "https://aomedia.googlesource.com/aom",
            str(libaom_src),
        ],
        "Clone libaom",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libaom_build_dir = nc.work_dir / "aom-build"

    # The AV1 decoder only (for AVIF in libheif): no encoder, apps or tests.
    # Single-threaded, like libwebp, so linking it needs nothing else.
    nc.run_command(
        [
            "cmake", "-S", str(libaom_src), "-B", str(libaom_build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            *cmake_cpu_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-DCONFIG_AV1_ENCODER=0",
            "-DCONFIG_MULTITHREAD=0",
            "-DENABLE_APPS=OFF",
            "-DENABLE_DOCS=OFF",
            "-DENABLE_EXAMPLES=OFF",
            "-DENABLE_TESTDATA=OFF",
            "-DENABLE_TESTS=OFF",
            "-DENABLE_TOOLS=OFF",
            *( [ "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL" ] if nc.is_windows() else [] ),
        ],
        "Configure libaom",
    )

    nc.run_command(
        [ "cmake", "--build", str(libaom_build_dir), *cmake_build_args( cpu_count ) ],
        "Build libaom",
    )

    nc.run_command(
        [ "cmake", "--install", str(libaom_build_dir) ],
        "Install libaom",
    )

    nc.create_install_dir_ok_marker()

    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        if nc.is_windows() and shutil.which("nmake") is None:
            raise RuntimeError(
                "MSVC environment is not set up: 'nmake' not found in PATH.\n"
                "Run 'vcvars64.bat' (or vcvarsarm64.bat) or use a Native Tools Command Prompt."
            )
        build_and_install()

nc.ensure_dep( build_all )
