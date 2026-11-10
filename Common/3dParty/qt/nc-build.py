#!/usr/bin/env python3
import sys
import os
import platform
import hashlib
import shutil
import tarfile
import time
import urllib.request
from pathlib import Path

script_path = Path( sys.argv[0] ).resolve()
script_dir = script_path.parent

# ---------------------------------------------------------------------------
# What to install.
#
# Linux builds Qt from source (build_from_source below): Qt's prebuilt Linux
# binaries need glibc 2.34 (amd64) or 2.38 (arm64), newer than the oldest
# distro we support (Rocky/RHEL 8, glibc 2.28). Qt 6.12 is an LTS release.
#
# Windows installs Qt's prebuilt binaries with aqt. aqtinstall 3.3.0 can't
# read download.qt.io's Windows layout from 6.11 on (aqtinstall issue #1047),
# so Windows stays on 6.10.x until it can. Discover what aqt can install with:
#     python -m aqt list-qt windows desktop            # list versions
#     python -m aqt list-qt windows desktop --arch 6.10.1   # list archs
# ---------------------------------------------------------------------------
qt_version = "6.10.1" if sys.platform == "win32" else "6.12.0"

# aqt: add-on modules that are NOT part of the base desktop package.
# Core, Gui, Widgets, PrintSupport, Svg and the Linguist tools
# (lrelease/lupdate/lconvert + the Qt6LinguistTools CMake package) all ship in
# the base package, so only Multimedia (which also provides MultimediaWidgets)
# has to be requested explicitly. List everything available with:
#     python -m aqt list-qt <host> desktop --modules <qt_version> <arch>
qt_modules = [ "qtmultimedia" ] # , "debug_info"

# Source build: the Qt submodules, in build order, with the SHA-256 that
# download.qt.io publishes next to each tarball. qtshadertools is a build
# dependency of qtmultimedia. The Wayland client and platform plugin are part
# of qtbase since 6.12, so qtwayland (now mostly the compositor) isn't needed.
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

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )

import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "Qt",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "2"
)


def detect_host_and_arch():
    """Map the running platform to the (aqt host, aqt arch) pair.

    Qt only ships MSVC binaries for Windows-on-ARM, and Linux desktop arm64
    binaries exist from Qt 6.7.0 onward. macOS desktop Qt (6.1.2+) is a single
    universal binary covering both Intel and Apple Silicon, so there is only
    one arch ("clang_64", installed under a "macos" subdirectory by aqt).
    """
    machine = platform.machine().lower()
    is_arm = machine in ( "arm64", "aarch64" )

    if nc.is_windows():
        if is_arm:
            return "windows_arm64", "win64_msvc2022_arm64"
        return "windows", "win64_msvc2022_64"

    elif nc.is_linux():
        if is_arm:
            return "linux_arm64", "linux_gcc_arm64"
        return "linux", "linux_gcc_64"

    elif nc.is_apple_silicon():
        return "mac", "clang_64"

    else:
        nc.abort_op( f"Unsupported platform: {sys.platform} / {machine}" )


def _pip_install_aqt( python_bin : str ) -> bool:
    try:
        nc.capture_process_output( [ python_bin, "-m", "pip", "install", "--quiet", "--upgrade", "aqtinstall" ] )
        return True
    except Exception:
        return False


def get_aqt_python() -> str:
    """Return a python executable with an up-to-date 'aqt' importable.

    download.qt.io's directory layout can change between Qt releases (e.g.
    6.11.0), so a stale aqtinstall can fail to resolve versions -- always
    upgrade it before use. Homebrew (and other PEP 668 "externally managed")
    pythons refuse a plain "pip install" into system site-packages, so fall
    back to a small local venv under the work dir rather than mutating the
    system install (same approach as v8/nc-build.py's
    get_gn_hermetic_check_python).
    """
    if _pip_install_aqt( sys.executable ):
        return sys.executable

    venv_dir = nc.work_dir / "aqt_venv"
    venv_python = venv_dir / ( "Scripts" if nc.is_windows() else "bin" ) / ( "python.exe" if nc.is_windows() else "python3" )

    if not venv_python.exists():
        print( "System python refused aqtinstall (externally managed?); creating a local venv instead (not touching the system install)..." )
        nc.run_command( [ sys.executable, "-m", "venv", str( venv_dir ) ], "Creating aqt venv" )

    if not _pip_install_aqt( str( venv_python ) ):
        nc.abort_op( "Failed to install aqtinstall (system python refused it, and venv install also failed)." )

    return str( venv_python )


