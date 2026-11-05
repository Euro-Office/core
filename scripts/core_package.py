#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

"""Prebuilt core packages, so DocumentServer, DesktopEditors and MobileEditors
don't have to build core again.

Core CI packs its build output and publishes it, keyed by the git tree of the
core checkout (`git rev-parse HEAD^{tree}`): any commit with the same sources
gets the same package. Downstream builds fetch the package for their core
submodule and fall back to building core from source when there is none.

Remote layout (below EO_CORE_CACHE_PREFIX, default "core"):
  <channel>/<platform>/<product>/<tree>.tar.gz   build output
  <channel>/<platform>/<product>/<tree>.json     manifest
  pr-heads/<pr>/<platform>--<product>            tree last published by a PR

<channel> is "main" (pushes to main) or "pr" (same-repo pull requests). Only
the newest package of a PR is kept: publishing a new one deletes the previous
one, and `prune-pr` deletes the rest when the PR is closed, or the older ones
when its newest commit can't have a package (--keep-tree). Either keeps a
package that another PR still has as its head. Packages are never replaced:
the manifest marks a package as complete, so an archive without one is a
leftover that the next publish overwrites.

Commands:
  pack      archive a build output directory and write its manifest
  publish   upload packed archives (needs credentials)
  prune-pr  delete all packages of a pull request (needs credentials)
  fetch     download and extract the package matching a core checkout

fetch exits with 0 when the package is in place, with 3 when core has to be
built from source, and with 1 on errors. It never takes the cache for a core
checkout with local changes (`git status --porcelain` lists anything,
untracked files included), so local work on core is always built, nor for
a core directory that isn't a git checkout of its own.

A package doubles as input for other CMake projects: see
cmake/prebuilt-core.cmake (EO_CORE_PREBUILT_DIR).

Environment:
  EO_CORE_CACHE                 fetch mode: "auto" (default), "off" (always
                                build from source) or "require" (fail
                                instead of building, for CI)
  EO_CORE_CACHE_PREFIX          key prefix (default "core")
  EO_CORE_CACHE_S3_URL          <endpoint>/<bucket> (default: the
                                eo-build-assets bucket, readable anonymously)
  EO_CORE_CACHE_S3_ACCESS_KEY,
  EO_CORE_CACHE_S3_SECRET_KEY   credentials for publish and prune-pr
"""

import argparse
import datetime
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zlib
from pathlib import Path

import remote_cache

FORMAT = 1
EXIT_BUILD_FROM_SOURCE = 3

CHANNELS = ( "main", "pr" )
MARKER = ".core-package.json"
_NAME_RE = re.compile( r"[a-z0-9][a-z0-9_.-]*" )
_TREE_RE = re.compile( r"[0-9a-f]{40}([0-9a-f]{24})?" )


class Error( Exception ):
    pass


def _prefix():
    return remote_cache.env( "EO_CORE_CACHE_PREFIX", default="core" ).strip( "/" )


def make_backend():
    url = remote_cache.env( "EO_CORE_CACHE_S3_URL", default=remote_cache.DEFAULT_S3_URL ).rstrip( "/" )
    if not remote_cache.valid_s3_url( url ):
        raise Error( "EO_CORE_CACHE_S3_URL must look like https://<endpoint>/<bucket>" )
    return remote_cache.S3Backend( url,
                                   access_key=remote_cache.env( "EO_CORE_CACHE_S3_ACCESS_KEY" ),
                                   secret_key=remote_cache.env( "EO_CORE_CACHE_S3_SECRET_KEY" ) )


def _check_name( kind, value ):
    if not _NAME_RE.fullmatch( value ):
        raise Error( f"invalid { kind } { value !r}" )
    return value


def package_path( channel, platform, product, tree, ext ):
    return f"{ _prefix() }/{ channel }/{ platform }/{ product }/{ tree }.{ ext }"


def pr_head_path( pr, platform, product ):
    return f"{ _prefix() }/pr-heads/{ pr }/{ platform }--{ product }"


# ---------------------------------------------------------------------------
# git
# ---------------------------------------------------------------------------

def _git( core, *args ):
    r = subprocess.run( [ "git", "-C", str( core ), *args ], capture_output=True, text=True )
    if r.returncode != 0:
        raise Error( f"git { ' '.join( args ) } failed in { core }: { r.stderr.strip() }" )
    return r.stdout


