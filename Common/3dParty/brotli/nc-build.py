#!/usr/bin/env python3

import sys
import shutil
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.work_dir = Path( sys.argv[1] )
nc.install_dir = Path( sys.argv[2] )
nc.work_dir = nc.install_dir
nc.force_redo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
nc.dep_name = "Brotli"
nc.debug_mode = True

def fetch_and_patch():
    nc.create_install_dir()

    nc.run_command(
        [ "git", "init" ],
        "Git init",
        nc.install_dir
    )

    nc.run_command(
        [ "git", "remote", "add", "origin", "https://github.com/google/brotli.git" ],
        "Add origin",
        nc.install_dir
    )

    nc.run_command(
        [ "git", "fetch", "--depth", "1", "origin", "a47d7475063eb223c87632eed806c0070e70da29" ],
        "Fetch commit",
        nc.install_dir
    )

    nc.run_command(
        [ "git", "checkout", "FETCH_HEAD" ],
        "Checkout head",
        nc.install_dir
    )

    nc.create_install_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.install_dir_looks_ok():
    fetch_and_patch()
