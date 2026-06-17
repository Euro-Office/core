#!/usr/bin/env python3

import sys
import shutil
import os
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent
icu_major = "74"
icu_minor = "2"

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "ICU",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
)

def fetch_and_patch():
    nc.create_workdir()
    
    nc.run_command(
        [ "git", "-c", "core.autocrlf=false", "-c", "core.eol=lf", "clone",
          "--depth", "1", "--branch", f"release-{icu_major}-{icu_minor}",
          "https://github.com/unicode-org/icu.git", str(nc.work_dir / "icu2")
        ],
        "Checkout git repo",
    )

    try:
        shutil.copytree(
            nc.work_dir / "icu2" / "icu4c",
            nc.work_dir / "icu",
            copy_function=shutil.copy2
        )
    except Exception as e:
        nc.abort_op( f"Copy failed: {e}" )

    try:
        shutil.copy2( nc.work_dir / "icu2" / "LICENSE", nc.work_dir / "LICENSE" )
    except Exception as e:
        nc.abort_op( f"License copy failed: {e}" )

    try:
        shutil.rmtree( nc.work_dir / "icu2" )
    except FileNotFoundError:
        pass

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

def build_and_install():
    nc.create_install_dir()

    if nc.is_linux():
        nc.run_command(
            [   "./configure",
                f"--prefix={nc.install_dir}",
                "--enable-rpath",
                "CC=gcc",
                "CXX=g++",
                "AR=ar",
                "RANLIB=ranlib",
                "CXXFLAGS=-static-libstdc++ -static-libgcc -std=c++11",
                "LDFLAGS=-Wl,-rpath,$ORIGIN"
            ],
            "Configure",
            nc.work_dir / "icu" / "source"
        )

        nc.run_command(
            [ "make", f"-j{os.cpu_count()}" ],
            "Build",
            nc.work_dir / "icu" / "source"
        )

        nc.run_command(
            [ "make", "install" ],
            "Install",
            nc.work_dir / "icu" / "source"
        )

    elif nc.is_windows():
        icu_source_dir = nc.work_dir / "icu" / "source"
        bat_path = script_dir / "nc-build.bat"
        nc.run_command(
            [   "cmd.exe",
                "/c",
                "call",
                str(bat_path),
                "", "",
                str(icu_source_dir),
                str(nc.install_dir)
            ],
            "Cygwin build",
            nc.work_dir / "icu" / "source"
        )

    else:
        nc.abort_op( f"Unkown target platform: {sys.platform}" )

    nc.create_install_dir_ok_marker()
    nc.fix_terminal_encoding()
    print( "Build and install completed" )

def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        build_and_install()

nc.ensure_dep( build_all )