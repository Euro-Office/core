#!/usr/bin/env python3
import sys
import shutil
import os
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

qt_major = "5.9"
qt_version = "5.9.9"
qt_src_name = f"qt-everywhere-opensource-src-{qt_version}"
qt_url = f"https://download.qt.io/archive/qt/{qt_major}/{qt_version}/single/{qt_src_name}.tar.xz"

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


def fetch_and_patch():
    nc.create_workdir()

    tarball = nc.work_dir / f"qt_source_{qt_version}.tar.xz"

    nc.run_command(
        [ "wget", qt_url, "-O", str( tarball ) ],
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

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()

    qt_source_dir = nc.work_dir / qt_src_name

    if nc.is_linux():
        # Keep the same install layout as the previous docker build
        # (prefix was ../qt_build/Qt-5.9.9/gcc_64)
        qt_prefix = nc.install_dir / f"qt"

        nc.run_command(
            [   "./configure",
                "-opensource",
                "-confirm-license",
                "-release",
                "-shared",
                "-accessibility",
                "-prefix", str( qt_prefix ),
                "-qt-zlib",
                "-qt-libpng",
                "-qt-libjpeg",
                "-qt-xcb",
                "-qt-pcre",
                "-no-sql-sqlite",
                "-no-qml-debug",
                "-gstreamer", "1.0",
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
        nc.abort_op( "Windows build is not implemented yet" )

    else:
        nc.abort_op( f"Unkown target platform: {sys.platform}" )

    nc.create_install_dir_ok_marker()
    nc.fix_terminal_encoding()
    print( "Build and install completed" )


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()
    if not nc.install_dir_looks_ok():
        build_and_install()


nc.ensure_dep( build_all )