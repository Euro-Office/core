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
    depname = "jbig2enc",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "4",
    deps = [ "leptonica-wasm" if nc.is_wasm() else "leptonica" ],)

jbig2enc_src = nc.work_dir / "jbig2enc"

def cmake_configure_cmd() -> list[str]:
    if nc.is_wasm():
        if shutil.which( "emcmake" ) is None:
            nc.abort_op( "emcmake not found — activate an Emscripten environment before building jbig2enc for WASM" )
        return [ "emcmake", "cmake" ]
    return [ "cmake" ]

def cmake_generator_args() -> list[str]:
    return [ "-G", "NMake Makefiles" ] if nc.is_windows() else []

def cmake_build_args( cpu_count: int ) -> list[str]:
    return [] if nc.is_windows() else [ f"--parallel={cpu_count}" ]

def cmake_android_args() -> list[str]:
    if not nc.is_android():
        return []
    return [
        f"-DCMAKE_TOOLCHAIN_FILE={ nc.android_ndk_root() / 'build' / 'cmake' / 'android.toolchain.cmake' }",
        f"-DANDROID_ABI={ nc.android_abi() }",
        f"-DANDROID_PLATFORM=android-{ nc.ANDROID_API_LEVEL }",
    ]

def fetch_and_patch():
    nc.create_workdir()

    # 0.32
    nc.shallow_checkout( jbig2enc_src, "https://github.com/agl/jbig2enc.git", "309b2d55c7dfdcf0ab6afccb6d88834afc0bf2c0" )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1
    build_dir = nc.work_dir / "jbig2enc-build"

    leptonica_name = "leptonica-wasm" if nc.is_wasm() else "leptonica"
    leptonica_install_dir = nc.install_dir.parent / leptonica_name

    nc.run_command(
        [
            *cmake_configure_cmd(), "-S", str(jbig2enc_src), "-B", str(build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            f"-DCMAKE_PREFIX_PATH={leptonica_install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
        ],
        "Configure jbig2enc",
    )

    nc.run_command(
        [ "cmake", "--build", str(build_dir), *cmake_build_args( cpu_count ) ],
        "Build jbig2enc",
    )

    if nc.is_wasm():
        # cmake --install fails for WASM because Emscripten compiles the jbig2
        # tool to jbig2.js, not a native binary, so cmake can't find the file
        # it expects to install. Copy only what common.cmake consumes.
        ( nc.install_dir / "lib" ).mkdir( parents=True, exist_ok=True )
        ( nc.install_dir / "include" ).mkdir( parents=True, exist_ok=True )
        shutil.copy2( str( build_dir / "liblibjbig2enc.a" ), str( nc.install_dir / "lib" / "liblibjbig2enc.a" ) )
        shutil.copy2( str( jbig2enc_src / "src" / "jbig2enc.h" ), str( nc.install_dir / "include" / "jbig2enc.h" ) )
    else:
        nc.run_command(
            [ "cmake", "--install", str(build_dir) ],
            "Install jbig2enc",
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
