#!/usr/bin/env python3
import sys
import os
import hashlib
import shlex
import shutil
import tarfile
import time
import urllib.request
from pathlib import Path

script_path = Path( sys.argv[0] ).resolve()
script_dir = script_path.parent

# ---------------------------------------------------------------------------
# Qt is built from the official source tarballs on Linux and Windows, so both
# get the same Qt version and modules. Qt 6.12 is an LTS release.
#
# Linux: Qt's prebuilt binaries need glibc 2.34 (amd64) or 2.38 (arm64), newer
# than the oldest distro we support (Rocky/RHEL 8, glibc 2.28).
# Windows: aqtinstall can't install Qt from 6.11 on (aqtinstall issue #1047).
# macOS doesn't use Qt (the editors are native Cocoa).
# ---------------------------------------------------------------------------
qt_version = "6.12.0"

# The Qt submodules, in build order, with the SHA-256 that download.qt.io
# publishes next to each tarball. qtshadertools is a build dependency of
# qtmultimedia. The Wayland client and platform plugin are part of qtbase
# since 6.12, so qtwayland (now mostly the compositor) isn't needed.
qt_source_url = f"https://download.qt.io/official_releases/qt/{ '.'.join( qt_version.split( '.' )[ :2 ] ) }/{ qt_version }/submodules"
qt_source_modules = {
    "qtbase":        "a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307",
    "qtshadertools": "c7d84f436e1aaef39fdcadebf2bd71bdf24dc69497e13f2e0198873cbbfea2ab",
    "qtsvg":         "e4ab39534ec97987b1b9b60ef7f4d3253d5912a69f8c713a01753174a4029331",
    "qttools":       "8dab8f3611496486a470ad5f115ceea584f36bc22a2b8b6f6ebdbafbb8160693",
    "qtmultimedia":  "3143f53b64257ba2a0685c940f5dce476c75871ff86b516de45cfd54ea93a62d",
}

# FFmpeg for Qt Multimedia's FFmpeg backend, the version Qt 6.12's own CI
# builds (qt5.git coin/provisioning/common/unix/ffmpeg-installation-utils.sh).
ffmpeg_version = "9.0.1"
ffmpeg_sha256 = "cf38e0e28c7e5605942c4a77755349b0145804a397af37eb1fb4c77cb237f635"

# Windows: FFmpeg's configure needs a POSIX shell and make. As in Qt's CI
# (coin/provisioning/common/windows/install-ffmpeg.ps1) that is MSYS2, with
# MSVC as the compiler. An existing MSYS2 is used (MSYS2_ROOT, or C:\msys64 as
# on GitHub's x64 runners), else this base archive is unpacked into the work
# dir (signature checked against the MSYS2 maintainer key when it was pinned).
msys2_version = "20260927"
msys2_sha256 = "ea2f31a0b6ade63914ce441ffb022f0f6aa96982bfefa2326460a26d5fb01322"
msys2_packages = [ "make", "diffutils", "nasm" ]

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )

import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "Qt",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "3"
)


def _jobs() -> int:
    # The CPUs this process may run on (a container's cpuset), not all of the host's
    try:
        return len( os.sched_getaffinity( 0 ) )
    except AttributeError:
        return os.cpu_count() or 1


def _download( url : str, dest : Path, sha256 : str ):
    if dest.exists() and hashlib.sha256( dest.read_bytes() ).hexdigest() == sha256:
        return
    for attempt in range( 1, 4 ):
        try:
            req = urllib.request.Request( url, headers = { "User-Agent": "eo-3rdparty" } )
            with urllib.request.urlopen( req ) as resp, open( dest, "wb" ) as out:
                shutil.copyfileobj( resp, out )
            break
        except Exception as e:
            if attempt == 3:
                nc.abort_op( f"Download of { url } failed: { e }" )
            print( f"Download of { url } failed ({ e }), retrying in 10 seconds..." )
            time.sleep( 10 )
    digest = hashlib.sha256( dest.read_bytes() ).hexdigest()
    if digest != sha256:
        dest.unlink()
        nc.abort_op( f"Checksum mismatch for { dest.name }: expected { sha256 }, got { digest }" )


