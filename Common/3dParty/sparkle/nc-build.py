#!/usr/bin/env python3
# Sparkle (https://sparkle-project.org), the macOS desktop app's updater.
# Fetches the pinned prebuilt release; desktop-apps links SPARKLE_ROOT/Sparkle.framework
# and release CI signs updates with SPARKLE_ROOT/bin/sign_update.
import sys
import shutil
import tarfile
from pathlib import Path

script_dir = Path( sys.argv[0] ).resolve().parent
third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )

import build_3rdparty_common as nc

sparkle_version = "2.10.1"
sparkle_sha256 = "1a856d2a9edb8e628de88121d7be04cf0e2167f8641650b78fca00b14244cf27"
sparkle_url = f"https://github.com/sparkle-project/Sparkle/releases/download/{sparkle_version}/Sparkle-{sparkle_version}.tar.xz"

nc.init_for_dep(
    depname = "sparkle",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len( sys.argv ) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

if sys.platform != "darwin":
    print( "Sparkle is only used on macOS, skipping" )
    sys.exit( 0 )

extract_dir = nc.work_dir / "extracted"


def fetch():
    nc.create_workdir()
    archive = nc.work_dir / f"Sparkle-{sparkle_version}.tar.xz"
    print( f"Downloading Sparkle {sparkle_version}" )
    nc.download_verified( sparkle_url, archive, sparkle_sha256 )

    if extract_dir.exists():
        shutil.rmtree( extract_dir )
    with tarfile.open( archive, "r:xz" ) as tar:
        try:
            tar.extractall( extract_dir, filter = "data" )  # Python 3.12+
        except TypeError:
            tar.extractall( extract_dir )

    nc.create_work_dir_ok_marker()


def install():
    nc.create_install_dir()
    # symlinks = True keeps the framework's Versions/Current layout intact for codesign
    for name in ( "Sparkle.framework", "bin" ):
        shutil.copytree( extract_dir / name, nc.install_dir / name, symlinks = True, dirs_exist_ok = True )
    shutil.copy2( extract_dir / "LICENSE", nc.install_dir / "LICENSE" )
    nc.create_install_dir_ok_marker()


def build_all():
    if not nc.work_dir_looks_ok():
        fetch()
    install()


nc.ensure_dep( build_all )