def is_checkout( core ):
    # core must be the top of its own work tree: inside another repository
    # (a plain copy in a superproject), git would name that one's tree
    try:
        r = subprocess.run( [ "git", "-C", str( core ), "rev-parse", "--show-toplevel" ],
                            capture_output=True, text=True )
    except OSError:
        return False
    return r.returncode == 0 and os.path.realpath( r.stdout.strip() ) == os.path.realpath( core )


def source_tree( core ):
    return _git( core, "rev-parse", "HEAD^{tree}" ).strip()


def local_changes( core, untracked=True ):
    args = [ "status", "--porcelain" ]
    if not untracked:
        args.append( "--untracked-files=no" )
    return _git( core, *args ).splitlines()


# ---------------------------------------------------------------------------
# pack
# ---------------------------------------------------------------------------

def _sha256( path ):
    h = hashlib.sha256()
    with open( path, "rb" ) as f:
        for block in iter( lambda: f.read( 1024 * 1024 ), b"" ):
            h.update( block )
    return h.hexdigest()


def _reset_owner( info ):
    info.uid = info.gid = 0
    info.uname = info.gname = ""
    return info


def pack( src, out, platform, product, core, toolchain="" ):
    """Archives the contents of src as out/<platform>--<product>.tar.gz and
    writes the manifest next to it. Returns the manifest path."""
    src = Path( src )
    if not src.is_dir() or not any( src.iterdir() ):
        raise Error( f"{ src } is missing or empty" )
    _check_name( "platform", platform )
    _check_name( "product", product )
    # The tree names the sources: tracked changes would make it lie.
    # Untracked files (build dirs, caches) don't change what was built.
    changes = local_changes( core, untracked=False )
    if changes:
        raise Error( "the core checkout has local changes:\n  " + "\n  ".join( changes ) )

    out = Path( out )
    out.mkdir( parents=True, exist_ok=True )
    archive = out / f"{ platform }--{ product }.tar.gz"
    with tarfile.open( archive, "w:gz", compresslevel=6 ) as tar:
        for path in sorted( src.rglob( "*" ) ):
            tar.add( path, arcname=path.relative_to( src ).as_posix(),
                     recursive=False, filter=_reset_owner )

    run = ""
    if os.environ.get( "GITHUB_RUN_ID" ):
        run = ( f"{ os.environ.get( 'GITHUB_SERVER_URL', 'https://github.com' ) }/"
                f"{ os.environ.get( 'GITHUB_REPOSITORY', '' ) }/actions/runs/{ os.environ[ 'GITHUB_RUN_ID' ] }" )
    manifest = {
        "format": FORMAT,
        "tree": source_tree( core ),
        "commit": _git( core, "rev-parse", "HEAD" ).strip(),
        "platform": platform,
        "product": product,
        "sha256": _sha256( archive ),
        "size": archive.stat().st_size,
        "created": datetime.datetime.now( datetime.timezone.utc ).strftime( "%Y-%m-%dT%H:%M:%SZ" ),
        "toolchain": toolchain,
        "run": run,
    }
    manifest_path = archive.with_name( f"{ platform }--{ product }.json" )
    manifest_path.write_text( json.dumps( manifest, indent=2 ) + "\n" )
    print( f"Packed { archive } ({ manifest[ 'size' ] // ( 1024 * 1024 ) } MiB, tree { manifest[ 'tree' ] })" )
    return manifest_path


# ---------------------------------------------------------------------------
# publish / prune
# ---------------------------------------------------------------------------

def _put_text( backend, path, text ):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join( tmp, "obj" )
        Path( src ).write_text( text )
        return backend.put( src, path )


def _delete_package( backend, channel, platform, product, tree ):
    # Manifest first: what's left after a failure is only a leftover archive
    deleted = True
    for ext in ( "json", "tar.gz" ):
        path = package_path( channel, platform, product, tree, ext )
        if not backend.delete( path ):
            print( f"  [WARN]  could not delete { path }" )
            deleted = False
    if deleted:
        print( f"Deleted { channel }/{ platform }/{ product }/{ tree }" )
    return deleted