def _extract( archive : Path, dest_parent : Path, name : str | None = None ) -> Path:
    # Each tarball holds a single top-level directory, by default named like the archive
    name = name or archive.name.removesuffix( ".tar.xz" )
    target = dest_parent / name
    if target.exists():
        shutil.rmtree( target )
    with tarfile.open( archive, "r:xz" ) as tar:
        tar.extractall( dest_parent, filter = "data" )
    if not target.is_dir():
        nc.abort_op( f"{ archive.name } did not unpack to { name }/" )
    return target


def source_prefix() -> Path:
    # The layout aqt used (<version>/<arch>), which common.cmake looks for
    if nc.is_windows():
        arch = "msvc2022_arm64" if nc.is_arm64() else "msvc2022_64"
    else:
        arch = "gcc_arm64" if nc.is_arm64() else "gcc_64"
    return nc.install_dir / qt_version / arch


def fetch_sources():
    downloads = nc.work_dir / "downloads"
    src = nc.work_dir / "src"
    nc.ensure_directory_exists( downloads )
    nc.ensure_directory_exists( src )

    ffmpeg_archive = downloads / f"ffmpeg-{ ffmpeg_version }.tar.xz"
    print( f"Downloading FFmpeg { ffmpeg_version }" )
    _download( f"https://ffmpeg.org/releases/{ ffmpeg_archive.name }", ffmpeg_archive, ffmpeg_sha256 )
    _extract( ffmpeg_archive, src )

    for module, sha256 in qt_source_modules.items():
        archive = downloads / f"{ module }-everywhere-src-{ qt_version }.tar.xz"
        print( f"Downloading { archive.name }" )
        _download( f"{ qt_source_url }/{ archive.name }", archive, sha256 )
        _extract( archive, src )


# ---------------------------------------------------------------------------
# FFmpeg
# ---------------------------------------------------------------------------

def _msys_path( path : Path ) -> str:
    # C:\a\b -> /c/a/b
    p = str( path.resolve() ).replace( "\\", "/" )
    if len( p ) > 1 and p[ 1 ] == ":":
        p = f"/{ p[ 0 ].lower() }{ p[ 2: ] }"
    return p


def _msys2_bash() -> Path:
    candidates = [ Path( os.environ[ "MSYS2_ROOT" ] ) ] if os.environ.get( "MSYS2_ROOT" ) else []
    candidates += [ Path( "C:/msys64" ), nc.work_dir / "msys64" ]
    root = next( ( c for c in candidates if ( c / "usr" / "bin" / "bash.exe" ).is_file() ), None )

    if root is None:
        archive = nc.work_dir / "downloads" / f"msys2-base-x86_64-{ msys2_version }.tar.xz"
        nc.ensure_directory_exists( archive.parent )
        print( f"Downloading MSYS2 { msys2_version }" )
        _download( f"https://repo.msys2.org/distrib/x86_64/{ archive.name }", archive, msys2_sha256 )
        root = _extract( archive, nc.work_dir, "msys64" )

    bash = root / "usr" / "bin" / "bash.exe"
    print( f"MSYS2: { root }" )
    # The first login shell of a fresh MSYS2 sets up its keyring
    _msys_run( bash, "true", "Initialize MSYS2", nc.work_dir )
    # Only what's missing, so an MSYS2 of the developer's isn't touched otherwise
    packages = " ".join( msys2_packages )
    _msys_run( bash, f"pacman -Q { packages } >/dev/null 2>&1 || pacman -Sy --needed --noconfirm { packages }",
               "Install MSYS2 packages", nc.work_dir )
    return bash


