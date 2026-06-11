#!/usr/bin/env python3
import sys
import shutil
import os
import time
import subprocess
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

cef_branch = "5414"
cef_checkout = "f1c41e4b1392ef8816171c4d51a2fca308db5501"
automate_git_url = "https://raw.githubusercontent.com/chromiumembedded/cef/master/tools/automate/automate-git.py"

gn_defines = ( "is_official_build=true use_sysroot=true symbol_level=0 "
               "is_cfi=false use_thin_lto=true use_vaapi=false "
               "use_gtk=true use_dbus=true use_partition_alloc_as_malloc=false" )

max_gclient_retries = 20
max_recovery_tries = 3

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )

import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "cef",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
)

automate_dir = nc.work_dir / "automate"
download_dir = nc.work_dir / "chromium_git"


def fetch_and_patch():
    nc.create_workdir()

    automate_dir.mkdir( parents = True, exist_ok = True )
    download_dir.mkdir( parents = True, exist_ok = True )

    nc.run_command(
        [ "wget", automate_git_url, "-O", str( automate_dir / "automate-git.py" ) ],
        "Download automate-git.py",
        automate_dir
    )

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def setup_build_env():
    os.environ["LANG"] = "en_US.UTF-8"
    os.environ["LC_ALL"] = "en_US.UTF-8"
    os.environ["GN_DEFINES"] = gn_defines
    os.environ["CEF_ARCHIVE_FORMAT"] = "tar.bz2"


def run_gclient_recovery():
    print( "!!! automate-git.py failed. Attempting to recover via gclient sync..." )

    # automate-git.py downloads depot_tools inside the download dir.
    # Add it to PATH so we can use 'gclient'.
    depot_tools_dir = download_dir / "depot_tools"
    os.environ["PATH"] = os.environ["PATH"] + os.pathsep + str( depot_tools_dir )

    chromium_dir = download_dir / "chromium"

    retry_count = 0
    while True:
        result = subprocess.run(
            [ "gclient", "sync", "--force", "--reset",
              "--with_branch_heads", "--with_tags" ],
            cwd = chromium_dir
        )
        if result.returncode == 0:
            break
        retry_count += 1
        if retry_count > max_gclient_retries:
            nc.abort_op( f"Failed to sync after {max_gclient_retries} attempts" )
        print( f"gclient sync failed (Attempt {retry_count}/{max_gclient_retries}). "
               "Retrying in 10 seconds..." )
        time.sleep( 10 )

    print( "!!! gclient sync succeeded. Resuming build..." )

    shutil.rmtree( download_dir / "cef", ignore_errors = True )
    shutil.rmtree( depot_tools_dir, ignore_errors = True )


def run_automate_git():
    # Not using nc.run_command here because failure is expected and
    # handled by the recovery loop instead of aborting.
    result = subprocess.run(
        [   sys.executable,
            str( automate_dir / "automate-git.py" ),
            f"--download-dir={download_dir}",
            f"--checkout={cef_checkout}",
            f"--branch={cef_branch}",
            "--minimal-distrib",
            "--client-distrib",
            "--force-clean",
            "--no-chromium-history",
            "--build-target=cefsimple",
            "--x64-build",
            "--with-pgo-profiles"
        ],
        cwd = automate_dir
    )
    return result.returncode == 0


def find_binary_distrib_dir():
    binary_distrib = download_dir / "chromium" / "src" / "cef" / "binary_distrib"
    matches = sorted( binary_distrib.glob( "cef_binary_*_linux64" ) )
    if not matches:
        nc.abort_op( f"No cef_binary_*_linux64 found in {binary_distrib}" )
    return matches[0]

def build_and_install():
    nc.create_install_dir()

    setup_build_env()

    # Main build loop: if automate-git.py fails, run gclient recovery and
    # retry. After max_recovery_tries failed recoveries, wipe the chromium
    # checkout entirely and start over.
    recovery_count = 0
    while True:
        if run_automate_git():
            print( "Build completed successfully!" )
            break
        run_gclient_recovery()
        recovery_count += 1
        if recovery_count > max_recovery_tries:
            print( "!!! Too many failed recoveries. Wiping chromium checkout..." )
            shutil.rmtree( download_dir / "chromium", ignore_errors = True )
            recovery_count = 0

    cef_binary_dir = find_binary_distrib_dir()

    # Copy the binary distribution into the install dir
    target_dir = nc.install_dir / f"cef"
    try:
        if target_dir.exists():
            shutil.rmtree( target_dir )
        shutil.copytree( cef_binary_dir, target_dir, copy_function = shutil.copy2 )
    except Exception as e:
        nc.abort_op( f"Copy of binary distrib failed: {e}" )

    nc.create_install_dir_ok_marker()
    nc.fix_terminal_encoding()
    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()
    if not nc.install_dir_looks_ok():
        build_and_install()


nc.ensure_dep( build_all )