def _head_of_other_pr( backend, pr, platform, product, tree ):
    # Packages are keyed by tree, so two PRs can share one
    own = pr_head_path( pr, platform, product )
    for head in backend.keys( f"{ _prefix() }/pr-heads" ):
        if ( head != own and head.rsplit( "/", 1 )[ -1 ] == f"{ platform }--{ product }"
                and backend.read( head ) == tree ):
            return True
    return False


def _release_pr_package( backend, pr, platform, product, tree ):
    """Deletes a package PR pr no longer needs. Returns False on failure."""
    if _head_of_other_pr( backend, pr, platform, product, tree ):
        print( f"Keeping pr/{ platform }/{ product }/{ tree }: another PR uses it" )
        return True
    return _delete_package( backend, "pr", platform, product, tree )


def _upload( backend, archive, manifest, channel, platform, product, tree ):
    manifest_key = package_path( channel, platform, product, tree, "json" )
    if backend.read( manifest_key ) is not None:
        print( f"  { manifest_key } exists, not replacing the package" )
        return
    # Archive before manifest: a manifest always points at a complete upload
    for src, path in ( ( archive, package_path( channel, platform, product, tree, "tar.gz" ) ),
                       ( manifest, manifest_key ) ):
        if not backend.put( str( src ), path ):
            raise Error( f"upload of { path } failed" )
        print( f"  uploaded { path }" )


def publish( backend, manifests, channel, pr=None ):
    if channel not in CHANNELS:
        raise Error( f"channel must be one of { ', '.join( CHANNELS ) }" )
    if ( channel == "pr" ) != ( pr is not None ):
        raise Error( "--pr is required for the pr channel, and only for it" )
    if not backend.can_write:
        raise Error( "publishing needs EO_CORE_CACHE_S3_ACCESS_KEY and EO_CORE_CACHE_S3_SECRET_KEY" )

    for manifest_path in map( Path, manifests ):
        m = json.loads( manifest_path.read_text() )
        if m.get( "format" ) != FORMAT:
            raise Error( f"{ manifest_path }: unsupported format { m.get( 'format' ) }" )
        platform = _check_name( "platform", m[ "platform" ] )
        product  = _check_name( "product", m[ "product" ] )
        tree     = m[ "tree" ]
        if not _TREE_RE.fullmatch( tree ):
            raise Error( f"{ manifest_path }: invalid tree { tree !r}" )
        archive = manifest_path.with_name( f"{ platform }--{ product }.tar.gz" )
        if _sha256( archive ) != m[ "sha256" ]:
            raise Error( f"{ archive } does not match its manifest" )

        print( f"Publishing { channel }/{ platform }/{ product }/{ tree }" )
        if channel == "pr":
            # The head moves first: whatever fails below, every package of
            # this PR is recorded, and another PR's prune sees this one's claim
            head = pr_head_path( pr, platform, product )
            previous = backend.read( head )
            if previous and previous != tree and _TREE_RE.fullmatch( previous ):
                if not _release_pr_package( backend, pr, platform, product, previous ):
                    raise Error( f"could not delete the previous package { previous }" )
            if not _put_text( backend, head, tree ):
                raise Error( f"could not record { head }" )
        _upload( backend, archive, manifest_path, channel, platform, product, tree )


def prune_pr( backend, pr, keep_tree=None ):
    if not backend.can_write:
        raise Error( "pruning needs EO_CORE_CACHE_S3_ACCESS_KEY and EO_CORE_CACHE_S3_SECRET_KEY" )
    heads = backend.keys( f"{ _prefix() }/pr-heads/{ pr }" )
    failed = []
    for head in heads:
        platform, _, product = head.rsplit( "/", 1 )[ -1 ].partition( "--" )
        tree = backend.read( head )
        if keep_tree and tree == keep_tree:
            continue
        if tree and _TREE_RE.fullmatch( tree ) and _NAME_RE.fullmatch( platform ) and _NAME_RE.fullmatch( product ):
            # The head is the only reference to the package: keep it for a retry
            if not _release_pr_package( backend, pr, platform, product, tree ):
                failed.append( head )
                continue
        if not backend.delete( head ):
            failed.append( head )
    if failed:
        raise Error( "could not prune " + ", ".join( failed ) )
    if not heads:
        print( f"No packages recorded for PR { pr }" )


# ---------------------------------------------------------------------------
# fetch
# ---------------------------------------------------------------------------