def _msys_run( bash : Path, command : str, description : str, cwd : Path ):
    nc.run_command( [ str( bash ), "-lc", command ], description, cwd, env = {
        "MSYSTEM": "MSYS",
        # Keep the MSVC environment (cl, link, INCLUDE, LIB) of the caller
        "MSYS2_PATH_TYPE": "inherit",
        # Stay in cwd instead of the login shell's home
        "CHERE_INVOKING": "1",
    } )


def build_ffmpeg( prefix : Path ):
    src = nc.work_dir / "src" / f"ffmpeg-{ ffmpeg_version }"
    build = nc.work_dir / "build" / "ffmpeg"
    if build.exists():
        shutil.rmtree( build )
    nc.ensure_directory_exists( build )

    # Qt's own option set (qt5.git coin/provisioning/common/shared/
    # ffmpeg_config_options.txt), shared. On top of it, nothing that would
    # make the libraries need more than the stock OS at runtime: no OpenSSL
    # (el8 has 1.1), no SDL; on Linux no VA-API/VDPAU/DRM/X11.
    options = [
        "--disable-programs", "--disable-doc", "--disable-debug", "--enable-network",
        "--disable-lzma", "--enable-pic", "--disable-vulkan", "--disable-v4l2-m2m",
        "--disable-decoder=truemotion1", "--disable-avdevice", "--disable-avfilter",
        "--enable-shared", "--disable-static",
        "--disable-openssl", "--disable-sdl2",
    ]

    if nc.is_windows():
        # The DLLs and their import libraries go to bin/, next to Qt's DLLs
        # (desktop-apps installs bin/av*.dll and sw*.dll). As in Qt's CI,
        # arm64 has no assembly (FFmpeg's needs gas-preprocessor with MSVC).
        # No D3D12 video acceleration: it needs a newer Windows SDK than x64's
        # 10.0.19041 (ID3D12Resource::GetDesc's C interface changed), and Qt
        # Multimedia decodes in hardware through D3D11 (d3d11va)
        options += [ "--toolchain=msvc", f"--prefix={ _msys_path( prefix ) }", "--disable-d3d12va" ]
        if nc.is_arm64():
            options += [ "--arch=arm64", "--disable-asm" ]
        # libswscale includes <stdalign.h>, which the UCRT only has from the
        # Windows SDK 10.0.20348 on: older SDKs get a stand-in (FFmpeg builds
        # as C11, where MSVC knows _Alignas and _Alignof)
        include_dirs = os.environ.get( "INCLUDE", "" ).split( os.pathsep )
        if not any( d and ( Path( d ) / "stdalign.h" ).exists() for d in include_dirs ):
            compat = build / "compat"
            nc.ensure_directory_exists( compat )
            ( compat / "stdalign.h" ).write_text(
                "#pragma once\n"
                "#define alignas _Alignas\n"
                "#define alignof _Alignof\n"
                "#define __alignas_is_defined 1\n"
                "#define __alignof_is_defined 1\n" )
            options += [ f"--extra-cflags=-I{ compat.as_posix() }" ]
        bash = _msys2_bash()
        configure = " ".join( shlex.quote( a ) for a in [ f"{ _msys_path( src ) }/configure" ] + options )
        _msys_run( bash, configure, "Configure FFmpeg", build )
        _msys_run( bash, f"make -j{ _jobs() }", "Build FFmpeg", build )
        _msys_run( bash, "make install", "Install FFmpeg", build )
        return

    # The rpath lets libavformat & co. find each other next to themselves;
    # the backslash and "$$" make "$ORIGIN" survive both make and the shell.
    options += [
        "--disable-vaapi", "--disable-vdpau", "--disable-libdrm",
        "--disable-xlib", "--disable-libxcb",
        f"--prefix={ prefix }",
        "--extra-ldflags=-Wl,-rpath,\\$$ORIGIN",
    ]
    nc.run_command( [ str( src / "configure" ) ] + options, "Configure FFmpeg", build )
    nc.run_command( [ "make", f"-j{ _jobs() }" ], "Build FFmpeg", build )
    nc.run_command( [ "make", "install" ], "Install FFmpeg", build )


