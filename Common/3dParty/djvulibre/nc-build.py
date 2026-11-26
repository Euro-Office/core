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

# Source only: core compiles libdjvu into DjVuFile and drawingfile
# (DJVULIBRE_SOURCES in common.cmake). DjVuLibre builds with autotools only.
nc.init_for_dep(
    depname = "djvulibre",
    workdir = Path( sys.argv[2] ).resolve(), # Work dir is intentionally the same as install dir
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "2"
)

patches_dir = script_dir / "patch"

def fetch_and_patch():
    nc.create_workdir()

    # 3.5.30.1 plus the decoder hardening that came after it (MMR runs,
    # INCL nesting, thread creation, overflow checks), 2026-11-04
    nc.shallow_checkout( nc.work_dir, "https://git.code.sf.net/p/djvu/djvulibre-git", "4f2359aa9533f6f125102cec874aaf6e718585f0" )

    # The WebAssembly build has no threads
    nc.run_command(
        [ "git", "apply", f"{ patches_dir / 'djvulibre-wasm-nothreads.patch' }" ],
        "Apply djvulibre patch",
        nc.work_dir
    )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.work_dir_looks_ok():
    fetch_and_patch()
