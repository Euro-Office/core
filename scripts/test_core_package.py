# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Core package cache tests. Run: python3 -m unittest discover -s scripts

import io
import json
import os
import subprocess
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest import mock
from urllib.error import HTTPError, URLError

import core_package as cp
import remote_cache


class FakeBackend:
    def __init__( self, can_write=True ):
        self.files = {}
        self.can_write = can_write
        self.undeletable = set()

    def exists( self, path ):
        return path in self.files

    def keys( self, dir ):
        return sorted( k for k in self.files if k.startswith( dir + "/" ) )

    def read( self, path ):
        if path not in self.files:
            return None
        return self.files[ path ].decode( "utf-8", "replace" ).strip()

    def get( self, path, dest_file ):
        if path not in self.files:
            return False
        Path( dest_file ).write_bytes( self.files[ path ] )
        return True

    def put( self, src_file, path ):
        self.files[ path ] = Path( src_file ).read_bytes()
        return True

    def delete( self, path ):
        if path in self.undeletable:
            return False
        self.files.pop( path, None )
        return True


def git( repo, *args ):
    subprocess.run( [ "git", "-C", str( repo ), *args ], check=True, capture_output=True )


class CorePackageTest( unittest.TestCase ):
    def setUp( self ):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup( tmp.cleanup )
        self.tmp = Path( tmp.name )

        self.core = self.tmp / "core"
        self.core.mkdir()
        git( self.core, "init", "-q" )
        ( self.core / ".gitignore" ).write_text( "build/\n" )
        ( self.core / "a.cpp" ).write_text( "int a;\n" )
        self.commit( "initial" )

        self.output = self.core / "build" / "package"
        ( self.output / "bin" ).mkdir( parents=True )
        ( self.output / "bin" / "x2t" ).write_text( "x2t" )
        os.symlink( "libicuuc.so.74.2", self.output / "bin" / "libicuuc.so.74" )
        ( self.output / "bin" / "libicuuc.so.74.2" ).write_text( "icu" )

        self.backend = FakeBackend()
        self.quiet = mock.patch( "sys.stdout", new_callable=io.StringIO )
        self.quiet.start()
        self.addCleanup( self.quiet.stop )

    def commit( self, message ):
        git( self.core, "add", "-A" )
        git( self.core, "-c", "user.name=t", "-c", "user.email=t@t", "commit", "-q", "-m", message )

    def tree( self ):
        return cp.source_tree( self.core )

    def pack_and_publish( self, channel="main", pr=None ):
        manifest = cp.pack( self.output, self.tmp / "dist", "linux-amd64", "server", self.core )
        cp.publish( self.backend, [ manifest ], channel, pr )
        return manifest

    def fetch( self, platform="linux-amd64", product="server", **kwargs ):
        kwargs.setdefault( "mode", "auto" )
        return cp.fetch( self.backend, self.core, platform, product, self.tmp / "out", **kwargs )

    def key( self, ext, channel="main", tree=None ):
        return f"core/{ channel }/linux-amd64/server/{ tree or self.tree() }.{ ext }"

    def next_commit( self, content ):
        ( self.core / "a.cpp" ).write_text( content )
        self.commit( content )

    def test_roundtrip( self ):
        self.pack_and_publish()
        self.assertIn( f"core/main/linux-amd64/server/{ self.tree() }.tar.gz", self.backend.files )
        self.assertEqual( self.fetch(), 0 )
        out = self.tmp / "out"
        self.assertEqual( ( out / "bin" / "x2t" ).read_text(), "x2t" )
        self.assertEqual( os.readlink( out / "bin" / "libicuuc.so.74" ), "libicuuc.so.74.2" )
        self.assertEqual( json.loads( ( out / cp.MARKER ).read_text() )[ "channel" ], "main" )

    def test_untracked_build_output_does_not_block_pack( self ):
        ( self.core / "stray.log" ).write_text( "x" )
        self.pack_and_publish()

    def test_pack_refuses_tracked_changes( self ):
        ( self.core / "a.cpp" ).write_text( "int b;\n" )
        with self.assertRaises( cp.Error ):
            cp.pack( self.output, self.tmp / "dist", "linux-amd64", "server", self.core )

    def test_local_changes_build_from_source( self ):
        self.pack_and_publish()
        for change in ( "a.cpp", "new.cpp" ):
            ( self.core / change ).write_text( "int c;\n" )
            self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE, change )
            self.assertFalse( ( self.tmp / "out" ).exists() )
            git( self.core, "checkout", "-q", "--", "." )
            git( self.core, "clean", "-qfx", "--", "new.cpp" )

    def test_local_change_removes_earlier_fetch( self ):
        self.pack_and_publish()
        self.assertEqual( self.fetch(), 0 )
        ( self.core / "a.cpp" ).write_text( "int c;\n" )
        self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )
        self.assertFalse( ( self.tmp / "out" ).exists() )

    def test_not_a_checkout_builds_from_source( self ):
        self.pack_and_publish()
        copy = self.tmp / "copy"
        copy.mkdir()
        ( copy / "a.cpp" ).write_text( "int a;\n" )
        self.assertEqual( cp.fetch( self.backend, copy, "linux-amd64", "server", self.tmp / "out", mode="auto" ),
                          cp.EXIT_BUILD_FROM_SOURCE )
        # a plain directory inside another checkout isn't one either
        ( self.core / "sub" ).mkdir()
        self.assertEqual( cp.fetch( self.backend, self.core / "sub", "linux-amd64", "server", self.tmp / "out", mode="auto" ),
                          cp.EXIT_BUILD_FROM_SOURCE )

    def test_modes( self ):
        self.pack_and_publish()
        self.assertEqual( self.fetch( mode="off" ), cp.EXIT_BUILD_FROM_SOURCE )
        ( self.core / "a.cpp" ).write_text( "int c;\n" )
        self.commit( "change" )
        self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )
        with self.assertRaises( cp.Error ):
            self.fetch( mode="require" )

    def test_releases_ignore_pr_packages( self ):
        self.pack_and_publish( "pr", 7 )
        self.assertEqual( self.fetch( channels=[ "main" ] ), cp.EXIT_BUILD_FROM_SOURCE )
        self.assertEqual( self.fetch(), 0 )

    def test_channels_from_the_environment( self ):
        self.pack_and_publish( "pr", 7 )
        args = [ "fetch", "--platform", "linux-amd64", "--product", "server",
                 "--core", str( self.core ), "--dest", str( self.tmp / "out" ) ]
        for channels, expected in ( ( "main", cp.EXIT_BUILD_FROM_SOURCE ), ( "", 0 ) ):
            with mock.patch.dict( os.environ, { "EO_CORE_CHANNELS": channels } ), \
                 mock.patch.object( cp, "make_backend", return_value=self.backend ), \
                 mock.patch( "sys.stderr", new_callable=io.StringIO ):
                self.assertEqual( cp.main( args ), expected, channels )

    def pending( self, running=True, packages=( "linux-amd64/server", ), channels=cp.CHANNELS ):
        with mock.patch.object( cp, "_ci_running", return_value=running ), \
             mock.patch( "sys.stdout", new_callable=io.StringIO ):
            return cp.pending( self.backend, self.core, packages, channels )

    def test_pending_only_with_a_package_missing_and_ci_running( self ):
        self.assertTrue( self.pending() )
        self.assertFalse( self.pending( running=False ) )
        self.pack_and_publish()
        self.assertFalse( self.pending() )
        self.assertTrue( self.pending( packages=( "linux-amd64/server", "linux-arm64/server" ) ) )

    def test_pending_for_a_release_ignores_pr_packages( self ):
        self.pack_and_publish( "pr", 7 )
        self.assertTrue( self.pending( channels=[ "main" ] ) )
        self.assertFalse( self.pending() )

    def test_pending_not_with_the_cache_off( self ):
        with mock.patch.dict( os.environ, { "EO_CORE_CACHE": "off" } ):
            self.assertFalse( self.pending() )

    def test_pending_errors_mean_not_waiting( self ):
        output = self.tmp / "output"
        args = [ "pending", "--core", str( self.core ), "linux-amd64/server" ]
        with mock.patch.dict( os.environ, { "GITHUB_OUTPUT": str( output ) } ), \
             mock.patch.object( cp, "make_backend", return_value=self.backend ), \
             mock.patch.object( cp, "_ci_running", side_effect=OSError( "no network" ) ), \
             mock.patch( "sys.stderr", new_callable=io.StringIO ):
            self.assertEqual( cp.main( args ), 0 )
        self.assertEqual( output.read_text(), "pending=false\n" )

    def test_ci_running_looks_only_at_build_workflows( self ):
        def running( *runs ):
            body = json.dumps( { "workflow_runs": [ { "path": p, "status": s } for p, s in runs ] } )
            with mock.patch( "urllib.request.urlopen", return_value=io.BytesIO( body.encode() ) ):
                return cp._ci_running( "abc" )
        self.assertTrue( running( ( ".github/workflows/build-windows.yml", "in_progress" ) ) )
        self.assertTrue( running( ( ".github/workflows/build.yml", "queued" ) ) )
        self.assertFalse( running( ( ".github/workflows/build.yml", "completed" ) ) )
        self.assertFalse( running( ( ".github/workflows/codeql.yaml", "in_progress" ) ) )
        self.assertTrue( running( ( ".github/workflows/build-wasm.yml", "in_progress" ) ) )
        self.assertFalse( running() )

    def test_corrupt_archive_is_not_used( self ):
        self.pack_and_publish()
        key = f"core/main/linux-amd64/server/{ self.tree() }.tar.gz"
        self.backend.files[ key ] = b"garbage"
        self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )

    def test_installed_package_of_another_platform_is_not_reused( self ):
        self.pack_and_publish()
        self.assertEqual( self.fetch(), 0 )
        manifest = cp.pack( self.output, self.tmp / "dist", "linux-arm64", "server", self.core )
        cp.publish( self.backend, [ manifest ], "main" )
        self.assertEqual( self.fetch( platform="linux-arm64" ), 0 )
        marker = json.loads( ( self.tmp / "out" / cp.MARKER ).read_text() )
        self.assertEqual( marker[ "platform" ], "linux-arm64" )
        # nothing for that product: the amd64 files must not stay behind
        self.assertEqual( self.fetch( platform="linux-arm64", product="desktop" ), cp.EXIT_BUILD_FROM_SOURCE )
        self.assertFalse( ( self.tmp / "out" ).exists() )

    def test_installed_pr_package_is_not_reused_for_a_release( self ):
        self.pack_and_publish( "pr", 7 )
        self.assertEqual( self.fetch(), 0 )
        self.assertEqual( self.fetch( channels=[ "main" ] ), cp.EXIT_BUILD_FROM_SOURCE )
        self.assertFalse( ( self.tmp / "out" ).exists() )

    def test_malformed_manifests_build_from_source( self ):
        self.pack_and_publish()
        good = json.loads( self.backend.files[ self.key( "json" ) ] )
        without_size = { k: v for k, v in good.items() if k != "size" }
        for text in ( "not json", "[]", json.dumps( without_size ), json.dumps( dict( good, sha256=None ) ),
                      json.dumps( dict( good, platform="linux-arm64" ) ) ):
            self.backend.files[ self.key( "json" ) ] = text.encode()
            self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE, text )

    def test_unsafe_archive_builds_from_source( self ):
        manifest = self.pack_and_publish()
        evil = self.tmp / "evil.tar.gz"
        with tarfile.open( evil, "w:gz" ) as tar:
            link = tarfile.TarInfo( "bin/escape" )
            link.type = tarfile.SYMTYPE
            link.linkname = "/etc/passwd"
            tar.addfile( link )
        self.backend.files[ self.key( "tar.gz" ) ] = evil.read_bytes()
        m = json.loads( manifest.read_text() )
        m[ "sha256" ] = cp._sha256( evil )
        self.backend.files[ self.key( "json" ) ] = json.dumps( m ).encode()
        self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )

    def test_outage_builds_from_source( self ):
        self.pack_and_publish()
        with mock.patch.object( self.backend, "read", side_effect=remote_cache.RemoteError( "HTTP 503" ) ):
            self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )

    def test_foreign_dest_is_not_replaced( self ):
        ( self.tmp / "out" ).mkdir()
        ( self.tmp / "out" / "mine" ).write_text( "x" )
        with self.assertRaises( cp.Error ):
            self.fetch()
        self.assertTrue( ( self.tmp / "out" / "mine" ).exists() )

    def test_published_packages_are_not_replaced( self ):
        self.pack_and_publish()
        key = f"core/main/linux-amd64/server/{ self.tree() }.tar.gz"
        self.backend.files[ key ] = b"first"
        self.pack_and_publish()
        self.assertEqual( self.backend.files[ key ], b"first" )

    def test_leftover_archive_is_replaced( self ):
        self.pack_and_publish()
        del self.backend.files[ self.key( "json" ) ]
        self.backend.files[ self.key( "tar.gz" ) ] = b"from an interrupted publish"
        self.pack_and_publish()
        self.assertEqual( self.fetch(), 0 )

    def test_interrupted_pr_publish_can_be_pruned( self ):
        real_put = self.backend.put
        def put( src, path ):
            if path.endswith( ".json" ):
                return False
            return real_put( src, path )
        with mock.patch.object( self.backend, "put", side_effect=put ):
            with self.assertRaises( cp.Error ):
                self.pack_and_publish( "pr", 7 )
        self.assertIn( self.key( "tar.gz", "pr" ), self.backend.files )
        cp.prune_pr( self.backend, 7 )
        self.assertEqual( self.backend.files, {} )

    def test_new_pr_commit_keeps_a_package_another_pr_uses( self ):
        self.pack_and_publish( "pr", 7 )
        self.pack_and_publish( "pr", 8 )
        shared = self.tree()
        self.next_commit( "int d;\n" )
        self.pack_and_publish( "pr", 7 )
        self.assertIn( self.key( "tar.gz", "pr", shared ), self.backend.files )
        self.assertIn( self.key( "tar.gz", "pr" ), self.backend.files )

    def test_prune_keeps_a_package_another_pr_uses( self ):
        self.pack_and_publish( "pr", 7 )
        self.pack_and_publish( "pr", 8 )
        cp.prune_pr( self.backend, 7 )
        self.assertIn( self.key( "tar.gz", "pr" ), self.backend.files )
        self.assertNotIn( "core/pr-heads/7/linux-amd64--server", self.backend.files )
        cp.prune_pr( self.backend, 8 )
        self.assertEqual( self.backend.files, {} )

    def test_shared_package_of_another_platform_is_not_kept( self ):
        self.pack_and_publish( "pr", 7 )
        self.backend.files[ "core/pr-heads/8/linux-arm64--server" ] = self.tree().encode()
        cp.prune_pr( self.backend, 7 )
        self.assertNotIn( self.key( "tar.gz", "pr" ), self.backend.files )

    def test_failed_delete_keeps_the_head_for_a_retry( self ):
        self.pack_and_publish( "pr", 7 )
        self.backend.undeletable.add( self.key( "tar.gz", "pr" ) )
        with self.assertRaises( cp.Error ):
            cp.prune_pr( self.backend, 7 )
        self.assertIn( "core/pr-heads/7/linux-amd64--server", self.backend.files )
        self.backend.undeletable.clear()
        cp.prune_pr( self.backend, 7 )
        self.assertEqual( self.backend.files, {} )

    def test_failed_delete_of_the_previous_package_fails_publish( self ):
        self.pack_and_publish( "pr", 7 )
        old = self.tree()
        self.backend.undeletable.add( self.key( "tar.gz", "pr" ) )
        self.next_commit( "int d;\n" )
        with self.assertRaises( cp.Error ):
            self.pack_and_publish( "pr", 7 )
        self.assertEqual( self.backend.files[ "core/pr-heads/7/linux-amd64--server" ].decode(), old )

    def test_failed_listing_fails_prune( self ):
        self.pack_and_publish( "pr", 7 )
        with mock.patch.object( self.backend, "keys", side_effect=remote_cache.RemoteError( "HTTP 403" ) ):
            with self.assertRaises( remote_cache.RemoteError ):
                cp.prune_pr( self.backend, 7 )
        self.assertIn( self.key( "tar.gz", "pr" ), self.backend.files )

    def test_failed_head_read_fails_publish( self ):
        self.pack_and_publish( "pr", 7 )
        self.next_commit( "int d;\n" )
        with mock.patch.object( self.backend, "read", side_effect=remote_cache.RemoteError( "HTTP 503" ) ):
            with self.assertRaises( remote_cache.RemoteError ):
                self.pack_and_publish( "pr", 7 )

    def test_cli_reports_remote_errors( self ):
        with mock.patch.object( cp, "make_backend", return_value=self.backend ), \
             mock.patch.object( self.backend, "keys", side_effect=remote_cache.RemoteError( "HTTP 403" ) ), \
             mock.patch( "sys.stderr", new_callable=io.StringIO ):
            self.assertEqual( cp.main( [ "prune-pr", "--pr", "7" ] ), 1 )

    def test_new_pr_commit_deletes_previous_package( self ):
        self.pack_and_publish( "pr", 7 )
        old = self.tree()
        ( self.core / "a.cpp" ).write_text( "int d;\n" )
        self.commit( "next" )
        self.pack_and_publish( "pr", 7 )
        trees = { k.rsplit( "/", 1 )[ -1 ].split( "." )[ 0 ]
                  for k in self.backend.files if k.startswith( "core/pr/" ) }
        self.assertEqual( trees, { self.tree() } )
        self.assertNotEqual( old, self.tree() )
        self.assertEqual( self.backend.files[ "core/pr-heads/7/linux-amd64--server" ].decode(), self.tree() )

    def test_prune_pr( self ):
        self.pack_and_publish( "pr", 7 )
        self.pack_and_publish( "main" )
        cp.prune_pr( self.backend, 7 )
        self.assertFalse( [ k for k in self.backend.files if k.startswith( ( "core/pr/", "core/pr-heads/" ) ) ] )
        self.assertTrue( [ k for k in self.backend.files if k.startswith( "core/main/" ) ] )

    def test_prune_pr_can_keep_the_head_package( self ):
        self.pack_and_publish( "pr", 7 )
        kept = self.tree()
        cp.prune_pr( self.backend, 7, keep_tree=kept )
        self.assertIn( self.key( "json", "pr", kept ), self.backend.files )
        self.assertIn( "core/pr-heads/7/linux-amd64--server", self.backend.files )

    def test_behind_base_commit_drops_the_older_package( self ):
        self.pack_and_publish( "pr", 7 )
        old = self.tree()
        self.next_commit( "int e;\n" )
        cp.prune_pr( self.backend, 7, keep_tree=self.tree() )
        self.assertNotIn( self.key( "json", "pr", old ), self.backend.files )
        self.assertNotIn( "core/pr-heads/7/linux-amd64--server", self.backend.files )

    def test_missing_git_builds_from_source( self ):
        self.pack_and_publish()
        with mock.patch( "subprocess.run", side_effect=FileNotFoundError( "git" ) ):
            self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )

    def test_failed_install_builds_from_source( self ):
        self.pack_and_publish()
        with mock.patch.object( Path, "rename", side_effect=PermissionError( "busy" ) ):
            self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )
            with self.assertRaises( cp.Error ):
                self.fetch( mode="require" )

    def test_failed_cleanup_builds_from_source( self ):
        self.pack_and_publish()
        self.assertEqual( self.fetch(), 0 )
        ( self.core / "a.cpp" ).write_text( "int f;\n" )
        with mock.patch( "shutil.rmtree", side_effect=PermissionError( "busy" ) ):
            self.assertEqual( self.fetch(), cp.EXIT_BUILD_FROM_SOURCE )

    def test_bad_cache_url_builds_from_source( self ):
        args = [ "fetch", "--platform", "linux-amd64", "--product", "server",
                 "--core", str( self.core ), "--dest", str( self.tmp / "out" ) ]
        for mode, expected in ( ( "off", cp.EXIT_BUILD_FROM_SOURCE ), ( "auto", cp.EXIT_BUILD_FROM_SOURCE ),
                                ( "require", 1 ) ):
            env = { "EO_CORE_CACHE": mode, "EO_CORE_CACHE_S3_URL": "not a url" }
            with mock.patch.dict( os.environ, env ), mock.patch( "sys.stderr", new_callable=io.StringIO ):
                self.assertEqual( cp.main( args ), expected, mode )

    def test_cli_reports_os_errors( self ):
        ( self.tmp / "out" ).write_text( "a file" )
        with mock.patch.object( cp, "make_backend", return_value=self.backend ), \
             mock.patch( "sys.stderr", new_callable=io.StringIO ):
            self.assertEqual( cp.main( [ "fetch", "--platform", "linux-amd64", "--product", "server",
                                         "--core", str( self.core ), "--dest", str( self.tmp / "out" ) ] ), 1 )

    def test_publish_needs_credentials( self ):
        manifest = cp.pack( self.output, self.tmp / "dist", "linux-amd64", "server", self.core )
        with self.assertRaises( cp.Error ):
            cp.publish( FakeBackend( can_write=False ), [ manifest ], "main" )

    def test_pr_number_only_for_pr_channel( self ):
        manifest = cp.pack( self.output, self.tmp / "dist", "linux-amd64", "server", self.core )
        for channel, pr in ( ( "pr", None ), ( "main", 7 ) ):
            with self.assertRaises( cp.Error ):
                cp.publish( self.backend, [ manifest ], channel, pr )


