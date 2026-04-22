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
nc.dep_name = "MD"
nc.debug_mode = True

def fetch_and_patch():
    nc.create_install_dir()

    nc.run_command(
        [ "git", "clone", "https://github.com/mity/md4c.git", nc.install_dir ],
        "Git clone",
    )

    nc.run_command(
        [ "git", "checkout", "481fbfbdf72daab2912380d62bb5f2187d438408" ],
        "Checkout commit",
        nc.install_dir
    )

    nc.create_install_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.install_dir_looks_ok():
    fetch_and_patch()
