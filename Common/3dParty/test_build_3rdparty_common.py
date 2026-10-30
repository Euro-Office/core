# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Remote cache tests. Run: python3 -m unittest discover -s Common/3dParty

import datetime
import io
import tempfile
import unittest
from pathlib import Path
from unittest import mock
from urllib.error import HTTPError

import build_3rdparty_common as nc


class FakeBackend:
    def __init__( self, files=() ):
        self.files = set( files )
        self.puts = []

    def exists( self, path ):
        return path in self.files

    def list( self, dir ):
        return "".join( f"<Key>{ f }</Key>" for f in self.files if f.startswith( dir + "/" ) )

    def put( self, src_file, path ):
        self.puts.append( path )
        return True


class RemoteCacheTest( unittest.TestCase ):
    def setUp( self ):
        patches = {
            "install_dir": Path( "/nonexistent/v8" ),
            "dep_version": "3",
            "BASE_REMOTE_PATH": "3rdparty",
            "PLATFORM_TAG": "linux-x86_64",
            "LOCAL_GLIBC": ( 2, 35 ),
        }
        for name, value in patches.items():
            p = mock.patch.object( nc, name, value )
            p.start()
            self.addCleanup( p.stop )

    def use_backend( self, backend ):
        p = mock.patch.object( nc, "_backend_instance", backend )
        p.start()
        self.addCleanup( p.stop )

    def test_region_from_url( self ):
        self.assertEqual( nc._s3_region( "https://s3.eu-central-4.ionoscloud.com/eo-build-assets" ), "eu-central-4" )
        self.assertEqual( nc._s3_region( "https://minio.example.com/bucket" ), "us-east-1" )
        self.assertEqual( nc._s3_region( "https://s3.example.com/bucket" ), "us-east-1" )

    def test_url_needs_endpoint_and_bucket( self ):
        self.assertTrue( nc._valid_s3_url( "https://s3.eu-central-4.ionoscloud.com/eo-build-assets" ) )
        for url in ( "s3.example.com/bucket", "https://s3.example.com", "https://h/b/c", "https://h/b?x=1" ):
            self.assertFalse( nc._valid_s3_url( url ), url )

    def test_key_contains_version_and_glibc( self ):
        self.assertEqual( nc._remote_file_path( ( 2, 34 ) ),
                          "3rdparty/linux-x86_64/v8/3/v8-glibc2.34.tar.bz2" )

    def test_picks_newest_compatible_glibc( self ):
        d = "3rdparty/linux-x86_64/v8/3"
        self.use_backend( FakeBackend( [ f"{ d }/v8-glibc2.28.tar.bz2",
                                         f"{ d }/v8-glibc2.34.tar.bz2",
                                         f"{ d }/v8-glibc2.39.tar.bz2",
                                         "3rdparty/linux-x86_64/v8/2/v8-glibc2.35.tar.bz2" ] ) )
        self.assertEqual( nc._remote_find_archive(), f"{ d }/v8-glibc2.34.tar.bz2" )

    def test_existing_archive_is_not_replaced( self ):
        backend = FakeBackend( [ "3rdparty/linux-x86_64/v8/3/v8-glibc2.35.tar.bz2" ] )
        self.use_backend( backend )
        with mock.patch.object( nc, "USE_REMOTE_CACHE", True ), \
             mock.patch.object( nc, "REMOTE_CACHE_READ_ONLY", False ), \
             mock.patch.object( nc, "_remote_find_archive", return_value=None ), \
             mock.patch.object( nc, "install_dir_looks_ok", side_effect=[ False, True ] ), \
             mock.patch.object( nc, "_remote_upload", side_effect=AssertionError( "uploaded" ) ):
            nc.ensure_dep( lambda: None, forceredo=False )

    def test_broken_archive_builds_locally( self ):
        built = []
        backend = FakeBackend()
        backend.get = lambda path, dest: Path( dest ).write_bytes( b"not a tarball" ) or True
        self.use_backend( backend )
        with tempfile.TemporaryDirectory() as tmp, \
             mock.patch.object( nc, "install_dir", Path( tmp ) / "v8" ), \
             mock.patch.object( nc, "USE_REMOTE_CACHE", True ), \
             mock.patch.object( nc, "REMOTE_CACHE_READ_ONLY", True ), \
             mock.patch.object( nc, "_remote_find_archive", return_value="a/v8.tar.bz2" ), \
             mock.patch.object( nc, "install_dir_looks_ok", side_effect=[ False, True ] ):
            nc.ensure_dep( lambda: built.append( True ), forceredo=False )
        self.assertEqual( built, [ True ] )

    def test_http_errors_other_than_404_are_reported( self ):
        backend = nc._S3Backend()
        req = backend._request( "HEAD", "x" )
        for code, warns in ( ( 404, False ), ( 403, True ) ):
            err = HTTPError( req.full_url, code, "msg", {}, None )
            with mock.patch( "urllib.request.urlopen", side_effect=err ), \
                 mock.patch( "sys.stdout", new_callable=io.StringIO ) as out:
                self.assertIsNone( backend._open( req ) )
            self.assertEqual( "[WARN]" in out.getvalue(), warns, code )

    def test_truncated_download_fails( self ):
        response = io.BytesIO( b"short" )
        response.headers = { "Content-Length": "100" }
        backend = nc._S3Backend()
        with mock.patch.object( backend, "_open", return_value=response ), \
             mock.patch( "builtins.open", mock.mock_open() ) as m:
            m.return_value.tell.return_value = 5
            self.assertFalse( backend.get( "x", "dest" ) )

    def test_sigv4_matches_aws_example( self ):
        # "GET Bucket (List Objects)" example from the AWS docs:
        # https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
        fixed = datetime.datetime( 2013, 5, 24, tzinfo=datetime.timezone.utc )
        with mock.patch.object( nc, "S3_URL", "https://examplebucket.s3.amazonaws.com" ), \
             mock.patch.object( nc, "S3_REGION", "us-east-1" ), \
             mock.patch.object( nc, "S3_ACCESS_KEY", "AKIAIOSFODNN7EXAMPLE" ), \
             mock.patch.object( nc, "S3_SECRET_KEY", "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY" ), \
             mock.patch.object( nc, "S3_SESSION_TOKEN", "" ), \
             mock.patch( "datetime.datetime", wraps=datetime.datetime ) as dt:
            dt.now.return_value = fixed
            req = nc._S3Backend()._request( "GET", query={ "max-keys": "2", "prefix": "J" } )
        self.assertTrue( req.get_header( "Authorization" ).endswith(
            "Signature=34b48302e7b5fa45bde8084f4b7868a86f0a534bc59db6670ed5711ef69dc6f7" ) )


if __name__ == "__main__":
    unittest.main()