# ---------------------------------------------------------------------------
# Qt
# ---------------------------------------------------------------------------

def _cmake_module( module : str, prefix : Path, args : list[ str ] ):
    src = nc.work_dir / "src" / f"{ module }-everywhere-src-{ qt_version }"
    build = nc.work_dir / "build" / module
    if build.exists():
        shutil.rmtree( build )
    nc.ensure_directory_exists( build )

    if module == "qtbase":
        configure = [ "cmake", "-S", str( src ), "-B", str( build ), "-G", "Ninja",
                      f"-DCMAKE_INSTALL_PREFIX={ prefix }" ]
    else:
        # qt-cmake points CMake at the Qt just installed into the prefix
        qt_cmake = prefix / "bin" / ( "qt-cmake.bat" if nc.is_windows() else "qt-cmake" )
        configure = [ str( qt_cmake ), "-S", str( src ), "-B", str( build ), "-G", "Ninja" ]

    configure += [
        "-DCMAKE_BUILD_TYPE=Release",
        "-DQT_BUILD_EXAMPLES=OFF",
        "-DQT_BUILD_TESTS=OFF",
        "-DQT_GENERATE_SBOM=OFF",
    ]
    # Only when there are any: an empty CMAKE_<LANG>_FLAGS would drop CMake's
    # defaults (/DWIN32 /D_WINDOWS /EHsc with MSVC)
    flags = " ".join( nc.baseline_arch_flags() )
    if flags:
        configure += [ f"-DCMAKE_C_FLAGS={ flags }", f"-DCMAKE_CXX_FLAGS={ flags }" ]
    configure += args

    nc.run_command( configure, f"Configure { module }", build )
    nc.run_command( [ "cmake", "--build", ".", "--parallel", str( _jobs() ) ], f"Build { module }", build )
    nc.run_command( [ "cmake", "--install", "." ], f"Install { module }", build )


def _patch_source( module : str, relative : str, old : str, new : str ):
    # Idempotent: the work dir with the unpacked sources is reused across builds
    path = nc.work_dir / "src" / f"{ module }-everywhere-src-{ qt_version }" / relative
    text = path.read_text()
    if new in text:
        return
    if old not in text:
        nc.abort_op( f"Can't patch { path }: expected text not found" )
    path.write_text( text.replace( old, new, 1 ) )


def _openssl3_args() -> list[ str ]:
    # Qt loads OpenSSL at runtime (libssl.so.3) and only needs its headers to
    # build. On el8 they come from EPEL's openssl3-devel, which installs
    # beside the system's OpenSSL 1.1.
    include = Path( "/usr/include/openssl3" )
    if include.is_dir():
        return [ f"-DOPENSSL_INCLUDE_DIR={ include }",
                 "-DOPENSSL_CRYPTO_LIBRARY=/usr/lib64/openssl3/libcrypto.so",
                 "-DOPENSSL_SSL_LIBRARY=/usr/lib64/openssl3/libssl.so" ]
    return []


