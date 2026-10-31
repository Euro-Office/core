import sys
import re
import http.client
import shutil
import stat
import subprocess
import os
import time
import platform
import tarfile
import tempfile
from pathlib import Path

_original_rmtree = shutil.rmtree

def _force_remove(func, path, exc):
    try:
        os.chmod(path, stat.S_IWRITE)
    except OSError:
        pass
    for _ in range(5):
        try:
            func(path)
            return
        except PermissionError:
            time.sleep(0.5)
    func(path)  # final attempt; raises if genuinely locked

def _safe_rmtree(path, *args, **kwargs):
    if sys.version_info >= (3, 12):
        kwargs.setdefault("onexc", _force_remove)
    else:
        kwargs.setdefault("onerror", _force_remove)
    return _original_rmtree(path, *args, **kwargs)

shutil.rmtree = _safe_rmtree
# --- end read-only-safe rmtree ---

dep_name = None
debug_mode = False
work_dir = None
install_dir = None
force_redo = False
dep_version = ""

def init_for_dep(
    depname : str,
    workdir : Path,
    installdir : Path,
    forceredo : bool,
    version : str = "", # Arbitrary version string. Integer numbers suggested.
                        # If a deps version/branch/tag/patch changes, bump this in the respective build script.
                        # This will force that dep to be rebuilt.
    debugmode : bool = True
):
    global dep_name, work_dir, install_dir, force_redo, debug_mode, dep_version, log_cleared

    dep_name = depname
    work_dir = workdir
    install_dir = installdir
    force_redo = forceredo
    dep_version = version
    debug_mode = debugmode
    log_cleared = False
    
def abort_op( message : str, keep_work : bool = False, error_is_fatal : bool = True ):
    print( f"Aborting {dep_name}: {message}", file = sys.stderr )
    if error_is_fatal:
        if not debug_mode:
            try:
                if not keep_work:
                    shutil.rmtree( work_dir )
                shutil.rmtree( install_dir )
            except FileNotFoundError:
                pass
        sys.exit( 1 )

# Cross-compilation target, set by build_3rdparty.py --target=<os>-<arch>
# (e.g. "android-arm64"). Empty means: build for the host.
CROSS_TARGET = os.environ.get( "EO_3RDPARTY_TARGET", "" ).strip().lower()
ANDROID_TARGET_ARCHES = ( "arm64", "x64" )

def is_android() -> bool:
    """True when cross-compiling for Android. The is_linux()/is_windows()
    checks keep describing the HOST, so dep scripts test is_android() first."""
    return CROSS_TARGET.startswith( "android-" )

def is_linux() -> bool:
    return sys.platform.startswith("linux")

def is_windows() -> bool:
    return sys.platform == "win32"

def target_arch() -> str:
    """Canonical arch we are building for: "x64", "arm64" or "x86"."""
    if is_android():
        arch = CROSS_TARGET[ len( "android-" ): ]
        if arch not in ANDROID_TARGET_ARCHES:
            abort_op( f"Unsupported Android architecture: {arch!r}" )
        return arch
    if is_windows():
        raw = ( os.environ.get( "VSCMD_ARG_TGT_ARCH", "" ).strip().lower()
                or platform.machine().lower() )
    else:
        raw = platform.machine().lower()

    if raw in ( "x86_64", "amd64", "x64" ):
        return "x64"
    if raw in ( "aarch64", "arm64" ):
        return "arm64"
    if raw in ( "x86", "i386", "i686", "win32" ):
        return "x86"
    abort_op( f"Unsupported architecture: {raw!r}" )

def is_arm64() -> bool:
    return target_arch() == "arm64"

def is_apple_silicon() -> bool:
    return sys.platform == "darwin" and platform.machine() == "arm64"

# ---------------------------------------------------------------------------
# Android NDK
#
# The NDK is located through ANDROID_NDK_ROOT (or ANDROID_NDK_HOME). The API
# level defaults to the app's minSdk and can be overridden with
# EO_ANDROID_API_LEVEL.
# ---------------------------------------------------------------------------

