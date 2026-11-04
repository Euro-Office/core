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
    depname = "zlib-ng-wasm",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

zlib_ng_src    = nc.work_dir / "zlib-ng"
minizip_ng_src = nc.work_dir / "minizip-ng"

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
    if shutil.which("emmake") is None:
        nc.abort_op( "emmake not found — activate an Emscripten environment before building zlib-ng-wasm" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    zlib_build_dir    = nc.work_dir / "zlib-ng-build"
    minizip_build_dir = nc.work_dir / "minizip-ng-build"

    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(zlib_ng_src), "-B", str(zlib_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DZLIB_COMPAT=ON",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DBUILD_TESTING=OFF",
        ],
        "Configure zlib-ng (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(zlib_build_dir), f"--parallel={cpu_count}" ],
        "Build zlib-ng",
    )

    nc.run_command(
        [ "cmake", "--install", str(zlib_build_dir) ],
        "Install zlib-ng",
    )

    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(minizip_ng_src), "-B", str(minizip_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            f"-DCMAKE_PREFIX_PATH={nc.install_dir}",
            "-DMZ_ZLIB=ON",
            "-DMZ_ZLIB_FLAVOR=zlib",
            "-DMZ_BZIP2=OFF",
            "-DMZ_LZMA=OFF",
            "-DMZ_ZSTD=OFF",
            "-DMZ_OPENSSL=OFF",
            "-DMZ_PPMD=OFF",
            # Emscripten counts as UNIX, so libbsd defaults to ON and needs
            # pkg-config, which the WASM image lacks. getrandom() is used instead.
            "-DMZ_LIBBSD=OFF",
            "-DMZ_COMPAT=ON",
            "-DBUILD_SHARED_LIBS=OFF",
        ],
        "Configure minizip-ng (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(minizip_build_dir), f"--parallel={cpu_count}" ],
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
        build_and_install()

nc.ensure_dep( build_all )