def _qtbase_args() -> list[ str ]:
    # Bundled harfbuzz, libpng, libjpeg, pcre2, zlib, double-conversion and
    # md4c keep the runtime dependencies to what every desktop system has.
    # No ICU: core ships its own ICU 74, and Qt doesn't need one.
    args = [
        "-DBUILD_SHARED_LIBS=ON",
        "-DFEATURE_icu=OFF",
        "-DFEATURE_system_harfbuzz=OFF",
        "-DFEATURE_system_png=OFF",
        "-DFEATURE_system_jpeg=OFF",
        "-DFEATURE_system_pcre2=OFF",
        "-DFEATURE_system_zlib=OFF",
        "-DFEATURE_system_doubleconversion=OFF",
        "-DFEATURE_system_libb2=OFF",
    ]
    if nc.is_windows():
        # TLS through Schannel only: the app doesn't ship OpenSSL DLLs for Qt
        # to load, so Qt's OpenSSL backend could never be used
        return args + [ "-DINPUT_openssl=no" ]
    return args + [
        "-DFEATURE_system_freetype=ON",
        "-DFEATURE_fontconfig=ON",
        "-DFEATURE_xcb=ON",
        "-DFEATURE_xcb_xlib=ON",
        "-DFEATURE_xkbcommon=ON",
        "-DFEATURE_wayland=ON",
        "-DFEATURE_gtk3=ON",
        "-DFEATURE_dbus=ON",
        "-DFEATURE_cups=ON",
        "-DFEATURE_glib=ON",
        "-DFEATURE_opengl=ON",
        "-DFEATURE_egl=ON",
        "-DINPUT_openssl=runtime",
    ] + _openssl3_args()


def build_from_source():
    prefix = source_prefix()
    if prefix.exists():
        shutil.rmtree( prefix )
    nc.ensure_directory_exists( prefix )
    print( f"Building Qt { qt_version } from source into { prefix }" )

    # FFmpeg goes into the Qt prefix, so its libraries end up next to Qt's
    # (lib/ on Linux, bin/ on Windows), as in aqt's layout
    build_ffmpeg( prefix )

    _cmake_module( "qtbase", prefix, _qtbase_args() )
    _cmake_module( "qtshadertools", prefix, [] )
    _cmake_module( "qtsvg", prefix, [] )

    # Only the Linguist tools (lrelease, lupdate, lconvert); no Designer,
    # Assistant or QDoc (which would need libclang)
    _cmake_module( "qttools", prefix, [
        "-DFEATURE_linguist=ON",
        "-DFEATURE_designer=OFF",
        "-DFEATURE_assistant=OFF",
        "-DFEATURE_qdoc=OFF",
        "-DFEATURE_clang=OFF",
        "-DFEATURE_distancefieldgenerator=OFF",
        "-DFEATURE_pixeltool=OFF",
        "-DFEATURE_qtdiag=OFF",
        "-DFEATURE_qtplugininfo=OFF",
        "-DFEATURE_qev=OFF",
    ] )

    # Qt Multimedia builds its test helper library whenever QtTest exists, even
    # without tests, and on Linux it needs linux/udmabuf.h (kernel 5.0
    # headers, el8 has 4.18). Nothing here uses it.
    _patch_source( "qtmultimedia", "src/CMakeLists.txt",
                   "if(QT_FEATURE_testlib AND TARGET Qt::Test)\n    add_subdirectory(multimediatestlib)",
                   "if(FALSE) # Euro-Office: no multimediatestlib\n    add_subdirectory(multimediatestlib)" )
    # FFmpeg is the default backend; on Windows Qt also builds its deprecated
    # native backend, as in Qt's own binaries
    multimedia_args = [ "-DFEATURE_ffmpeg=ON", f"-DFFMPEG_DIR={ prefix }" ]
    if nc.is_linux():
        multimedia_args += [ "-DFEATURE_gstreamer=OFF", "-DFEATURE_pulseaudio=ON" ]
    _cmake_module( "qtmultimedia", prefix, multimedia_args )

    # The build trees are big and never reused
    shutil.rmtree( nc.work_dir / "build", ignore_errors = True )


def fetch_and_patch():
    nc.create_workdir()
    fetch_sources()
    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def build_and_install():
    if not ( nc.is_linux() or nc.is_windows() ):
        nc.abort_op( f"Qt is only built for Linux and Windows, not { sys.platform }" )
    nc.create_install_dir()
    build_from_source()
    nc.create_install_dir_ok_marker()


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()
    if not nc.install_dir_looks_ok():
        build_and_install()


nc.ensure_dep( build_all )