ANDROID_API_LEVEL = os.environ.get( "EO_ANDROID_API_LEVEL", "26" )

def android_ndk_root() -> Path:
    ndk = os.environ.get( "ANDROID_NDK_ROOT" ) or os.environ.get( "ANDROID_NDK_HOME" )
    if not ndk or not Path( ndk ).is_dir():
        abort_op( "Android NDK not found: set ANDROID_NDK_ROOT (or ANDROID_NDK_HOME)" )
    return Path( ndk )

def android_abi() -> str:
    """Android ABI name for the target arch, e.g. "arm64-v8a"."""
    return { "arm64": "arm64-v8a", "x64": "x86_64" }[ target_arch() ]

def android_triple() -> str:
    """Clang/autotools target triple, e.g. "aarch64-linux-android"."""
    return { "arm64": "aarch64-linux-android", "x64": "x86_64-linux-android" }[ target_arch() ]

def android_toolchain_bin() -> Path:
    host_tag = "darwin-x86_64" if sys.platform == "darwin" else "linux-x86_64"
    return android_ndk_root() / "toolchains" / "llvm" / "prebuilt" / host_tag / "bin"

def android_toolchain_env() -> dict[ str, str ]:
    """Compiler environment for autotools/make based cross builds."""
    bin_dir = android_toolchain_bin()
    prefix = f"{ android_triple() }{ ANDROID_API_LEVEL }"
    return {
        "CC": str( bin_dir / f"{ prefix }-clang" ),
        "CXX": str( bin_dir / f"{ prefix }-clang++" ),
        "AR": str( bin_dir / "llvm-ar" ),
        "RANLIB": str( bin_dir / "llvm-ranlib" ),
        "STRIP": str( bin_dir / "llvm-strip" ),
        "NM": str( bin_dir / "llvm-nm" ),
        "LD": str( bin_dir / "ld.lld" ),
    }

def _marker_ok( marker_path : Path, label : str ) -> bool:
    if force_redo or not marker_path.exists():
        return False
    on_disk = marker_path.read_text()
    if on_disk != dep_version:
        print( f"  [INFO] {dep_name} {label} marker version changed "
               f"({on_disk!r} -> {dep_version!r}), redoing" )
        return False
    return True

def work_dir_looks_ok() -> bool:
    return _marker_ok( Path( work_dir / "ok_marker" ), "work dir" )

def install_dir_looks_ok() -> bool:
    return _marker_ok( Path( install_dir / "ok_marker" ), "install dir" )

def create_work_dir_ok_marker():
    Path( work_dir / "ok_marker" ).write_text( dep_version )

def create_install_dir_ok_marker():
    Path( install_dir / "ok_marker" ).write_text( dep_version )

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
            # Compilers such as MSVC's cl report errors on stdout, so show the
            # end of both streams
            output = "\n".join( stream.strip() for stream in ( e.stdout, e.stderr ) if stream and stream.strip() )
            output = "\n".join( output.splitlines()[ -80: ] )
            abort_op( f"{description} failed: {output or e}", error_is_fatal=error_is_fatal )

def capture_process_output( cmd : list[str] ) -> str:
    result = subprocess.run( cmd, capture_output = True, text = True, check = True )
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
            if sys.version_info >= (3, 12):
                shutil.rmtree( repo_dir, onexc=onerror )
            else:
                shutil.rmtree( repo_dir, onerror=onerror )
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


