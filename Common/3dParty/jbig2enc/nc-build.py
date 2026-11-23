#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

import sys
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

# Source only: core compiles jbig2enc into graphics (JBIG2ENC_SOURCES_DIR in
# common.cmake). Its own CMake build forces the /MD runtime and always builds
# the jbig2 tool.
nc.init_for_dep(
    depname = "jbig2enc",
    workdir = Path( sys.argv[2] ).resolve(), # Work dir is intentionally the same as install dir
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

def fetch_and_patch():
    nc.create_workdir()

    # 0.32
    nc.shallow_checkout( nc.work_dir, "https://github.com/agl/jbig2enc.git", "309b2d55c7dfdcf0ab6afccb6d88834afc0bf2c0" )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.work_dir_looks_ok():
    fetch_and_patch()
