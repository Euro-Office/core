# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Storage backends for the build caches: the third-party cache
# (build_3rdparty_common.py) and the core package cache (core_package.py).
#
# Each backend takes paths relative to the cache root and implements:
#   exists( path )         -> bool
#   list( dir )            -> str        (raw listing)
#   keys( dir )            -> list[str]  (paths below dir, S3 only)
#   get( path, dest_file ) -> bool
#   put( src_file, path )  -> bool
#   delete( path )         -> bool

import http.client
import os
import re
import shutil
import subprocess

DEFAULT_S3_URL = "https://s3.eu-central-4.ionoscloud.com/eo-build-assets"
DEFAULT_NEXTCLOUD_REMOTE = "https://cloud.nextcloud.com/remote.php/dav/files"


def env( *names, default="" ):
    # First non-empty variable out of names.
    for nm in names:
        value = os.environ.get( nm, "" ).strip()
        if value:
            return value
    return default


def s3_region( url ):
    host = url.split( "://", 1 )[ -1 ].split( "/", 1 )[ 0 ]
    m = re.match( r"s3[.-]([a-z]{2}(?:-[a-z]+)+-\d+)\.", host )
    return m.group( 1 ) if m else "us-east-1"


def valid_s3_url( url ):
    return re.fullmatch( r"https?://[^/?#]+/[^/?#]+", url ) is not None


class WebDavBackend:
    # 'curl' must be on PATH.

    def __init__( self, remote, user="", password="" ):
        self.root     = f"{ remote }/{ user }" if user else remote
        self.user     = user
        self.password = password

    def _url( self, path ):
        return f"{ self.root }/{ path }"

    def _curl( self, *args ):
        auth = ( [ "-u", f"{ self.user }:{ self.password }" ]
                 if self.user and self.password else [] )
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

    def keys( self, dir ):
        raise NotImplementedError( "listing keys needs the S3 backend" )

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

    def delete( self, path ):
        return self._curl( "-f", "-X", "DELETE", self._url( path ) ).returncode == 0


class S3Backend:
    # Plain urllib + AWS Signature V4, so no boto3 / aws cli is needed.
    # Path-style requests against <endpoint>/<bucket>. Without credentials,
    # requests are sent unsigned (enough to read a public bucket).

    _EMPTY_SHA256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"

    def __init__( self, url, region="", access_key="", secret_key="", session_token="" ):
        from urllib.parse import urlsplit
        parts = urlsplit( url )
        self.scheme        = parts.scheme
        self.host          = parts.netloc
        self.prefix        = parts.path
        self.region        = region or s3_region( url )
        self.access_key    = access_key
        self.secret_key    = secret_key
        self.session_token = session_token

    @property
    def can_write( self ):
        return bool( self.access_key and self.secret_key )

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

        if self.can_write:
            now   = datetime.datetime.now( datetime.timezone.utc )
            stamp = now.strftime( "%Y%m%dT%H%M%SZ" )
            day   = stamp[ :8 ]
            headers[ "x-amz-date" ] = stamp
            if self.session_token:
                headers[ "x-amz-security-token" ] = self.session_token

            signed    = ";".join( sorted( headers ) )
            canonical = "\n".join( [
                method, uri, qs,
                "".join( f"{ k }:{ headers[ k ].strip() }\n" for k in sorted( headers ) ),
                signed, payload_hash,
            ] )
            scope = f"{ day }/{ self.region }/s3/aws4_request"
            to_sign = "\n".join( [ "AWS4-HMAC-SHA256", stamp, scope,
                                   hashlib.sha256( canonical.encode() ).hexdigest() ] )

            def _hmac( k, msg ):
                return hmac.new( k, msg.encode(), hashlib.sha256 ).digest()
            k = _hmac( f"AWS4{ self.secret_key }".encode(), day )
            for part in ( self.region, "s3", "aws4_request" ):
                k = _hmac( k, part )
            signature = hmac.new( k, to_sign.encode(), hashlib.sha256 ).hexdigest()
            headers[ "authorization" ] = ( f"AWS4-HMAC-SHA256 Credential={ self.access_key }/{ scope }, "
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

    def keys( self, dir ):
        # Only the first 1000 keys; the folders this is used on are small.
        return re.findall( r"<Key>([^<]+)</Key>", self.list( dir ) )

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

    def delete( self, path ):
        # S3 answers 204 whether or not the object existed.
        r = self._open( self._request( "DELETE", path ) )
        if r is None:
            return False
        r.close()
        return True