# ===========================================================================
# Remote cache (Nextcloud WebDAV or S3)
#
# One entry point: ensure_dep(build_fn). Call it from every nc-build.py instead
# of the manual `if not install_dir_looks_ok(): build_fn()` check. It will:
#   0. skip if the install dir already looks ok locally
#   1. else download + extract a prebuilt archive from the remote, if present
#   2. else run build_fn() (which must end by creating the ok-marker) and then
#      archive the install dir and upload it for next time
#
# It relies on the module state that init_for_dep() already sets:
#   - install_dir              (the resolved install dir; confirmed used elsewhere)
#   - install_dir_looks_ok()   (the ok-marker check)
#   - the force-redo flag       (read defensively below)
#
# Everything is configured through the environment so nothing is committed.
# Archives are stored under <BASE_REMOTE_PATH>/<PLATFORM_TAG>/<dep>/<version>/
# on the remote, where <version> is the version passed to init_for_dep(). An
# archive is never overwritten: bump the version to publish a new build.
#
#   REMOTE_CACHE_TYPE       "s3" (default) | "nextcloud" | "none".
#   BASE_REMOTE_PATH        Folder / key prefix on the remote (default "3rdparty").
#   REMOTE_CACHE_READ_ONLY  "1" to only download, never upload. Implied for S3
#                           without credentials.
#
# S3 (any S3 compatible store, path-style requests):
#   REMOTE_CACHE_S3_URL     <endpoint>/<bucket> (default
#                           "https://s3.eu-central-4.ionoscloud.com/eo-build-assets",
#                           readable anonymously).
#   REMOTE_CACHE_S3_REGION  Signing region. Default: taken from an
#                           "s3.<region>." host, else "us-east-1".
#   REMOTE_CACHE_S3_ACCESS_KEY, REMOTE_CACHE_S3_SECRET_KEY,
#   REMOTE_CACHE_S3_SESSION_TOKEN
#                           Credentials for uploads. Without them, requests are
#                           sent unsigned.
#
# Nextcloud / WebDAV ('curl' must be on PATH):
#   NEXTCLOUD_REMOTE        WebDAV root (default
#                           "https://cloud.nextcloud.com/remote.php/dav/files").
#   NEXTCLOUD_USER          Appended to NEXTCLOUD_REMOTE as "<remote>/<user>"
#                           when set. Leave empty if NEXTCLOUD_REMOTE already is
#                           the full root, e.g. a public share:
#                           "https://host/public.php/dav/files/<share-token>".
#   NEXTCLOUD_PASS          Basic auth is used only if user and pass are set.
#
# If the cache is disabled, ensure_dep behaves like the old manual check
# (build locally only).
# ===========================================================================

def _env( *names, default="" ):
    # First non-empty variable out of names.
    for nm in names:
        value = os.environ.get( nm, "" ).strip()
        if value:
            return value
    return default

REMOTE_CACHE_TYPE      = _env( "REMOTE_CACHE_TYPE", default="s3" ).lower()
BASE_REMOTE_PATH       = _env( "BASE_REMOTE_PATH", default="3rdparty" ).strip( "/" )
REMOTE_CACHE_READ_ONLY = _env( "REMOTE_CACHE_READ_ONLY" ).lower() in ( "1", "true", "yes", "on" )

NEXTCLOUD_USER   = _env( "NEXTCLOUD_USER" )
NEXTCLOUD_PASS   = _env( "NEXTCLOUD_PASS" )
NEXTCLOUD_REMOTE = _env( "NEXTCLOUD_REMOTE",
                         default="https://cloud.nextcloud.com/remote.php/dav/files" ).rstrip( "/" )

S3_URL           = _env( "REMOTE_CACHE_S3_URL",
                         default="https://s3.eu-central-4.ionoscloud.com/eo-build-assets" ).rstrip( "/" )
S3_ACCESS_KEY    = _env( "REMOTE_CACHE_S3_ACCESS_KEY" )
S3_SECRET_KEY    = _env( "REMOTE_CACHE_S3_SECRET_KEY" )
S3_SESSION_TOKEN = _env( "REMOTE_CACHE_S3_SESSION_TOKEN" )

