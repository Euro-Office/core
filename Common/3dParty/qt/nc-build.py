#!/usr/bin/env python3
import sys
import shutil
import os
import urllib.request
import zipfile
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

qt_major = "5.9"
qt_version = "5.9.9"
qt_src_name = f"qt-everywhere-opensource-src-{qt_version}"
qt_url_base = f"https://download.qt.io/archive/qt/{qt_major}/{qt_version}/single"

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )

import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "Qt",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
)


def long_path( path ):
    # Extended-length path prefix so extraction of the deeply nested Qt
    # source tree does not hit the 260 char MAX_PATH limit on Windows
    if nc.is_windows():
        return "\\\\?\\" + str( Path( path ).resolve() )
    return str( path )


def fetch_and_patch():
    nc.create_workdir()

    if nc.is_linux():
        tarball = nc.work_dir / f"qt_source_{qt_version}.tar.xz"

        nc.run_command(
            [ "wget", f"{qt_url_base}/{qt_src_name}.tar.xz", "-O", str( tarball ) ],
            "Download Qt source",
            nc.work_dir
        )

        nc.run_command(
            [ "tar", "-xf", str( tarball ) ],
            "Extract Qt source",
            nc.work_dir
        )

        try:
            tarball.unlink()
        except FileNotFoundError:
            pass

    elif nc.is_windows():
        # Qt provides a .zip source package with CRLF line endings for Windows
        zip_path = nc.work_dir / f"qt_source_{qt_version}.zip"

        print( "Downloading Qt source (zip)..." )
        try:
            urllib.request.urlretrieve( f"{qt_url_base}/{qt_src_name}.zip", zip_path )
        except Exception as e:
            nc.abort_op( f"Download failed: {e}" )

        print( "Extracting Qt source..." )
        try:
            with zipfile.ZipFile( zip_path ) as zf:
                zf.extractall( long_path( nc.work_dir ) )
        except Exception as e:
            nc.abort_op( f"Extraction failed: {e}" )

        try:
            zip_path.unlink()
        except FileNotFoundError:
            pass

    else:
        nc.abort_op( f"Unknown target platform: {sys.platform}" )

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    qt_source_dir = nc.work_dir / qt_src_name

    # Flags shared between platforms
    common_flags = [
        "-opensource",
        "-confirm-license",
        "-release",
        "-shared",
        "-accessibility",
        "-qt-zlib",
        "-qt-libpng",
        "-qt-libjpeg",
        "-qt-pcre",
        "-no-sql-sqlite",
        "-no-qml-debug",
        "-nomake", "examples",
        "-nomake", "tests",
        "-skip", "qtenginio",
        "-skip", "qtlocation",
        "-skip", "qtserialport",
        "-skip", "qtsensors",
        "-skip", "qtxmlpatterns",
        "-skip", "qt3d",
        "-skip", "qtwebview",
        "-skip", "qtwebengine",
        "-skip", "qtscript",
    ]

    if nc.is_linux():
        # Keep the same install layout as the previous docker build
        qt_prefix = nc.install_dir / f"qt"

        nc.run_command(
            [ "./configure" ] + common_flags + [
                "-prefix", str( qt_prefix ),
                "-qt-xcb",
                "-gstreamer", "1.0",
                # flags to match target build after comparison to onlyoffice build
                "-dbus-linked",
                "-icu",
                "-no-iconv",
                "-fontconfig",
                "-system-freetype",
                "-system-harfbuzz",
                "-alsa",
                "-pulseaudio",
                # "-openssl-linked",
                "-cups",
                "-gtk"
            ],
            "Configure",
            qt_source_dir
        )

        nc.run_command(
            [ "make", f"-j{os.cpu_count()}" ],
            "Build",
            qt_source_dir
        )

        nc.run_command(
            [ "make", "install" ],
            "Install",
            qt_source_dir
        )

    elif nc.is_windows():
        qt_prefix = nc.install_dir / f"Qt-{qt_version}" / "msvc_64"

        # The bat file sets up the MSVC environment (vcvarsall) and then
        # runs configure.bat / nmake. Windows-only configure flags are
        # appended after the common ones.
        windows_flags = [
            "-prefix", str( qt_prefix ),
            "-platform", "win32-msvc",
            "-opengl", "desktop",
            "-mp",
            "-no-icu",
            "-no-iconv"
        ]

        bat_path = script_dir / "nc-build-qt.bat"

        nc.run_command(
            [   "cmd.exe",
                "/c",
                "call",
                str( bat_path ),
                str( qt_source_dir )
            ] + common_flags + windows_flags,
            "MSVC build",
            qt_source_dir
        )

    else:
        nc.abort_op( f"Unknown target platform: {sys.platform}" )

    nc.create_install_dir_ok_marker()
    nc.fix_terminal_encoding()
    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()
    if not nc.install_dir_looks_ok():
        build_and_install()


nc.ensure_dep( build_all )