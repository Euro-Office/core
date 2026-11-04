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
    depname = "leptonica",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

leptonica_src = nc.work_dir / "leptonica"

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

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "1.87.0",
            "https://github.com/DanBloomberg/leptonica.git",
            str(leptonica_src),
        ],
        "Clone leptonica",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    leptonica_build_dir = nc.work_dir / "leptonica-build"

    # Only the image processing for jbig2enc: like the Leptonica 1.67 that
    # core used to bundle, without any image format library
    nc.run_command(
        [
            "cmake", "-S", str(leptonica_src), "-B", str(leptonica_build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-DBUILD_PROG=OFF",
            "-DSW_BUILD=OFF",
            "-DENABLE_ZLIB=OFF",
            "-DENABLE_PNG=OFF",
            "-DENABLE_GIF=OFF",
            "-DENABLE_JPEG=OFF",
            "-DENABLE_TIFF=OFF",
            "-DENABLE_WEBP=OFF",
            "-DENABLE_OPENJPEG=OFF",
            # Leptonica sets policies of CMake 3.10, before CMP0091 made the
            # runtime library setting work; force it like for libtiff
            *( [ "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW", "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded" ] if nc.is_windows() else [] ),
        ],
        "Configure leptonica",
    )

    nc.run_command(
        [ "cmake", "--build", str(leptonica_build_dir), *cmake_build_args( cpu_count ) ],
        "Build leptonica",
    )

    nc.run_command(
        [ "cmake", "--install", str(leptonica_build_dir) ],
        "Install leptonica",
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
