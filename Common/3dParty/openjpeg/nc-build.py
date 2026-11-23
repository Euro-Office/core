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

# Sources only: core compiles openjp2 itself (PdfFile, the drawingfile WASM
# module), with the opj_config*.h from DesktopEditor/raster/Jp2/openjpeg.
nc.init_for_dep(
    depname = "OpenJPEG",
    workdir = Path( sys.argv[2] ).resolve(), # Work dir is intentionally the same as install dir
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

def fetch():
    nc.create_workdir()

    # v2.5.4
    nc.shallow_checkout( nc.work_dir, "https://github.com/uclouvain/openjpeg.git", "6c4a29b00211eb0430fa0e5e890f1ce5c80f409f" )

    nc.create_work_dir_ok_marker()

    print( "Fetch completed" )

if not nc.work_dir_looks_ok():
    fetch()