def find_qt_prefix():
    """Locate the directory aqt extracted into, without hard-coding the
    folder-name transform (it varies across hosts and Qt versions).

    A valid desktop Qt prefix always has both bin/ and lib/cmake/Qt6/.
    """
    version_dir = nc.install_dir / qt_version
    if not version_dir.is_dir():
        return None

    for child in sorted( version_dir.iterdir() ):
        if child.is_dir() \
                and ( child / "bin" ).is_dir() \
                and ( child / "lib" / "cmake" / "Qt6" ).is_dir():
            return child
    return None


# ---------------------------------------------------------------------------
# Linux: build from source
# ---------------------------------------------------------------------------

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


def _extract( archive : Path, dest_parent : Path ) -> Path:
    # Each tarball holds a single top-level directory named like the archive
    name = archive.name.removesuffix( ".tar.xz" )
    target = dest_parent / name
    if target.exists():
        shutil.rmtree( target )
    with tarfile.open( archive, "r:xz" ) as tar:
        tar.extractall( dest_parent, filter = "data" )
    if not target.is_dir():
        nc.abort_op( f"{ archive.name } did not unpack to { name }/" )
    return target


def source_prefix() -> Path:
    # Same layout as aqt's (<version>/gcc_64), which common.cmake and
    # find_qt_prefix() look for
    return nc.install_dir / qt_version / ( "gcc_arm64" if nc.is_arm64() else "gcc_64" )


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


def build_ffmpeg( prefix : Path ):
    src = nc.work_dir / "src" / f"ffmpeg-{ ffmpeg_version }"
    build = nc.work_dir / "build" / "ffmpeg"
    if build.exists():
        shutil.rmtree( build )
    nc.ensure_directory_exists( build )

    # Qt's own option set (qt5.git coin/provisioning/common/shared/
    # ffmpeg_config_options.txt), shared. On top of it, nothing that would
    # make the libraries need more than a stock el8 at runtime: no OpenSSL
    # (el8 has 1.1), no VA-API/VDPAU/DRM/X11/SDL.
    # The rpath lets libavformat & co. find each other next to themselves;
    # The backslash and "$$" make "$ORIGIN" survive both make and the shell.
    options = [
        "--disable-programs", "--disable-doc", "--disable-debug", "--enable-network",
        "--disable-lzma", "--enable-pic", "--disable-vulkan", "--disable-v4l2-m2m",
        "--disable-decoder=truemotion1", "--disable-avdevice", "--disable-avfilter",
        "--enable-shared", "--disable-static",
        "--disable-openssl", "--disable-vaapi", "--disable-vdpau", "--disable-libdrm",
        "--disable-xlib", "--disable-libxcb", "--disable-sdl2",
        f"--prefix={ prefix }",
        "--extra-ldflags=-Wl,-rpath,\\$$ORIGIN",
    ]
    nc.run_command( [ str( src / "configure" ) ] + options, "Configure FFmpeg", build )
    nc.run_command( [ "make", f"-j{ _jobs() }" ], "Build FFmpeg", build )
    nc.run_command( [ "make", "install" ], "Install FFmpeg", build )


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
        configure = [ str( prefix / "bin" / "qt-cmake" ), "-S", str( src ), "-B", str( build ), "-G", "Ninja" ]

    flags = " ".join( nc.baseline_arch_flags() )
    configure += [
        "-DCMAKE_BUILD_TYPE=Release",
        "-DQT_BUILD_EXAMPLES=OFF",
        "-DQT_BUILD_TESTS=OFF",
        "-DQT_GENERATE_SBOM=OFF",
        f"-DCMAKE_C_FLAGS={ flags }",
        f"-DCMAKE_CXX_FLAGS={ flags }",
    ] + args

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


