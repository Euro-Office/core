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
    depname = "libpng",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "2"
)

libpng_src = nc.work_dir / "libpng"

# Built after zlib-ng (see build_3rdparty.py)
zlib_install_dir = nc.install_dir.parent / "zlib-ng"

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

def cmake_zlib_args() -> list[str]:
    # Point at zlib-ng explicitly: find_package(ZLIB) would otherwise pick a
    # system zlib, or on Android the NDK's.
    zlib_lib = "zlibstatic.lib" if nc.is_windows() else "libz.a"
    return [
        f"-DZLIB_INCLUDE_DIR={ zlib_install_dir / 'include' }",
        f"-DZLIB_LIBRARY={ zlib_install_dir / 'lib' / zlib_lib }",
    ]

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "v1.6.59",
            "https://github.com/pnggroup/libpng.git",
            str(libpng_src),
        ],
        "Clone libpng",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    if not ( zlib_install_dir / "include" / "zlib.h" ).exists():
        nc.abort_op( f"zlib-ng not found in { zlib_install_dir } — build zlib-ng first" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libpng_build_dir = nc.work_dir / "libpng-build"

    nc.run_command(
        [
            "cmake", "-S", str(libpng_src), "-B", str(libpng_build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            *cmake_zlib_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DPNG_SHARED=OFF",
            "-DPNG_STATIC=ON",
            "-DPNG_TESTS=OFF",
            "-DPNG_TOOLS=OFF",
            # Default configuration, without generating it with awk (which
            # fails under NMake); libpng does the same on Android
            f"-DPNG_LIBCONF_HEADER={ libpng_src / 'scripts' / 'pnglibconf.h.prebuilt' }",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            *( [ "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded" ] if nc.is_windows() else [] ),
        ],
        "Configure libpng",
    )

    nc.run_command(
        [ "cmake", "--build", str(libpng_build_dir), *cmake_build_args( cpu_count ) ],
        "Build libpng",
    )

    nc.run_command(
        [ "cmake", "--install", str(libpng_build_dir) ],
        "Install libpng",
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