def _s3_region( url ):
    host = url.split( "://", 1 )[ -1 ].split( "/", 1 )[ 0 ]
    m = re.match( r"s3[.-]([a-z]{2}(?:-[a-z]+)+-\d+)\.", host )
    return m.group( 1 ) if m else "us-east-1"

def _valid_s3_url( url ):
    return re.fullmatch( r"https?://[^/?#]+/[^/?#]+", url ) is not None

S3_REGION = _env( "REMOTE_CACHE_S3_REGION", default=_s3_region( S3_URL ) )

if REMOTE_CACHE_TYPE == "s3":
    USE_REMOTE_CACHE = _valid_s3_url( S3_URL )
    if not USE_REMOTE_CACHE:
        print( "  [WARN]  REMOTE_CACHE_S3_URL must look like https://<endpoint>/<bucket>, remote cache disabled" )
    if not ( S3_ACCESS_KEY and S3_SECRET_KEY ):
        REMOTE_CACHE_READ_ONLY = True
elif REMOTE_CACHE_TYPE == "nextcloud":
    USE_REMOTE_CACHE = bool( ( NEXTCLOUD_USER and NEXTCLOUD_PASS )
                             or _env( "NEXTCLOUD_REMOTE" ) )
else:
    if REMOTE_CACHE_TYPE != "none":
        print( f"  [WARN]  Unknown REMOTE_CACHE_TYPE { REMOTE_CACHE_TYPE !r}, remote cache disabled" )
    USE_REMOTE_CACHE = False

def _cache_tag_platform() -> str:
    # Cross builds get their own folder, so they never reuse or overwrite host
    # archives, e.g. "android-arm64".
    if is_android():
        return CROSS_TARGET
    return f"{ sys.platform }-{ _cache_tag_arch() }"

def _cache_tag_arch() -> str:
    # Arch spelling used in the remote-cache path. Must stay stable, or archives
    # already uploaded become unreachable. On Windows it mirrors what
    # platform.machine() reports there (AMD64 / ARM64 / X86) but follows the
    # build's TARGET arch; elsewhere it's the raw host machine string.
    if is_windows():
        return { "x64": "AMD64", "arm64": "ARM64", "x86": "X86" }[ target_arch() ]
    return platform.machine()

# e.g. "linux-x86_64", "win32-AMD64", "win32-ARM64"
PLATFORM_TAG = _cache_tag_platform()

def _force_redo_flag():
    # init_for_dep() stores the forceredo flag in a module global; accept a few
    # likely names so this works without knowing the exact one.
    g = globals()
    for nm in ( "force_redo", "forceredo", "_force_redo" ):
        if nm in g:
            return bool( g[ nm ] )
    return False


def _cache_key():
    # Folder name of the install dir, e.g. "harfbuzz". Stable across runs and
    # independent of the (sometimes capitalised) depname.
    return install_dir.name


# ---------------------------------------------------------------------------
# glibc compatibility (Linux only)
#
# A binary built against glibc X runs on any glibc >= X, but not on an older
# one. Linux archives are therefore uploaded with the glibc version they were
# built against in the file name ("v8-glibc2.34.tar.bz2"), and a build may
# reuse any archive whose glibc is older than or equal to its own. If there is
# none, it builds locally and uploads under its own glibc version.
#
# Example: the Rocky 9 server build (2.34) uploads v8-glibc2.34. The Ubuntu
# 22.04 (2.35) and 24.04 (2.39) desktop builds then reuse that archive instead
# of building V8 again. An archive uploaded from 24.04 is never used by Rocky 9.
#
# Other platforms keep the plain "<key>.tar.bz2" name. Old untagged Linux
# archives are ignored, because their glibc requirement is unknown.
# ---------------------------------------------------------------------------

def _local_glibc():
    # Android binaries link against bionic, not the host glibc.
    if not sys.platform.startswith( "linux" ) or is_android():
        return None
    name, version = platform.libc_ver()
    if name != "glibc" or not version:
        return None
    try:
        return tuple( int( p ) for p in version.split( "." )[ :2 ] )
    except ValueError:
        return None

