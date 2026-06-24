#!/usr/bin/env python3
import sys
import shutil
import time
import json
import tarfile
import urllib.request
import urllib.parse
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

# --- Prebuilt CEF (Spotify automated builds) configuration --------------------
# We no longer build CEF from source; we download a prebuilt binary distribution
# from https://cef-builds.spotifycdn.com/ instead.
#
#   cef_index_url      : JSON manifest of every available build / platform.
#   cef_download_base  : where the .tar.bz2 archives live.
#   cef_channel        : "stable" or "beta".
#   cef_distrib_type   : "minimal" (release binaries + headers + libcef_dll_wrapper,
#                        no debug build / sample apps), "standard" (adds debug +
#                        cefclient/cefsimple), "client", etc.
#   cef_version        : pin an EXACT build for reproducibility, e.g.
#                        "114.4.1+g7c3f9a0+chromium-114.0.5735.134"
#                        Leave as None to auto-pick the newest build for the
#                        Chromium major below.
#   cef_chromium_major : only used when cef_version is None. The old source build
#                        tracked CEF branch 5414, i.e. the Chromium 114.x line.
cef_index_url = "https://cef-builds.spotifycdn.com/index.json"
cef_download_base = "https://cef-builds.spotifycdn.com/"
cef_channel = "stable"
cef_distrib_type = "minimal"
cef_version = "109.1.18+gf1c41e4+chromium-109.0.5414.120"
cef_chromium_major = None

max_download_retries = 5

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

download_dir = nc.work_dir / "download"


def cef_platform():
    if sys.platform.startswith( "win" ):
        return "windows64"
    if sys.platform.startswith( "linux" ):
        return "linux64"
    nc.abort_op( f"Unsupported platform for prebuilt CEF: {sys.platform}" )


def _version_key( entry ):
    parts = []
    for p in entry.get( "chromium_version", "0" ).split( "." ):
        parts.append( int( p ) if p.isdigit() else 0 )
    return parts


def resolve_build():
    """Pick a (cef_version, archive_filename) from the Spotify build index."""
    platform = cef_platform()
    print( f"Fetching CEF build index for {platform} ..." )

    req = urllib.request.Request( cef_index_url, headers = { "User-Agent": "cef-fetch" } )
    with urllib.request.urlopen( req ) as resp:
        index = json.load( resp )

    versions = index.get( platform, {} ).get( "versions", [] )
    if not versions:
        nc.abort_op( f"No CEF builds listed for platform '{platform}'" )

    if cef_version:
        chosen = next( ( v for v in versions if v.get( "cef_version" ) == cef_version ), None )
        if chosen is None:
            nc.abort_op( f"Pinned CEF version '{cef_version}' not found for '{platform}'" )
    else:
        candidates = [
            v for v in versions
            if v.get( "channel", "stable" ) == cef_channel
            and v.get( "chromium_version", "" ).split( "." )[0] == cef_chromium_major
        ]
        if not candidates:
            print( f"!!! No '{cef_channel}' build for Chromium {cef_chromium_major}; "
                   f"falling back to newest '{cef_channel}' build." )
            candidates = [ v for v in versions if v.get( "channel", "stable" ) == cef_channel ]
        if not candidates:
            nc.abort_op( f"No '{cef_channel}' CEF builds available for '{platform}'" )
        chosen = max( candidates, key = _version_key )

    files = chosen.get( "files", [] )
    match = next( ( f for f in files if f.get( "type" ) == cef_distrib_type ), None )
    if match is None:
        available = ", ".join( sorted( { f.get( "type", "?" ) for f in files } ) )
        nc.abort_op( f"No '{cef_distrib_type}' distrib for {chosen.get('cef_version')}. "
                     f"Available types: {available}" )

    return chosen.get( "cef_version" ), match["name"]


def download( url, dest ):
    for attempt in range( 1, max_download_retries + 1 ):
        try:
            req = urllib.request.Request( url, headers = { "User-Agent": "cef-fetch" } )
            with urllib.request.urlopen( req ) as resp, open( dest, "wb" ) as out:
                shutil.copyfileobj( resp, out )
            return
        except Exception as e:
            if attempt == max_download_retries:
                nc.abort_op( f"Download failed after {max_download_retries} attempts: {e}" )
            print( f"Download failed (attempt {attempt}/{max_download_retries}): {e}. "
                   "Retrying in 10 seconds..." )
            time.sleep( 10 )


def fetch_and_patch():
    nc.create_workdir()
    download_dir.mkdir( parents = True, exist_ok = True )

    version, file_name = resolve_build()
    url = cef_download_base + urllib.parse.quote( file_name )
    archive = download_dir / file_name

    print( f"Downloading prebuilt CEF {version} ({cef_distrib_type})" )
    print( f"  {url}" )
    download( url, archive )

    print( "Extracting binary distribution ..." )
    with tarfile.open( archive, "r:bz2" ) as tar:
        try:
            tar.extractall( download_dir, filter = "data" )  # Python 3.12+
        except TypeError:
            tar.extractall( download_dir )

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def find_binary_distrib_dir():
    matches = sorted( download_dir.glob( f"cef_binary_*_{cef_platform()}*" ) )
    if not matches:
        nc.abort_op( f"No extracted cef_binary_* directory found in {download_dir}" )
    return matches[0]


def build_and_install():
    nc.create_install_dir()

    cef_binary_dir = find_binary_distrib_dir()

    # Copy the binary distribution into the install dir
    target_dir = nc.install_dir
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