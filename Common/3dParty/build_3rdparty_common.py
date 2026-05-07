import sys
import shutil
import stat
import subprocess
import os
import time
from pathlib import Path

dep_name = None
debug_mode = False
work_dir = None
install_dir = None
force_redo = False

def abort_op( message : str, keep_work : bool = False, error_is_fatal : bool = True ):
    print( f"Aboring {dep_name}: {message}", file = sys.stderr )
    if error_is_fatal:
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
    return ( not force_redo ) and Path( work_dir / "ok_marker" ).exists()

def install_dir_looks_ok() -> bool:
    return ( not force_redo ) and Path( install_dir / "ok_marker" ).exists()

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

def ensure_directory_exists( dir : Path ):
    if not dir.exists():
        dir.mkdir( parents = True )

def run_command(
        cmd : list[str],
        description : str, cwd : Path | None = None,
        verbose : bool = False,
        error_is_fatal : bool = True,
        env : dict[ str, str ] | None = None
    ):

    cwd = (cwd or Path.cwd()).resolve()
    output_pipe = None if verbose else subprocess.PIPE
    final_env = os.environ.copy() | ( {} if env is None else env )

    try:
        _ = subprocess.run( cmd, check=True, stdout=output_pipe, stderr=output_pipe, text=True, cwd=cwd, env = final_env )
    except subprocess.CalledProcessError as e:
        if verbose:
            abort_op( f"{description} failed", error_is_fatal=error_is_fatal )
        else:
            abort_op( f"{description} failed: {e.stderr.strip() or e.stdout.strip() or e}", error_is_fatal=error_is_fatal )

def capture_process_output( cmd : list[str] ) -> str:
    result = subprocess.run( [ "clang", "--version" ], capture_output = True, text = True, check = True )
    return result.stdout

def fix_terminal_encoding():
    sys.stdout.reconfigure( encoding='utf-8' )
    if is_windows():
        os.system( 'chcp 65001 >nul' )

def shallow_checkout( repo_dir : Path, repo_url : str, commit : str ):
    if repo_dir.exists():
        def onerror(func, path, exc_info):
            os.chmod(path, stat.S_IWRITE)
            func(path)
        try:
            shutil.rmtree( repo_dir, onexc=onerror )
        except FileNotFoundError:
            pass

    # Create work dir (if needed)
    if not repo_dir.exists():
        try:
            repo_dir.mkdir( parents = True )
        except OSError:
            abort_op( "Failed to create repo dir" )

    run_command(
        [ "git", "init" ],
        f"Git init ({repo_dir.name})",
        repo_dir
    )

    run_command(
        [ "git", "remote", "add", "origin", repo_url ],
        f"Add origin ({repo_dir.name})",
        repo_dir
    )

    run_command(
        [ "git", "fetch", "--depth", "1", "origin", commit ],
        f"Fetch commit ({repo_dir.name})",
        repo_dir
    )

    run_command(
        [ "git", "checkout", "FETCH_HEAD" ],
        f"Checkout head ({repo_dir.name})",
        repo_dir
    )

class MeasurementObj:
    def __init__( self, name: str ):
        self.name = name
        self.start = time.perf_counter()

    def report( self ):
        end = time.perf_counter()
        elapsed = end - self.start
        print( f"{self.name}: {elapsed:.6f} second(s)" )
        
    def elapsed_string( self ) -> float:
        end = time.perf_counter()
        elapsed = end - self.start
        return f"{elapsed:.6f} second(s)"