LOCAL_GLIBC = _local_glibc()


def _glibc_str( ver ):
    return f"{ ver[ 0 ] }.{ ver[ 1 ] }"


def _remote_dir_path():
    # Remote folder of this dep version, relative to the cache root.
    parts = ( BASE_REMOTE_PATH, PLATFORM_TAG, _cache_key(), dep_version )
    return "/".join( p for p in parts if p )


def _remote_file_path( glibc=None ):
    key = _cache_key()
    name = f"{ key }-glibc{ _glibc_str( glibc ) }" if glibc else key
    return f"{ _remote_dir_path() }/{ name }.tar.bz2"


# ---------------------------------------------------------------------------
# Backends. Each one takes paths relative to the cache root and implements:
#   exists( path ) -> bool
#   list( dir )    -> str   (raw listing; only searched for archive names)
#   get( path, dest_file ) -> bool
#   put( src_file, path )  -> bool
# ---------------------------------------------------------------------------

class _WebDavBackend:
    def __init__( self ):
        self.root = ( f"{ NEXTCLOUD_REMOTE }/{ NEXTCLOUD_USER }"
                      if NEXTCLOUD_USER else NEXTCLOUD_REMOTE )

    def _url( self, path ):
        return f"{ self.root }/{ path }"

    def _curl( self, *args ):
        auth = ( [ "-u", f"{ NEXTCLOUD_USER }:{ NEXTCLOUD_PASS }" ]
                 if NEXTCLOUD_USER and NEXTCLOUD_PASS else [] )
        return subprocess.run(
            [ "curl", "-s", *auth, *args ],
            text=True, capture_output=True,
        )

    def exists( self, path ):
        # HEAD request: True only on a 2xx status code.
        r = self._curl( "-o", os.devnull, "-w", "%{http_code}", "--head", self._url( path ) )
        return r.stdout.strip().startswith( "2" )

    def list( self, dir ):
        # WebDAV PROPFIND, depth 1. A missing folder simply yields no entries.
        r = self._curl( "-X", "PROPFIND", "-H", "Depth: 1", self._url( dir ) + "/" )
        return r.stdout if r.returncode == 0 else ""

    def get( self, path, dest_file ):
        return self._curl( "-f", "-o", dest_file, self._url( path ) ).returncode == 0

    def put( self, src_file, path ):
        # WebDAV does not create intermediate collections, so MKCOL each level
        # (MKCOL on an existing collection just 405s, which we ignore).
        parts = path.split( "/" )[ :-1 ]
        for i in range( 1, len( parts ) + 1 ):
            self._curl( "-X", "MKCOL", self._url( "/".join( parts[ :i ] ) ) )
        self._curl( "-X", "DELETE", self._url( path ) )
        return self._curl( "-f", "-T", src_file, self._url( path ) ).returncode == 0


