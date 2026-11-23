#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

import sys
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent
patches_dir = script_dir / "patch"

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

# Sources only: core compiles FreeType itself (DesktopEditor/graphics,
# PdfFile, the WASM modules), each with its own defines.
nc.init_for_dep(
    depname = "FreeType",
    workdir = Path( sys.argv[2] ).resolve(), # Work dir is intentionally the same as install dir
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

def fetch_and_patch():
    nc.create_workdir()

    # VER-2-15-0
    nc.shallow_checkout( nc.work_dir, "https://github.com/freetype/freetype.git", "37515cf56ba3349656049edbed89ffc6f6466f2d" )

    # setjmp/longjmp -> C++ exceptions for the WASM builds (BUILDING_WASM_MODULE)
    nc.run_command(
        [ "git", "apply", f"{ patches_dir / 'freetype.patch' }" ],
        "Apply FreeType patch",
        nc.work_dir
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.work_dir_looks_ok():
    fetch_and_patch()
