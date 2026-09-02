#!/usr/bin/env python3
import sys
import os
import platform
from pathlib import Path

script_path = Path( sys.argv[0] ).resolve()
script_dir = script_path.parent

# ---------------------------------------------------------------------------
# What to install.
#
# Set qt_version to whatever the project targets. Discover what's actually
# available for a given platform with:
#     python -m aqt list-qt windows desktop            # list versions
#     python -m aqt list-qt windows desktop --arch 6.11.1   # list archs
#
# NOTE on 6.11.x: download.qt.io changed its directory layout at 6.11.0, so a
# stale aqt can fail to resolve it. The fetch phase upgrades aqt for this
# reason.
# ---------------------------------------------------------------------------
qt_version = "6.10.1"

# Add-on modules that are NOT part of the base desktop package.
# Core, Gui, Widgets, PrintSupport, Svg and the Linguist tools
# (lrelease/lupdate/lconvert + the Qt6LinguistTools CMake package) all ship in
# the base package, so only Multimedia (which also provides MultimediaWidgets)
# has to be requested explicitly. List everything available with:
#     python -m aqt list-qt <host> desktop --modules <qt_version> <arch>
qt_modules = [ "qtmultimedia" ] # , "debug_info"

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


def fetch_and_patch():
    nc.create_workdir()

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def build_and_install():
    nc.create_install_dir()
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