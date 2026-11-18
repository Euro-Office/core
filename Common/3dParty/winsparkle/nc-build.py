#!/usr/bin/env python3
# WinSparkle (https://winsparkle.org), the Windows desktop app's updater.
# Fetches the pinned prebuilt release and installs include/, lib/WinSparkle.lib
# and bin/WinSparkle.dll for the target arch.
import sys
import shutil
import zipfile
from pathlib import Path

script_dir = Path( sys.argv[0] ).resolve().parent
third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )

import build_3rdparty_common as nc

winsparkle_version = "0.9.4"
winsparkle_sha256 = "6037df37fc263bd1650a1c4949681a9d40ffe991d01f35892a406cb5d103c976"
winsparkle_url = f"https://github.com/vslavik/winsparkle/releases/download/v{winsparkle_version}/WinSparkle-{winsparkle_version}.zip"

nc.init_for_dep(
    depname = "winsparkle",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len( sys.argv ) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

if not nc.is_windows():
    print( "WinSparkle is only used on Windows, skipping" )
    sys.exit( 0 )

extract_dir = nc.work_dir / "extracted"
release_dir = extract_dir / f"WinSparkle-{winsparkle_version}"


def fetch():
    nc.create_workdir()
    archive = nc.work_dir / f"WinSparkle-{winsparkle_version}.zip"
    print( f"Downloading WinSparkle {winsparkle_version}" )
    nc.download_verified( winsparkle_url, archive, winsparkle_sha256 )

    if extract_dir.exists():
        shutil.rmtree( extract_dir )
    with zipfile.ZipFile( archive ) as zf:
        zf.extractall( extract_dir )

    nc.create_work_dir_ok_marker()


def install():
    nc.create_install_dir()
    arch_dir = release_dir / { "x64": "x64", "arm64": "ARM64", "x86": "Win32" }[ nc.target_arch() ] / "Release"

    shutil.copytree( release_dir / "include", nc.install_dir / "include", dirs_exist_ok = True )
    for sub, files in ( ( "lib", ( "WinSparkle.lib", ) ), ( "bin", ( "WinSparkle.dll", "WinSparkle.pdb" ) ) ):
        nc.ensure_directory_exists( nc.install_dir / sub )
        for f in files:
            shutil.copy2( arch_dir / f, nc.install_dir / sub / f )
    # Signing tool for release CI
    shutil.copy2( release_dir / "bin" / "winsparkle-tool.exe", nc.install_dir / "bin" / "winsparkle-tool.exe" )
    for f in ( "COPYING", "COPYING.expat" ):
        shutil.copy2( release_dir / f, nc.install_dir / f )
    nc.create_install_dir_ok_marker()


def build_all():
    if not nc.work_dir_looks_ok():
        fetch()
    install()


nc.ensure_dep( build_all )