def _extract( archive, dest ):
    with tarfile.open( archive, "r:gz" ) as tar:
        if hasattr( tarfile, "data_filter" ):
            tar.extractall( dest, filter="data" )
            return
        # Python without extraction filters: refuse paths leaving dest
        root = os.path.realpath( dest )
        for member in tar.getmembers():
            target = os.path.realpath( os.path.join( dest, member.name ) )
            if os.path.commonpath( [ root, target ] ) != root or member.islnk() or member.isdev():
                raise Error( f"unsafe archive member { member.name }" )
            if member.issym():
                link = os.path.realpath( os.path.join( os.path.dirname( target ), member.linkname ) )
                if os.path.commonpath( [ root, link ] ) != root:
                    raise Error( f"unsafe symlink { member.name }" )
        tar.extractall( dest )


def _is_installed( dest, tree, platform, product, channels ):
    # Not only the tree: dest doesn't name the platform or channel
    try:
        marker = json.loads( ( dest / MARKER ).read_text() )
    except ( OSError, ValueError ):
        return False
    return ( isinstance( marker, dict ) and marker.get( "tree" ) == tree
             and marker.get( "platform" ) == platform and marker.get( "product" ) == product
             and marker.get( "channel" ) in channels )


def _parse_manifest( text, platform, product, tree ):
    try:
        manifest = json.loads( text )
    except ValueError:
        return None
    if not isinstance( manifest, dict ):
        return None
    expected = { "format": FORMAT, "tree": tree, "platform": platform, "product": product }
    if any( manifest.get( key ) != value for key, value in expected.items() ):
        return None
    if not isinstance( manifest.get( "size" ), int ) or not isinstance( manifest.get( "sha256" ), str ):
        return None
    return manifest


def _try_channel( backend, channel, platform, product, tree, dest ):
    try:
        manifest_text = backend.read( package_path( channel, platform, product, tree, "json" ) )
    except remote_cache.RemoteError as e:
        print( f"  [WARN]  { e }" )
        return False
    if manifest_text is None:
        return False
    manifest = _parse_manifest( manifest_text, platform, product, tree )
    if manifest is None:
        print( f"  [WARN]  { channel } manifest for { tree } doesn't match, ignoring it" )
        return False

    print( f"Downloading core { channel }/{ platform }/{ product }/{ tree } "
           f"({ manifest[ 'size' ] // ( 1024 * 1024 ) } MiB)" )
    dest.parent.mkdir( parents=True, exist_ok=True )
    with tempfile.TemporaryDirectory( dir=dest.parent ) as tmp:
        archive = os.path.join( tmp, "core.tar.gz" )
        if not backend.get( package_path( channel, platform, product, tree, "tar.gz" ), archive ):
            print( "  [WARN]  download failed" )
            return False
        if _sha256( archive ) != manifest[ "sha256" ]:
            print( "  [WARN]  checksum mismatch, ignoring the package" )
            return False
        staging = Path( tmp ) / "out"
        staging.mkdir()
        try:
            _extract( archive, staging )
        except ( Error, tarfile.TarError, OSError, EOFError, zlib.error ) as e:
            print( f"  [WARN]  could not extract the package: { e }" )
            return False
        ( staging / MARKER ).write_text( json.dumps( dict( manifest, channel=channel ), indent=2 ) + "\n" )
        if dest.exists():
            shutil.rmtree( dest )
        staging.rename( dest )
    return True


def fetch_mode( mode=None ):
    mode = ( mode or remote_cache.env( "EO_CORE_CACHE", default="auto" ) ).lower()
    if mode not in ( "auto", "off", "require" ):
        raise Error( f"EO_CORE_CACHE must be auto, off or require, not { mode !r}" )
    return mode


def fetch_backend( mode ):
    # The cache is optional: a bad setting must not stop a build from source
    try:
        return make_backend()
    except Error as e:
        if mode == "require":
            raise
        print( f"  [WARN]  { e }" )
        return None