def build_from_source():
    prefix = source_prefix()
    if prefix.exists():
        shutil.rmtree( prefix )
    nc.ensure_directory_exists( prefix )
    print( f"Building Qt { qt_version } from source into { prefix }" )

    # FFmpeg goes into the Qt prefix, so its libraries end up in lib/ next to
    # Qt's, as in aqt's layout (and desktop-apps installs lib/lib*.so*)
    build_ffmpeg( prefix )

    # Bundled harfbuzz, libpng, libjpeg, pcre2, zlib, double-conversion and
    # md4c keep the runtime dependencies to what every desktop distro has.
    # No ICU: core ships its own ICU 74, and Qt doesn't need one.
    _cmake_module( "qtbase", prefix, [
        "-DBUILD_SHARED_LIBS=ON",
        "-DFEATURE_icu=OFF",
        "-DFEATURE_system_harfbuzz=OFF",
        "-DFEATURE_system_png=OFF",
        "-DFEATURE_system_jpeg=OFF",
        "-DFEATURE_system_pcre2=OFF",
        "-DFEATURE_system_zlib=OFF",
        "-DFEATURE_system_doubleconversion=OFF",
        "-DFEATURE_system_libb2=OFF",
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
    ] + _openssl3_args() )

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
    # without tests, and it needs linux/udmabuf.h (kernel 5.0 headers, el8 has
    # 4.18). Nothing here uses it.
    _patch_source( "qtmultimedia", "src/CMakeLists.txt",
                   "if(QT_FEATURE_testlib AND TARGET Qt::Test)\n    add_subdirectory(multimediatestlib)",
                   "if(FALSE) # Euro-Office: no multimediatestlib\n    add_subdirectory(multimediatestlib)" )
    _cmake_module( "qtmultimedia", prefix, [
        "-DFEATURE_ffmpeg=ON",
        f"-DFFMPEG_DIR={ prefix }",
        "-DFEATURE_gstreamer=OFF",
        "-DFEATURE_pulseaudio=ON",
    ] )

    # The build trees are big and never reused
    shutil.rmtree( nc.work_dir / "build", ignore_errors = True )


def fetch_and_patch():
    nc.create_workdir()
    if nc.is_linux():
        fetch_sources()

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()
    if nc.is_linux():
        build_from_source()
        nc.create_install_dir_ok_marker()
        return

    host, arch = detect_host_and_arch()
    aqt_python = get_aqt_python()
    print(f"Installing Qt {qt_version} for host '{host}', arch '{arch}'")

    # 1. Real Qt install: full base package (Core/Gui/Widgets/PrintSupport/Svg/
    #    Linguist tools) + add-on modules. NO --archives, so Svg etc. come in.
    cmd = [
        aqt_python, "-m", "aqt", "install-qt",
        host, "desktop", qt_version, arch,
        "--outputdir", str(nc.install_dir),
    ]
    real_modules = [m for m in qt_modules if m != "debug_info"]
    if real_modules:
        cmd += ["-m"] + real_modules
    nc.run_command(cmd, "Download Qt binaries (aqt)", nc.install_dir)

    # 2. Debug symbols: separate call, qtbase archive only, same prefix.
    #    --archives qtbase here is correct. We only want Core/Gui/Widgets/
    #    qwindows PDBs, not the whole multi-GB debug set.
    if nc.is_windows() and "debug_info" in qt_modules:
        dbg = [
            aqt_python, "-m", "aqt", "install-qt",
            host, "desktop", qt_version, arch,
            "--outputdir", str(nc.install_dir),
            "-m", "debug_info",
            "--archives", "qtbase",
        ]
        nc.run_command(dbg, "Download Qt debug symbols (aqt)", nc.install_dir)

    prefix = find_qt_prefix()

    nc.create_install_dir_ok_marker()


def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()
    if not nc.install_dir_looks_ok():
        build_and_install()


nc.ensure_dep( build_all )