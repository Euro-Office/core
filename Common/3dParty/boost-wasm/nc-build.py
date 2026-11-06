#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
#
# Boost for the WebAssembly build of x2t. Built with Boost's CMake superproject
# under emcmake, the way CryptPad's onlyoffice-x2t-wasm does it, because b2 has
# no reliable Emscripten toolset. Installs static libs plus the CMake package
# config, so common.cmake can find_package(Boost) for the wasm graph too.

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
    depname = "Boost-WASM",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

BOOST_TAG = "boost-1.84.0"

# Everything the converter graph links (see library/CMakeLists.txt) is
# header-only in 1.84 except filesystem and date_time. The excluded libraries
# either need threads or OS services that are not available in the wasm build.
EXCLUDED_LIBRARIES = [
    "context", "cobalt", "coroutine", "fiber", "log", "thread", "wave",
    "type_erasure", "serialization", "locale", "contract", "graph",
    "python", "mpi", "process", "stacktrace", "timer", "url", "charconv",
]

boost_src = nc.work_dir / "boost"
boost_build_dir = nc.work_dir / "boost-build"

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [   "git", "clone", "--depth", "1", "--branch", BOOST_TAG,
            "--recurse-submodules", "--shallow-submodules", "--jobs", "8",
            "https://github.com/boostorg/boost.git", str(boost_src)
        ],
        f"Clone Boost {BOOST_TAG} with submodules"
    )

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )

def build_and_install():
    if shutil.which( "emcmake" ) is None:
        nc.abort_op( "emcmake not found — activate an Emscripten environment before building boost-wasm" )

    nc.create_install_dir()

    cpu_count = os.cpu_count() or 2

    nc.run_command(
        [   "emcmake", "cmake", "-S", str(boost_src), "-B", str(boost_build_dir),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DBUILD_TESTING=OFF",
            "-DBOOST_ENABLE_CMAKE=ON",
            "-DBOOST_INSTALL_LAYOUT=system",
            f"-DBOOST_EXCLUDE_LIBRARIES={';'.join(EXCLUDED_LIBRARIES)}",
            # Boost.Regex: no ICU, matching the native build
            "-DBOOST_REGEX_ENABLE_ICU=OFF",
            # MS extensions are needed by a few headers under clang/wasm
            "-DCMAKE_CXX_FLAGS=-fms-extensions",
        ],
        "Configure Boost (WASM)",
    )

    nc.run_command(
        [ "cmake", "--build", str(boost_build_dir), f"--parallel={cpu_count}" ],
        "Build Boost (WASM)",
    )

    nc.run_command(
        [ "cmake", "--install", str(boost_build_dir) ],
        "Install Boost (WASM)",
    )

    nc.create_install_dir_ok_marker()
    print( "Build and install completed" )

def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()
    if not nc.install_dir_looks_ok():
        build_and_install()

nc.ensure_dep( build_all )
