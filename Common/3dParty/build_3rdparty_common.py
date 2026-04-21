import sys
import shutil
import subprocess
import os
from pathlib import Path

dep_name = None
debug_mode = False
work_dir = None
install_dir = None
force_redo = False

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

def is_linux() -> bool:
    return sys.platform.startswith("linux")

def is_windows() -> bool:
    return sys.platform == "win32"

def work_dir_looks_ok() -> bool:
    return Path( work_dir / "ok_marker" ).exists()

def install_dir_looks_ok() -> bool:
    return Path( install_dir / "ok_marker" ).exists()

def create_work_dir_ok_marker():
    Path( work_dir / "ok_marker" ).touch()

def create_install_dir_ok_marker():
    Path( install_dir / "ok_marker" ).touch()

def create_workdir():
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

def create_install_dir():
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

def run_command( cmd : list[str], description : str, cwd : Path | None = None, verbose : bool = False ):
    cwd = (cwd or Path.cwd()).resolve()
    output_pipe = None if verbose else subprocess.PIPE

    try:
        _ = subprocess.run( cmd, check=True, stdout=output_pipe, stderr=output_pipe, text=True, cwd=cwd )
    except subprocess.CalledProcessError as e:
        if verbose:
            abort_op( f"{description} failed" )
        else:
            abort_op( f"{description} failed: {e.stderr.strip() or e.stdout.strip() or e}" )

def fix_terminal_encoding():
    sys.stdout.reconfigure( encoding='utf-8' )
    if is_windows():
        os.system( 'chcp 65001 >nul' )
