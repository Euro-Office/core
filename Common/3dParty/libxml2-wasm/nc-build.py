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
    depname = "libxml2-wasm",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

libxml2_src = nc.work_dir / "libxml2"

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [
            "git", "clone", "--depth", "1",
            "--branch", "v2.15.4",
            "https://gitlab.gnome.org/GNOME/libxml2.git",
            str(libxml2_src),
        ],
        "Clone libxml2",
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    if shutil.which("emmake") is None:
        nc.abort_op( "emmake not found — activate an Emscripten environment before building libxml2-wasm" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libxml2_build_dir = nc.work_dir / "libxml2-build"

    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(libxml2_src), "-B", str(libxml2_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DLIBXML2_WITH_PYTHON=OFF",
            "-DLIBXML2_WITH_PROGRAMS=OFF",
            "-DLIBXML2_WITH_TESTS=OFF",
            "-DLIBXML2_WITH_LZMA=OFF",
            "-DLIBXML2_WITH_ZLIB=OFF",
            "-DLIBXML2_WITH_ICONV=OFF",
            "-DLIBXML2_WITH_ICU=OFF",
            "-DLIBXML2_WITH_THREADS=OFF",
        ],
        "Configure libxml2 (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(libxml2_build_dir), f"--parallel={cpu_count}" ],
        "Build libxml2",
    )

    nc.run_command(
        [ "cmake", "--install", str(libxml2_build_dir) ],
        "Install libxml2",
    )

    nc.create_install_dir_ok_marker()

    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        build_and_install()

nc.ensure_dep( build_all )