def fetch( backend, core, platform, product, dest, channels=CHANNELS, mode=None ):
    """Returns 0 when dest holds the package, EXIT_BUILD_FROM_SOURCE when core
    has to be built. Raises Error on a miss in "require" mode. backend is None
    when the cache can't be used."""
    mode = fetch_mode( mode )
    _check_name( "platform", platform )
    _check_name( "product", product )
    for channel in channels:
        if channel not in CHANNELS:
            raise Error( f"unknown channel { channel !r}" )
    dest = Path( dest )

    # dest is replaced as a whole, so it must be ours (or not exist)
    if dest.exists() and any( dest.iterdir() ) and not ( dest / MARKER ).exists():
        raise Error( f"{ dest } exists and was not created by fetch" )

    def miss( reason ):
        if mode == "require":
            raise Error( f"no prebuilt core: { reason }" )
        # A package fetched earlier no longer matches: don't leave it around
        if dest.exists():
            try:
                shutil.rmtree( dest )
            except OSError as e:
                print( f"  [WARN]  could not remove { dest }: { e }" )
        print( f"Building core from source: { reason }" )
        return EXIT_BUILD_FROM_SOURCE

    if mode == "off":
        return miss( "EO_CORE_CACHE=off" )
    if backend is None:
        return miss( "the build cache is not usable" )
    if not is_checkout( core ):
        return miss( f"{ core } is not a git checkout" )
    changes = local_changes( core )
    if changes:
        return miss( f"{ core } has local changes" )

    tree = source_tree( core )
    if _is_installed( dest, tree, platform, product, channels ):
        print( f"Core { tree } is already in { dest }" )
        return 0
    for channel in channels:
        try:
            if _try_channel( backend, channel, platform, product, tree, dest ):
                print( f"Core { tree } is in { dest }" )
                return 0
        except OSError as e:
            print( f"  [WARN]  could not install the package: { e }" )
    return miss( f"no { platform }/{ product } package for tree { tree }" )


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main( argv=None ):
    parser = argparse.ArgumentParser( description=__doc__.split( "\n\n" )[ 0 ] )
    sub = parser.add_subparsers( dest="command", required=True )

    p = sub.add_parser( "pack", help="archive a build output directory" )
    p.add_argument( "--from", dest="src", required=True, help="build output directory" )
    p.add_argument( "--out", required=True, help="directory for the archive and manifest" )
    p.add_argument( "--platform", required=True, help="e.g. linux-amd64, windows-x64, android-arm64-v8a" )
    p.add_argument( "--product", required=True, help="e.g. server, desktop, android" )
    p.add_argument( "--core", default=".", help="core checkout (default: .)" )
    p.add_argument( "--toolchain", default="", help="free text recorded in the manifest" )

    p = sub.add_parser( "publish", help="upload packed archives" )
    p.add_argument( "--channel", required=True, choices=CHANNELS )
    p.add_argument( "--pr", type=int, help="pull request number (pr channel)" )
    p.add_argument( "manifests", nargs="+", help="manifests written by pack" )

    p = sub.add_parser( "prune-pr", help="delete all packages of a pull request" )
    p.add_argument( "--pr", type=int, required=True )
    p.add_argument( "--keep-tree", help="keep the package of this tree" )

    p = sub.add_parser( "fetch", help="download the package matching a core checkout" )
    p.add_argument( "--platform", required=True )
    p.add_argument( "--product", required=True )
    p.add_argument( "--dest", required=True, help="directory to extract into (replaced)" )
    p.add_argument( "--core", default=".", help="core checkout (default: .)" )
    p.add_argument( "--channels", default="main,pr",
                    help="channels to try, in order (default: main,pr; releases: main)" )

    args = parser.parse_args( argv )
    try:
        if args.command == "pack":
            pack( args.src, args.out, args.platform, args.product, args.core, args.toolchain )
            return 0
        if args.command == "publish":
            publish( make_backend(), args.manifests, args.channel, args.pr )
            return 0
        if args.command == "prune-pr":
            prune_pr( make_backend(), args.pr, args.keep_tree )
            return 0
        mode = fetch_mode()
        result = fetch( fetch_backend( mode ), args.core, args.platform, args.product, args.dest,
                        [ c for c in args.channels.split( "," ) if c ], mode )
        if os.environ.get( "GITHUB_OUTPUT" ):
            with open( os.environ[ "GITHUB_OUTPUT" ], "a" ) as f:
                f.write( f"hit={ 'true' if result == 0 else 'false' }\n" )
        return result
    except ( Error, remote_cache.RemoteError, OSError ) as e:
        print( f"core_package: { e }", file=sys.stderr )
        return 1


if __name__ == "__main__":
    sys.exit( main() )
