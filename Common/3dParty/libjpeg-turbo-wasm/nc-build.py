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
    depname = "libjpeg-turbo-wasm",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

libjpeg_turbo_src = nc.work_dir / "libjpeg-turbo"

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
    if shutil.which("emmake") is None:
        nc.abort_op( "emmake not found — activate an Emscripten environment before building libjpeg-turbo-wasm" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libjpeg_turbo_build_dir = nc.work_dir / "libjpeg-turbo-build"

    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(libjpeg_turbo_src), "-B", str(libjpeg_turbo_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DENABLE_SHARED=OFF",
            "-DENABLE_STATIC=ON",
            "-DWITH_SIMD=OFF",
            "-DWITH_TURBOJPEG=OFF",
            "-DWITH_TOOLS=OFF",
            "-DWITH_TESTS=OFF",
        ],
        "Configure libjpeg-turbo (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(libjpeg_turbo_build_dir), f"--parallel={cpu_count}" ],
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
        build_and_install()

nc.ensure_dep( build_all )
