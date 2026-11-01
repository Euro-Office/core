#!/usr/bin/env python3

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
    depname = "libjpeg-turbo",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

libjpeg_turbo_src = nc.work_dir / "libjpeg-turbo"

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
            "--branch", "3.2.1",
            "https://github.com/libjpeg-turbo/libjpeg-turbo.git",
            str(libjpeg_turbo_src),
        ],
        "Clone libjpeg-turbo",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libjpeg_turbo_build_dir = nc.work_dir / "libjpeg-turbo-build"

    # SIMD on x86 needs NASM; without it libjpeg-turbo falls back to plain C
    # (REQUIRE_SIMD is off). Arm64 uses compiler intrinsics.
    nc.run_command(
        [
            "cmake", "-S", str(libjpeg_turbo_src), "-B", str(libjpeg_turbo_build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DENABLE_SHARED=OFF",
            "-DENABLE_STATIC=ON",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-DWITH_TURBOJPEG=OFF",
            "-DWITH_TOOLS=OFF",
            "-DWITH_TESTS=OFF",
        ],
        "Configure libjpeg-turbo",
    )

    nc.run_command(
        [ "cmake", "--build", str(libjpeg_turbo_build_dir), *cmake_build_args( cpu_count ) ],
        "Build libjpeg-turbo",
    )

    nc.run_command(
        [ "cmake", "--install", str(libjpeg_turbo_build_dir) ],
        "Install libjpeg-turbo",
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
