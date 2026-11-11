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
    depname = "libtiff",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "3"
)

libtiff_src = nc.work_dir / "libtiff"
patches_dir = script_dir / "patch"

# Built after zlib-ng and libjpeg-turbo (see build_3rdparty.py)
zlib_install_dir = nc.install_dir.parent / "zlib-ng"
jpeg_install_dir = nc.install_dir.parent / "libjpeg-turbo"

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

def cmake_dependency_args() -> list[str]:
    # Point at our zlib-ng and libjpeg-turbo explicitly: find_package() would
    # otherwise pick system libraries, or on Android the NDK's zlib.
    zlib_lib = "zlibstatic.lib" if nc.is_windows() else "libz.a"
    jpeg_lib = "jpeg-static.lib" if nc.is_windows() else "libjpeg.a"
    # Forward slashes: libtiff writes these into try_compile() projects, where
    # Windows backslashes would be read as escapes
    return [
        f"-DZLIB_INCLUDE_DIR={ ( zlib_install_dir / 'include' ).as_posix() }",
        f"-DZLIB_LIBRARY={ ( zlib_install_dir / 'lib' / zlib_lib ).as_posix() }",
        f"-DJPEG_INCLUDE_DIR={ ( jpeg_install_dir / 'include' ).as_posix() }",
        f"-DJPEG_LIBRARY={ ( jpeg_install_dir / 'lib' / jpeg_lib ).as_posix() }",
        # Otherwise libtiff first looks for a libjpeg-turbo CMake package and
        # ignores the paths above, e.g. picks Homebrew's on macOS
        "-Djpeg-prefer-standard=ON",
    ]

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "v4.7.2",
            "https://gitlab.com/libtiff/libtiff.git",
            str(libtiff_src),
        ],
        "Clone libtiff",
    )

    # Uncompressed tiles failed to read without memory mapping (CxImage
    # reads through its own I/O callbacks)
    nc.run_command(
        [ "git", "apply", f"{ patches_dir / 'libtiff.patch' }" ],
        "Apply libtiff patch",
        libtiff_src
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    if not ( zlib_install_dir / "include" / "zlib.h" ).exists():
        nc.abort_op( f"zlib-ng not found in { zlib_install_dir } — build zlib-ng first" )
    if not ( jpeg_install_dir / "include" / "jpeglib.h" ).exists():
        nc.abort_op( f"libjpeg-turbo not found in { jpeg_install_dir } — build libjpeg-turbo first" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libtiff_build_dir = nc.work_dir / "libtiff-build"

    # The same codecs as the libtiff 3.9.4 that core used to bundle
    nc.run_command(
        [
            "cmake", "-S", str(libtiff_src), "-B", str(libtiff_build_dir),
            *cmake_generator_args(),
            *cmake_android_args(),
            *cmake_dependency_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-Dccitt=ON",
            "-Dpackbits=ON",
            "-Dlzw=ON",
            "-Dthunder=ON",
            "-Dnext=ON",
            "-Dlogluv=ON",
            "-Dzlib=ON",
            "-Djpeg=ON",
            "-Dold-jpeg=ON",
            "-Dmdi=OFF",
            "-Dpixarlog=OFF",
            "-Djbig=OFF",
            "-Dlerc=OFF",
            "-Dlzma=OFF",
            "-Dzstd=OFF",
            "-Dwebp=OFF",
            "-Dlibdeflate=OFF",
            "-Dtiff-cxx=OFF",
            "-Dtiff-tools=OFF",
            "-Dtiff-tests=OFF",
            "-Dtiff-contrib=OFF",
            "-Dtiff-docs=OFF",
            # libtiff sets policies of CMake 3.10, before CMP0091 made the
            # runtime library setting work; force it like for minizip-ng
            *( [ "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW", "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded" ] if nc.is_windows() else [] ),
        ],
        "Configure libtiff",
    )

    nc.run_command(
        [ "cmake", "--build", str(libtiff_build_dir), *cmake_build_args( cpu_count ) ],
        "Build libtiff",
    )

    nc.run_command(
        [ "cmake", "--install", str(libtiff_build_dir) ],
        "Install libtiff",
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
