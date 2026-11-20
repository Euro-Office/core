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
    depname = "sioxx",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "2"
)

sioxx_src = nc.work_dir / "sioxx"

def cmake_generator_args() -> list[str]:
    return [ "-G", "NMake Makefiles" ] if nc.is_windows() else []

def cmake_build_args( cpu_count: int ) -> list[str]:
    return [] if nc.is_windows() else [ f"--parallel={cpu_count}" ]

def fetch_and_patch():
    nc.create_workdir()

    # v0.3.0
    nc.shallow_checkout( sioxx_src, "https://github.com/jfayot/sioxx.git", "ec2b423d87b6d05f20b15baa96331f67b74c1971" )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    cpu_count = os.cpu_count() or 1
    build_dir = nc.work_dir / "sioxx-build"

    # Boost and nlohmann_json install dirs from the sibling deps
    boost_install_dir         = nc.install_dir.parent / "boost"
    nlohmann_json_install_dir = nc.install_dir.parent / "nlohmann_json"
    openssl_install_dir       = nc.install_dir.parent / "openssl"

    nc.run_command(
        [
            "cmake", "-S", str(sioxx_src), "-B", str(build_dir),
            *cmake_generator_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            f"-DCMAKE_PREFIX_PATH={boost_install_dir};{nlohmann_json_install_dir};{openssl_install_dir}",
            "-DSIOXX_USE_SYSTEM_BOOST=ON",
            "-DSIOXX_USE_SYSTEM_JSON=ON",
            "-DSIOXX_BUILD_EXAMPLES=OFF",
            "-DSIOXX_BUILD_TESTS=OFF",
            "-DSIOXX_INSTALL=ON",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            *( [ "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL" ] if nc.is_windows() else [] ),
        ],
        "Configure sioxx",
    )

    nc.run_command(
        [ "cmake", "--build", str(build_dir), *cmake_build_args( cpu_count ) ],
        "Build sioxx",
    )

    nc.run_command(
        [ "cmake", "--install", str(build_dir) ],
        "Install sioxx",
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
