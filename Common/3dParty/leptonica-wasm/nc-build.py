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
    depname = "leptonica-wasm",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

leptonica_src = nc.work_dir / "leptonica"

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "1.87.0",
            "https://github.com/DanBloomberg/leptonica.git",
            str(leptonica_src),
        ],
        "Clone leptonica",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    if shutil.which("emmake") is None:
        nc.abort_op( "emmake not found — activate an Emscripten environment before building leptonica-wasm" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    leptonica_build_dir = nc.work_dir / "leptonica-build"

    # Only the image processing for jbig2enc, without any image format library
    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(leptonica_src), "-B", str(leptonica_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DBUILD_PROG=OFF",
            "-DSW_BUILD=OFF",
            "-DENABLE_ZLIB=OFF",
            "-DENABLE_PNG=OFF",
            "-DENABLE_GIF=OFF",
            "-DENABLE_JPEG=OFF",
            "-DENABLE_TIFF=OFF",
            "-DENABLE_WEBP=OFF",
            "-DENABLE_OPENJPEG=OFF",
        ],
        "Configure leptonica (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(leptonica_build_dir), f"--parallel={cpu_count}" ],
        "Build leptonica",
    )

    nc.run_command(
        [ "cmake", "--install", str(leptonica_build_dir) ],
        "Install leptonica",
    )

    nc.create_install_dir_ok_marker()

    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        build_and_install()

nc.ensure_dep( build_all )
