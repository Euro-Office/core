# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Core package cache tests. Run: python3 -m unittest discover -s scripts

import io
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import core_package as cp


class FakeBackend:
    def __init__( self, can_write=True ):
        self.files = {}
        self.can_write = can_write

    def exists( self, path ):
        return path in self.files

    def keys( self, dir ):
        return sorted( k for k in self.files if k.startswith( dir + "/" ) )

    def get( self, path, dest_file ):
        if path not in self.files:
            return False
        Path( dest_file ).write_bytes( self.files[ path ] )
        return True

    def put( self, src_file, path ):
        self.files[ path ] = Path( src_file ).read_bytes()
        return True

    def delete( self, path ):
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

    def fetch( self, **kwargs ):
        kwargs.setdefault( "mode", "auto" )
        return cp.fetch( self.backend, self.core, "linux-amd64", "server", self.tmp / "out", **kwargs )

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

    def test_corrupt_archive_is_not_used( self ):
        self.pack_and_publish()
        key = f"core/main/linux-amd64/server/{ self.tree() }.tar.gz"
        self.backend.files[ key ] = b"garbage"
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

    def test_publish_needs_credentials( self ):
        manifest = cp.pack( self.output, self.tmp / "dist", "linux-amd64", "server", self.core )
        with self.assertRaises( cp.Error ):
            cp.publish( FakeBackend( can_write=False ), [ manifest ], "main" )

    def test_pr_number_only_for_pr_channel( self ):
        manifest = cp.pack( self.output, self.tmp / "dist", "linux-amd64", "server", self.core )
        for channel, pr in ( ( "pr", None ), ( "main", 7 ) ):
            with self.assertRaises( cp.Error ):
                cp.publish( self.backend, [ manifest ], channel, pr )


if __name__ == "__main__":
    unittest.main()