class _S3Backend:
    # Plain urllib + AWS Signature V4, so no boto3 / aws cli is needed.

    _EMPTY_SHA256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

    def __init__( self ):
        from urllib.parse import urlsplit
        url = urlsplit( S3_URL )
        self.scheme = url.scheme
        self.host   = url.netloc
        self.prefix = url.path

    def _request( self, method, key="", query=None, body=None, size=0 ):
        import datetime, hashlib, hmac
        from urllib.parse import quote
        from urllib.request import Request

        uri = quote( f"{ self.prefix }/{ key }", safe="/-_.~" )
        qs  = "&".join( f"{ quote( k, safe='-_.~' ) }={ quote( v, safe='-_.~' ) }"
                        for k, v in sorted( ( query or {} ).items() ) )
        payload_hash = "UNSIGNED-PAYLOAD" if body is not None else self._EMPTY_SHA256

        headers = { "host": self.host, "x-amz-content-sha256": payload_hash }
        if body is not None:
            headers[ "content-length" ] = str( size )

        if S3_ACCESS_KEY and S3_SECRET_KEY:
            now   = datetime.datetime.now( datetime.timezone.utc )
            stamp = now.strftime( "%Y%m%dT%H%M%SZ" )
            day   = stamp[ :8 ]
            headers[ "x-amz-date" ] = stamp
            if S3_SESSION_TOKEN:
                headers[ "x-amz-security-token" ] = S3_SESSION_TOKEN

            signed    = ";".join( sorted( headers ) )
            canonical = "\n".join( [
                method, uri, qs,
                "".join( f"{ k }:{ headers[ k ].strip() }\n" for k in sorted( headers ) ),
                signed, payload_hash,
            ] )
            scope = f"{ day }/{ S3_REGION }/s3/aws4_request"
            to_sign = "\n".join( [ "AWS4-HMAC-SHA256", stamp, scope,
                                   hashlib.sha256( canonical.encode() ).hexdigest() ] )

            def _hmac( k, msg ):
                return hmac.new( k, msg.encode(), hashlib.sha256 ).digest()
            k = _hmac( f"AWS4{ S3_SECRET_KEY }".encode(), day )
            for part in ( S3_REGION, "s3", "aws4_request" ):
                k = _hmac( k, part )
            signature = hmac.new( k, to_sign.encode(), hashlib.sha256 ).hexdigest()
            headers[ "authorization" ] = ( f"AWS4-HMAC-SHA256 Credential={ S3_ACCESS_KEY }/{ scope }, "
                                           f"SignedHeaders={ signed }, Signature={ signature }" )

        url = f"{ self.scheme }://{ self.host }{ uri }" + ( f"?{ qs }" if qs else "" )
        headers.pop( "host" )  # urllib sets it from the URL
        return Request( url, data=body, method=method, headers=headers )

    def _open( self, req ):
        # Response on 2xx, None on any HTTP or network error. Anything but a
        # 404 is reported, so a broken setup doesn't look like a cache miss.
        from urllib.request import urlopen
        from urllib.error import HTTPError, URLError
        try:
            return urlopen( req, timeout=120 )
        except HTTPError as e:
            if e.code != 404:
                print( f"  [WARN]  S3 { req.get_method() } failed: HTTP { e.code } { e.reason }" )
        except ( URLError, OSError, http.client.HTTPException ) as e:
            print( f"  [WARN]  S3 { req.get_method() } failed: { e }" )
        return None

    def exists( self, path ):
        r = self._open( self._request( "HEAD", path ) )
        if r is None:
            return False
        r.close()
        return True

    def list( self, dir ):
        r = self._open( self._request( "GET", query={ "list-type": "2", "prefix": dir + "/" } ) )
        if r is None:
            return ""
        with r:
            return r.read().decode( "utf-8", "replace" )

    def get( self, path, dest_file ):
        r = self._open( self._request( "GET", path ) )
        if r is None:
            return False
        try:
            with r, open( dest_file, "wb" ) as f:
                shutil.copyfileobj( r, f, 1024 * 1024 )
                expected = r.headers.get( "Content-Length" )
                # urllib doesn't raise when the connection drops early
                return expected is None or f.tell() == int( expected )
        except ( OSError, http.client.HTTPException ):
            return False

    def put( self, src_file, path ):
        with open( src_file, "rb" ) as f:
            size = os.fstat( f.fileno() ).st_size
            r = self._open( self._request( "PUT", path, body=f, size=size ) )
        if r is None:
            return False
        r.close()
        return True


_backend_instance = None

def _backend():
    global _backend_instance
    if _backend_instance is None:
        _backend_instance = _S3Backend() if REMOTE_CACHE_TYPE == "s3" else _WebDavBackend()
    return _backend_instance


