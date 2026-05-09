#!/usr/bin/env python3

import sys
import shutil
import os
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.work_dir = Path( sys.argv[1] )
nc.install_dir = Path( sys.argv[2] )
nc.force_redo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
nc.dep_name = "Boost"
nc.debug_mode = True

modules_needed = [ "headers", "system", "filesystem", "regex", "date_time" ]
header_only_modules_needed = [ "any", "asio", "beast", "format" ]

def fetch_and_patch():

    nc.create_workdir()
    nc.run_command(
        [ "git", "clone", "https://github.com/boostorg/boost.git", "-b", "boost-1.72.0", nc.work_dir, "--depth", "1" ],
        "Clone Boost 1.72.0"
    )

    nc.run_command(
        [ "git", "submodule", "update", "--depth", "1", "-q", "--init", Path( "tools" ) / "boostdep" ],
        "Get boostdep",
        nc.work_dir
    )

    for module in ( modules_needed + header_only_modules_needed ):
        nc.run_command(
            [ "git", "submodule", "update", "--depth", "1", "-q", "--init", Path( "libs" ) / module ],
            f"Init { module }",
            nc.work_dir
        )

        nc.run_command(
            [ "python", Path( "tools" ) / "boostdep" / "depinst" / "depinst.py", "-X", "test", "-g", "--depth 1", module ],
            f"Get dependencies for {module}",
            nc.work_dir
        )

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )

def build_and_install():
    nc.create_install_dir()
    
    if nc.is_linux():
        nc.run_command(
            [ "./bootstrap.sh", f"--prefix={ nc.install_dir }" ],
            "Running bootstrap",
            nc.work_dir
        )
    elif nc.is_windows():
        nc.run_command(
            [ "bootstrap.bat", f"--prefix={ nc.install_dir }" ],
            "Running bootstrap",
            nc.work_dir
        )
    else:
        abort_op( f"Unkown target platform: {sys.platform}" )

    build_cmd = [ "b2.exe" if nc.is_windows() else "./b2" ]
    for module in modules_needed:
        build_cmd.append( f"--with-{ module }" )
    build_cmd.append( "variant=release" )
    build_cmd.append( "link=static" )
    if not nc.is_windows():
        build_cmd.append( "cflags=-fPIC" )
        build_cmd.append( "cxxflags=-fPIC" )
    build_cmd.append( f"--prefix={ nc.install_dir }" )
    build_cmd.append( "install" )

    nc.run_command(
        build_cmd,
        "Build and install boost libs",
        nc.work_dir
    )

    nc.create_install_dir_ok_marker()
    
    print( "Build and install completed" )

if not nc.work_dir_looks_ok():
    fetch_and_patch()

if not nc.install_dir_looks_ok():
    if nc.is_windows() and shutil.which("nmake") is None:
        raise RuntimeError(
            "MSVC environment is not set up: 'nmake' not found in PATH.\n"
            "Run 'vcvarsx86_amd64.bat' or use 'x64 Native Tools Command Prompt'."
        )
    build_and_install()