class S3BackendTest( unittest.TestCase ):
    def setUp( self ):
        self.backend = remote_cache.S3Backend( "https://s3.example/bucket" )
        self.requests = []

    def respond( self, *responses ):
        responses = iter( responses )
        def urlopen( req, timeout=None ):
            self.requests.append( req.full_url )
            response = next( responses )
            if isinstance( response, Exception ):
                raise response
            return io.BytesIO( response )
        return mock.patch( "urllib.request.urlopen", side_effect=urlopen )

    def http_error( self, code ):
        return HTTPError( "https://s3.example", code, "nope", {}, None )

    def test_keys_follow_the_continuation_token( self ):
        pages = ( b"<Key>d/a</Key><NextContinuationToken>t&amp;1</NextContinuationToken>", b"<Key>d/b</Key>" )
        with self.respond( *pages ):
            self.assertEqual( self.backend.keys( "d" ), [ "d/a", "d/b" ] )
        self.assertIn( "continuation-token=t%261", self.requests[ 1 ] )

    def test_keys_raise_on_failure( self ):
        for error in ( self.http_error( 403 ), self.http_error( 404 ), URLError( "down" ) ):
            with self.respond( error ), self.assertRaises( remote_cache.RemoteError ):
                self.backend.keys( "d" )

    def test_read_tells_missing_from_failed( self ):
        with self.respond( b"tree" ):
            self.assertEqual( self.backend.read( "k" ), "tree" )
        with self.respond( self.http_error( 404 ) ):
            self.assertIsNone( self.backend.read( "k" ) )
        with self.respond( self.http_error( 503 ) ), self.assertRaises( remote_cache.RemoteError ):
            self.backend.read( "k" )

    def test_exists_stays_lenient( self ):
        with self.respond( self.http_error( 503 ) ), mock.patch( "sys.stdout", new_callable=io.StringIO ):
            self.assertFalse( self.backend.exists( "k" ) )


if __name__ == "__main__":
    unittest.main()