def _remote_glibc_versions():
    # glibc versions of the archives in this dep's remote folder.
    listing = _backend().list( _remote_dir_path() )
    pattern = re.compile( re.escape( _cache_key() ) + r"-glibc(\d+)\.(\d+)\.tar\.bz2" )
    return sorted( { ( int( a ), int( b ) ) for a, b in pattern.findall( listing ) } )


def _remote_find_archive():
    # Remote path of the archive to reuse, or None.
    if LOCAL_GLIBC is None:
        path = _remote_file_path()
        return path if _backend().exists( path ) else None
    # Newest archive that is still compatible with the local glibc.
    usable = [ v for v in _remote_glibc_versions() if v <= LOCAL_GLIBC ]
    return _remote_file_path( usable[ -1 ] ) if usable else None


def _remote_download_and_extract( path ):
    with tempfile.TemporaryDirectory() as tmp:
        archive = str( Path( tmp ) / "dep.tar.bz2" )
        if not _backend().get( path, archive ):
            return False
        if install_dir.exists():
            shutil.rmtree( install_dir )
        install_dir.mkdir( parents=True, exist_ok=True )
        try:
            shutil.unpack_archive( archive, str( install_dir ) )
        except ( EOFError, OSError, shutil.ReadError, tarfile.TarError ) as e:
            print( f"  [WARN]  Could not extract { path.rsplit( '/', 1 )[ -1 ] }: { e }" )
            return False
        return True


def _remote_upload():
    with tempfile.TemporaryDirectory() as tmp:
        # Archive into a temp dir so the .tar.bz2 is never inside install_dir
        # (important since work dir and install dir can be the same path).
        archive = shutil.make_archive( str( Path( tmp ) / _cache_key() ),
                                       "bztar", root_dir=str( install_dir ) )
        return _backend().put( archive, _remote_file_path( LOCAL_GLIBC ) )


def _remote_upload_needed():
    # Archives are immutable: never replace one that is already there.
    return not _backend().exists( _remote_file_path( LOCAL_GLIBC ) )


def ensure_dep( build_fn, forceredo=None ):
    """
    Drop-in replacement for `if not install_dir_looks_ok(): build_fn()`.

    Call once per dependency, after init_for_dep(). `build_fn` must create the
    ok-marker (e.g. via create_install_dir_ok_marker()) when it finishes -- the
    marker is what makes the dep count as "done" both locally and inside the
    uploaded archive.
    """
    force_redo = _force_redo_flag() if forceredo is None else bool( forceredo )
    name = _cache_key()

    # 0. Already built locally. install_dir_looks_ok() already returns False on
    #    a forced redo, so this correctly does NOT skip in that case.
    if install_dir_looks_ok():
        print( f"  [OK] { name } already present locally, skipping" )
        return

    # 1. Prebuilt archive on the remote (skipped on a forced redo, which always
    #    builds locally; the remote copy is never replaced).
    remote_path = _remote_find_archive() if USE_REMOTE_CACHE and not force_redo else None
    if remote_path:
        print( f"  [GET]  Found { name } on remote ({ remote_path.rsplit( '/', 1 )[ -1 ] }), downloading..." )
        if _remote_download_and_extract( remote_path ) and install_dir_looks_ok():
            print( f"  [OK] { name } fetched from remote" )
            return
        print( "  [WARN]  Remote copy missing/incomplete, building locally." )

    # 2. Build locally, then archive + upload for next time.
    build_fn()
    if not install_dir_looks_ok():
        print( f"  [WARN]  { name }: build finished but no ok-marker was created" )
        return
    if USE_REMOTE_CACHE and not REMOTE_CACHE_READ_ONLY:
        if not _remote_upload_needed():
            print( f"  [OK] { name } already on remote, not replacing it" )
            return
        print( f"  [PUT]  Uploading { name } to remote..." )
        if _remote_upload():
            print( f"  [OK] { name } uploaded" )
        else:
            print( f"  [WARN]  Upload of { name } failed" )