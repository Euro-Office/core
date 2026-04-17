#!/usr/bin/env python3

import sys
import shutil
import subprocess
import os
from pathlib import Path

work_dir = Path( sys.argv[1] )
install_dir = Path( sys.argv[2] )
force_redo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent
icu_major = "74"
icu_minor = "2"
debug_mode = True
dep_name = "ICU"

def abort_op( message : str, keep_work : bool = False ):
    print( f"Aboring {dep_name}: {message}", file = sys.stderr )
    if not debug_mode:
        try:
            if not keep_work:
                shutil.rmtree( work_dir )
            shutil.rmtree( install_dir )
        except FileNotFoundError:
            pass
    sys.exit( 1 )

def work_dir_looks_ok() -> bool:
    return Path( work_dir / "ok_marker" ).exists()

def install_dir_looks_ok() -> bool:
    return Path( install_dir / "ok_marker" ).exists()

def fetch_and_patch():
    # If exists and needed, remove work dir
    if work_dir.exists() and ( force_redo or not work_dir_looks_ok() ):
        try:
            shutil.rmtree( work_dir )
        except FileNotFoundError:
            pass

    # Create work dir (if needed)
    if not work_dir.exists():
        try:
            work_dir.mkdir( parents = True )
        except OSError:
            abort_op( "Failed to create work dir" )

    # Check out the git repo
    git_cmd = [
        "git",
        "-c", "core.autocrlf=false", "-c", "core.eol=lf",
        "clone",
        "--depth", "1",
        "--branch", f"release-{icu_major}-{icu_minor}",
        "https://github.com/unicode-org/icu.git",
        str(work_dir / "icu2"),
    ]

    try:
        _ = subprocess.run( git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True )
    except subprocess.CalledProcessError as e:
        abort_op( f"git clone failed: {e.stderr.strip() or e.stdout.strip() or e}" )

    try:
        shutil.copytree(
            work_dir / "icu2" / "icu4c",
            work_dir / "icu",
            copy_function=shutil.copy2
        )
    except Exception as e:
        abort_op( f"Copy failed: {e}" )

    try:
        shutil.copy2( work_dir / "icu2" / "LICENSE", work_dir / "LICENSE" )
    except Exception as e:
        abort_op( f"License copy failed: {e}" )

    # Create ok marker
    Path( work_dir / "ok_marker" ).touch()

    print( "Fetch & patch completed" )

def build_and_install():
    # If exists and needed, remove install dir
    if install_dir.exists() and ( force_redo or not install_dir_looks_ok() ):
        try:
            shutil.rmtree( install_dir )
        except FileNotFoundError:
            pass

    # Create install dir (if needed)
    if not install_dir.exists():
        try:
            install_dir.mkdir( parents = True )
        except OSError:
            abort_op( "Failed to create install dir" )


    if sys.platform.startswith("linux"):
        # Configure
        configure_cmd = [
            "./configure",
            f"--prefix={install_dir}",
            "--enable-rpath",
            "CC=gcc",
            "CXX=g++",
            "AR=ar",
            "RANLIB=ranlib",
            "CXXFLAGS=-static-libstdc++ -static-libgcc -std=c++11",
            "LDFLAGS=-Wl,-rpath,$ORIGIN"
        ]
        try:
            _ = subprocess.run( configure_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd = work_dir / "icu" / "source" )
        except subprocess.CalledProcessError as e:
            abort_op( f"Configure failed: {e.stderr.strip() or e.stdout.strip() or e}" )

        # Build
        build_cmd = [ "make", f"-j{os.cpu_count()}" ]
        try:
            _ = subprocess.run( build_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd = work_dir / "icu" / "source" )
        except subprocess.CalledProcessError as e:
            abort_op( f"Build failed: {e.stderr.strip() or e.stdout.strip() or e}" )

        # Install
        install_cmd = [ "make", "install" ]
        try:
            _ = subprocess.run( install_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd = work_dir / "icu" / "source" )
        except subprocess.CalledProcessError as e:
            abort_op( f"Install failed: {e.stderr.strip() or e.stdout.strip() or e}" )

    elif sys.platform == "win32":
        icu_source_dir = work_dir / "icu" / "source"
        bat_path = script_dir / "nc-build.bat"
        build_cmd = [
            "cmd.exe",
            "/c",
            "call",
            str(bat_path),
            "", "",
            str(icu_source_dir),
            str(install_dir)
        ]

        try:
            _ = subprocess.run( build_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd = work_dir / "icu" / "source" )
        except subprocess.CalledProcessError as e:
            abort_op( f"Windows build failed: {e.stderr.strip() or e.stdout.strip() or e}" )

    else:
        abort_op( f"Unkown target platform: {sys.platform}" )

    # Create ok marker
    Path( install_dir / "ok_marker" ).touch()

    sys.stdout.reconfigure(encoding='utf-8')
    if sys.platform == 'win32':
        os.system('chcp 65001 >nul')
    print( "Build and install completed" )

if not work_dir_looks_ok():
    fetch_and_patch()

if not install_dir_looks_ok():
    build_and_install()
