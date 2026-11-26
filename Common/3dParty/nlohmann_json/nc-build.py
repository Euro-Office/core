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

# Header-only. We still run cmake --install so that the nlohmann_jsonConfig.cmake
# package files are present for downstream find_package(nlohmann_json) calls
# (e.g. from sioxx).
nc.init_for_dep(
    depname = "nlohmann_json",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "2"
)

nlohmann_json_src = nc.work_dir / "nlohmann_json"

def cmake_generator_args() -> list[str]:
    return [ "-G", "NMake Makefiles" ] if nc.is_windows() else []

def fetch_and_patch():
    nc.create_workdir()

    # v3.12.0
    nc.shallow_checkout( nlohmann_json_src, "https://github.com/nlohmann/json.git", "65ee68451d8eb2b5f3a30b410476ab83deb3289b" )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    build_dir = nc.work_dir / "nlohmann_json-build"

    nc.run_command(
        [
            "cmake", "-S", str(nlohmann_json_src), "-B", str(build_dir),
            *cmake_generator_args(),
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={nc.install_dir}",
            "-DJSON_BuildTests=OFF",
            "-DJSON_Install=ON",
        ],
        "Configure nlohmann_json",
    )

    nc.run_command(
        [ "cmake", "--install", str(build_dir) ],
        "Install nlohmann_json",
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
