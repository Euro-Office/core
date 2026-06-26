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

nc.init_for_dep(
    depname = "OpenSSL",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
)

def fetch_and_patch():
    nc.create_workdir()

    nc.run_command(
        [   "git", "-c", "core.autocrlf=false", "-c", "core.eol=lf",
            "clone", "--depth", "1",
            "--branch", "OpenSSL_1_1_1f",
            "https://github.com/openssl/openssl.git",
            str(nc.work_dir)
        ],
        "Clone repo"
    )
    
    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

def build_and_install():
    nc.create_install_dir()
    
    if nc.is_linux():
        nc.run_command(
            [   "./config",
                f"--prefix={nc.install_dir}",
                f"--openssldir={nc.install_dir}",
                "enable-md2",
                "no-shared",
                "no-asm",
            ],
            "Configure",
            nc.work_dir
        )

        nc.run_command(
            [ "make", f"-j{os.cpu_count()}" ],
            "Build",
            nc.work_dir
        )

        nc.run_command(
            [ "make", "install" ],
            "Install",
            nc.work_dir
        )

    elif nc.is_windows():
        import platform
        # OpenSSL's Configure target must match the MSVC environment's TARGET arch
        # (not the host). vcvarsall.bat / the Native Tools prompt export this:
        tgt = os.environ.get("VSCMD_ARG_TGT_ARCH", "").lower()
        if not tgt:  # fallback for a native prompt that didn't set it
            tgt = {"AMD64": "x64", "ARM64": "arm64", "X86": "x86"}.get(
                platform.machine().upper(), "x64")
        ossl_target = {
            "x64":   "VC-WIN64A",
            "arm64": "VC-WIN64-ARM",
            "x86":   "VC-WIN32",
        }.get(tgt)
        if ossl_target is None:
            nc.abort_op(f"Unsupported Windows target arch: {tgt!r}")
        nc.run_command(
            [   shutil.which("perl"),
                "Configure",
                ossl_target,
                f"--prefix={nc.install_dir}",
                f"--openssldir={nc.install_dir}",
                "enable-md2",
                "no-shared",
                "no-asm",
            ],
            "Configure",
            nc.work_dir,
            verbose=True 
        )

        nc.run_command(
            [ shutil.which("perl"), "configdata.pm", "--dump" ],
            "Dump config", 
            nc.work_dir, 
            verbose=True 
        )

        nc.run_command(
            [ "nmake" ],
            "Build",
            nc.work_dir,
            verbose=True 
        )

        nc.run_command(
            [ "nmake", "install" ],
            "Install",
            nc.work_dir,
            verbose=True 
        )

    else:
        nc.abort_op( f"Unkown target platform: {sys.platform}" )

    nc.create_install_dir_ok_marker()
    
    print( "Build and install completed" )

def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        if nc.is_windows() and shutil.which("nmake") is None:
            raise RuntimeError(
                "MSVC environment is not set up: 'nmake' not found in PATH.\n"
                "Run 'vcvarsx86_amd64.bat' or use 'x64 Native Tools Command Prompt'."
            )
        build_and_install()
    
nc.ensure_dep( build_all )
