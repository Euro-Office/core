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
    depname = "zlib-ng",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "6"
)

zlib_ng_src    = nc.work_dir / "zlib-ng"
minizip_ng_src = nc.work_dir / "minizip-ng"

def cmake_generator_args() -> list[str]:
    # Windows: NMake Makefiles, like openssl (nmake) - build_3rdparty.py
    # already requires nmake on PATH, so unlike Ninja it is always there. It
    # also builds for whatever target arch the active vcvars environment sets
    # (the default Visual Studio generator would ignore that).
    return [ "-G", "NMake Makefiles" ] if nc.is_windows() else []

def cmake_build_args( cpu_count : int ) -> list[str]:
    # NMake has no parallel mode, so don't pass --parallel there.
    return [] if nc.is_windows() else [ f"--parallel={cpu_count}" ]

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [   "git",
            "clone", "--depth", "1",
            "--branch", "2.3.3",
            "https://github.com/zlib-ng/zlib-ng.git",
            str(zlib_ng_src)
        ],
        "Clone zlib-ng"
    )

    nc.run_command(
        [   "git",
            "clone", "--depth", "1",
            "--branch", "4.2.2",
            "https://github.com/zlib-ng/minizip-ng.git",
            str(minizip_ng_src)
        ],
        "Clone minizip-ng"
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    zlib_build_dir    = nc.work_dir / "zlib-ng-build"
    minizip_build_dir = nc.work_dir / "minizip-ng-build"

    nc.run_command(
        [
            "cmake", "-S", str(zlib_ng_src), "-B", str(zlib_build_dir),
            *cmake_generator_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DZLIB_COMPAT=ON",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-DBUILD_TESTING=OFF",
            *( [ "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded" ] if nc.is_windows() else [] ),
        ],
        "Configure zlib-ng",
    )

    nc.run_command(
        [ "cmake", "--build", str(zlib_build_dir), *cmake_build_args( cpu_count ) ],
        "Build zlib-ng",
    )

    nc.run_command(
        [ "cmake", "--install", str(zlib_build_dir) ],
        "Install zlib-ng",
    )

    # Windows only: MZ_FETCH_LIBS defaults to ON on Windows and would clone
    # zlib & co from GitHub if they are not found; we want the zlib-ng built above.
    minizip_windows_args = [
        "-DMZ_FETCH_LIBS=OFF",
        "-DMZ_WZAES=OFF",       # disables mz_crypt_winvista.c → no Bcrypt.lib dep
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",  # /MT to match the main project
    ] if nc.is_windows() else []

    nc.run_command(
        [
            "cmake", "-S", str(minizip_ng_src), "-B", str(minizip_build_dir),
            *cmake_generator_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            f"-DCMAKE_PREFIX_PATH={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DMZ_ZLIB=ON",
            "-DMZ_ZLIB_FLAVOR=zlib",
            "-DMZ_BZIP2=OFF",
            "-DMZ_LZMA=OFF",
            "-DMZ_ZSTD=OFF",
            "-DMZ_PPMD=OFF",
            "-DMZ_OPENSSL=OFF",
            "-DMZ_COMPAT=ON",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            *minizip_windows_args,
        ],
        "Configure minizip-ng",
    )

    nc.run_command(
        [ "cmake", "--build", str(minizip_build_dir), *cmake_build_args( cpu_count ) ],
        "Build minizip-ng",
    )

    nc.run_command(
        [ "cmake", "--install", str(minizip_build_dir) ],
        "Install minizip-ng",
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
