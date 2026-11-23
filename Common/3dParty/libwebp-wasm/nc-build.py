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
    depname = "libwebp-wasm",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

libwebp_src = nc.work_dir / "libwebp"

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "v1.6.0",
            "https://github.com/webmproject/libwebp.git",
            str(libwebp_src),
        ],
        "Clone libwebp",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    if shutil.which("emmake") is None:
        nc.abort_op( "emmake not found — activate an Emscripten environment before building libwebp-wasm" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libwebp_build_dir = nc.work_dir / "libwebp-build"

    # The libraries only (libwebp, libsharpyuv, libwebpdemux, libwebpmux),
    # without the tools and their image format libraries
    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(libwebp_src), "-B", str(libwebp_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DWEBP_BUILD_ANIM_UTILS=OFF",
            "-DWEBP_BUILD_CWEBP=OFF",
            "-DWEBP_BUILD_DWEBP=OFF",
            "-DWEBP_BUILD_GIF2WEBP=OFF",
            "-DWEBP_BUILD_IMG2WEBP=OFF",
            "-DWEBP_BUILD_VWEBP=OFF",
            "-DWEBP_BUILD_WEBPINFO=OFF",
            "-DWEBP_BUILD_WEBPMUX=OFF",
            "-DWEBP_BUILD_EXTRAS=OFF",
            "-DWEBP_USE_THREAD=OFF",
        ],
        "Configure libwebp (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(libwebp_build_dir), f"--parallel={cpu_count}" ],
        "Build libwebp",
    )

    nc.run_command(
        [ "cmake", "--install", str(libwebp_build_dir) ],
        "Install libwebp",
    )

    nc.create_install_dir_ok_marker()

    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        build_and_install()

nc.ensure_dep( build_all )
