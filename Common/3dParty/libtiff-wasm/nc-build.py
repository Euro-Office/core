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
    depname = "libtiff-wasm",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

libtiff_src = nc.work_dir / "libtiff"
patches_dir = script_dir.parent / "libtiff" / "patch"

# Built after zlib-ng-wasm and libjpeg-turbo-wasm (see build_3rdparty.py)
zlib_install_dir = nc.install_dir.parent / "zlib-ng-wasm"
jpeg_install_dir = nc.install_dir.parent / "libjpeg-turbo-wasm"

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
    if shutil.which("emmake") is None:
        nc.abort_op( "emmake not found — activate an Emscripten environment before building libtiff-wasm" )
    if not ( zlib_install_dir / "include" / "zlib.h" ).exists():
        nc.abort_op( f"zlib-ng-wasm not found in { zlib_install_dir } — build zlib-ng-wasm first" )
    if not ( jpeg_install_dir / "include" / "jpeglib.h" ).exists():
        nc.abort_op( f"libjpeg-turbo-wasm not found in { jpeg_install_dir } — build libjpeg-turbo-wasm first" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1

    libtiff_build_dir = nc.work_dir / "libtiff-build"

    # The same codecs as the libtiff 3.9.4 that core used to bundle
    nc.run_command(
        [
            "emcmake", "cmake", "-S", str(libtiff_src), "-B", str(libtiff_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            f"-DZLIB_INCLUDE_DIR={ zlib_install_dir / 'include' }",
            f"-DZLIB_LIBRARY={ zlib_install_dir / 'lib' / 'libz.a' }",
            f"-DJPEG_INCLUDE_DIR={ jpeg_install_dir / 'include' }",
            f"-DJPEG_LIBRARY={ jpeg_install_dir / 'lib' / 'libjpeg.a' }",
            "-DBUILD_SHARED_LIBS=OFF",
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
        ],
        "Configure libtiff (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(libtiff_build_dir), f"--parallel={cpu_count}" ],
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
        build_and_install()

nc.ensure_dep( build_all )
