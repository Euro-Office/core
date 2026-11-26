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
    depname = "libheif",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "3"
)

libheif_src = nc.work_dir / "libheif"

# Built after libde265 and libaom (see build_3rdparty.py)
libde265_install_dir = nc.install_dir.parent / "libde265"
libaom_install_dir = nc.install_dir.parent / "libaom"

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

def static_lib( name : str ) -> str:
    return f"{name}.lib" if nc.is_windows() else f"lib{name}.a"

def windows_static_env() -> dict[ str, str ] | None:
    # On Windows the libde265 and libheif headers declare their functions
    # dllimport unless told the libraries are static. CMake adds its default
    # flags to CFLAGS/CXXFLAGS.
    if not nc.is_windows():
        return None
    flags = "/DLIBDE265_STATIC_BUILD /DLIBHEIF_STATIC_BUILD"
    return { "CFLAGS": flags, "CXXFLAGS": flags }

def cmake_dependency_args() -> list[str]:
    # Point at our libde265 and libaom explicitly: find_package() would
    # otherwise pick system libraries (libaom's CMake package is tried first)
    return [
        f"-DLIBDE265_INCLUDE_DIR={ ( libde265_install_dir / 'include' ).as_posix() }",
        f"-DLIBDE265_LIBRARY={ ( libde265_install_dir / 'lib' / ( 'libde265.lib' if nc.is_windows() else 'libde265.a' ) ).as_posix() }",
        f"-DAOM_DIR={ ( libaom_install_dir / 'lib' / 'cmake' / 'AOM' ).as_posix() }",
        f"-DAOM_INCLUDE_DIR={ ( libaom_install_dir / 'include' ).as_posix() }",
        f"-DAOM_LIBRARY={ ( libaom_install_dir / 'lib' / static_lib( 'aom' ) ).as_posix() }",
        # The Android toolchain only searches the NDK sysroot for packages
        "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH",
    ]

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "v1.23.7",
            "https://github.com/strukturag/libheif.git",
            str(libheif_src),
        ],
        "Clone libheif",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    if not ( libde265_install_dir / "include" / "libde265" / "de265.h" ).exists():
        nc.abort_op( f"libde265 not found in { libde265_install_dir } — build libde265 first" )
    if not ( libaom_install_dir / "include" / "aom" / "aom_decoder.h" ).exists():
        nc.abort_op( f"libaom not found in { libaom_install_dir } — build libaom first" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libheif_build_dir = nc.work_dir / "libheif-build"

    # Decoding only: HEIC through libde265, AVIF through libaom. No encoders
    # (x265 is GPL), no plugins, tools or other codecs. Decodes on the
    # caller's thread, like libwebp.
    nc.run_command(
        [
            "cmake", "-S", str(libheif_src), "-B", str(libheif_build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            *cmake_dependency_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-DENABLE_PLUGIN_LOADING=OFF",
            "-DENABLE_MULTITHREADING_SUPPORT=OFF",
            "-DWITH_LIBDE265=ON",
            "-DWITH_LIBDE265_PLUGIN=OFF",
            "-DWITH_AOM_DECODER=ON",
            "-DWITH_AOM_DECODER_PLUGIN=OFF",
            "-DWITH_AOM_ENCODER=OFF",
            "-DWITH_X265=OFF",
            "-DWITH_X264=OFF",
            "-DWITH_KVAZAAR=OFF",
            "-DWITH_UVG266=OFF",
            "-DWITH_VVDEC=OFF",
            "-DWITH_VVENC=OFF",
            "-DWITH_OpenH264_DECODER=OFF",
            "-DWITH_DAV1D=OFF",
            "-DWITH_SvtEnc=OFF",
            "-DWITH_RAV1E=OFF",
            "-DWITH_JPEG_DECODER=OFF",
            "-DWITH_JPEG_ENCODER=OFF",
            "-DWITH_OpenJPEG_DECODER=OFF",
            "-DWITH_OpenJPEG_ENCODER=OFF",
            "-DWITH_FFMPEG_DECODER=OFF",
            "-DWITH_OPENJPH_ENCODER=OFF",
            "-DWITH_UNCOMPRESSED_CODEC=OFF",
            "-DWITH_LIBSHARPYUV=OFF",
            "-DWITH_HEADER_COMPRESSION=OFF",
            "-DWITH_EXAMPLES=OFF",
            "-DWITH_EXAMPLE_HEIF_THUMB=OFF",
            "-DWITH_EXAMPLE_HEIF_VIEW=OFF",
            "-DWITH_GDK_PIXBUF=OFF",
            "-DBUILD_TESTING=OFF",
            "-DBUILD_DOCUMENTATION=OFF",
            *( [ "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL" ] if nc.is_windows() else [] ),
        ],
        "Configure libheif",
        env = windows_static_env(),
    )

    nc.run_command(
        [ "cmake", "--build", str(libheif_build_dir), *cmake_build_args( cpu_count ) ],
        "Build libheif",
    )

    nc.run_command(
        [ "cmake", "--install", str(libheif_build_dir) ],
        "Install libheif",
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
