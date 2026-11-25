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
    depname = "Hyphen",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "3")

# Pinned to master tip on the hunspell/hyphen repo (2026-11-24).
HYPHEN_REPO   = "https://github.com/hunspell/hyphen"
HYPHEN_COMMIT = "c281a3334362f78a1a6a85561807a63856afa1ae"


def cmake_configure_cmd() -> list:
    if nc.is_wasm():
        if shutil.which( "emcmake" ) is None:
            nc.abort_op( "emcmake not found — activate an Emscripten environment before building hyphen for WASM" )
        return [ "emcmake", "cmake" ]
    return [ "cmake" ]


def cmake_generator_args() -> list:
    return [ "-G", "NMake Makefiles" ] if nc.is_windows() else []


def cmake_build_args( cpu_count: int ) -> list:
    return [] if nc.is_windows() else [ f"--parallel={cpu_count}" ]


def cmake_android_args() -> list:
    if not nc.is_android():
        return []
    return [
        f"-DCMAKE_TOOLCHAIN_FILE={ nc.android_ndk_root() / 'build' / 'cmake' / 'android.toolchain.cmake' }",
        f"-DANDROID_ABI={ nc.android_abi() }",
        f"-DANDROID_PLATFORM=android-{ nc.ANDROID_API_LEVEL }",
    ]


def fetch_and_patch():
    nc.create_workdir()
    nc.shallow_checkout( nc.work_dir, HYPHEN_REPO, HYPHEN_COMMIT )
    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1
    build_dir = nc.work_dir / "build"

    nc.run_command(
        [
            *cmake_configure_cmd(),
            "-S", str(script_dir), "-B", str(build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            f"-DHYPHEN_SRC_DIR={ nc.work_dir }",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={ nc.install_dir }",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
        ],
        "Configure hyphen",
    )

    nc.run_command(
        [ "cmake", "--build", str(build_dir), *cmake_build_args( cpu_count ) ],
        "Build hyphen",
    )

    nc.run_command(
        [ "cmake", "--install", str(build_dir) ],
        "Install hyphen",
    )

    nc.create_install_dir_ok_marker()


if not nc.work_dir_looks_ok():
    fetch_and_patch()

if not nc.install_dir_looks_ok():
    build_and_install()
