#!/usr/bin/env python3

import sys
import shutil
import os
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "Boost",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
)

modules_needed = [ "headers", "system", "filesystem", "regex", "date_time" ]
# Only these submodules get checked out, so every boost header the project
# includes must be reachable from this list (directly or as a boostdep-resolved
# dependency of one of them).
#
# ptr_container / serialization / spirit / variant are listed explicitly because
# they are NOT dependencies of anything above: they used to be pulled in
# transitively on 1.78, but boost has been migrating away from Boost.Variant
# towards std::variant, so on current boost they are no longer dragged in and
# the headers went missing -> "fatal error C1083: Cannot open include file:
# 'boost/variant.hpp'" in libetonyek, and the same for the spirit / ptr_container
# / boost-archive includes in librevenge.
#
# NOTE: boost/archive/** (the base64 iterators librevenge uses) ships in the
# SERIALIZATION module - there is no boost module called "archive". Those
# iterators are pure templates, so headers alone are enough and serialization
# does not have to be built.
header_only_modules_needed = [ "any", "asio", "beast", "foreach", "format", "functional",
                               "multi_index", "ptr_container", "serialization", "spirit",
                               "uuid", "variant" ]

def fetch_and_patch():
    nc.create_workdir()
    print( "Clone Boost 1.78.0..." )
    nc.run_command(
        [ "git", "clone", "https://github.com/boostorg/boost.git", nc.work_dir, "--depth", "1" ],
        "Clone Boost 1.78.0"
    )

    print( "Get boostdep..." )
    nc.run_command(
        [ "git", "submodule", "update", "--depth", "1", "-q", "--init", Path( "tools" ) / "boostdep" ],
        "Get boostdep",
        nc.work_dir
    )

    for module in ( modules_needed + header_only_modules_needed ):
        print( f"Initializing { module }..." )
        nc.run_command(
            [ "git", "submodule", "update", "--depth", "1", "-q", "--init", Path( "libs" ) / module ],
            f"Init { module }",
            nc.work_dir
        )

    for module in ( modules_needed + header_only_modules_needed ):
        print( f"Running boostdep for { module }..." )
        nc.run_command(
            [ "python", Path( "tools" ) / "boostdep" / "depinst" / "depinst.py", "-X", "test", "-g", "--depth 1", module ],
            f"Get dependencies for {module}",
            nc.work_dir
        )

    nc.create_work_dir_ok_marker()
    print( "Fetch & patch completed" )


def boost_msvc_arch() -> tuple[ str, str ]:
    """
    Windows-only. Returns ( b2 'architecture=' value, MSVC Host*/* tools
    subdir ) for the Boost build. b2 calls the whole Intel/AMD family "x86".
    """
    a = nc.target_arch()
    if a == "x64":
        return "x86", "Hostx64\\x64"
    if a == "arm64":
        return "arm", "Hostarm64\\arm64"
    nc.abort_op( f"Unsupported target arch for boost: {a!r}" )

def jam_path( p ) -> str:
    return str( Path( p ) ).replace( "\\", "\\\\" )

def boost_msvc_toolset_version() -> str:
    """
    The 'using msvc : <version>' value for the compiler we actually build with.

    Boost derives the library name tag from this value - common.jam's
    toolset-tag joins major+minor, so 14.0 -> vc140, 14.3 -> vc143,
    14.5 -> vc145. CMake's BoostConfig computes the same tag from the compiler
    it detects and REJECTS libs whose tag differs:

        libboost_filesystem-vc140-mt-x64-1_92.lib (vc140, detected vc145)
        No suitable build variant has been found.

    which is exactly what happened while this was hardcoded to 14.0 but cl.exe
    came from MSVC 14.51 - the libs were built by the right compiler, only the
    name lied. So derive it from the toolchain actually in use.

    MSVC's tag keeps only the FIRST digit of the minor version (14.51 -> vc145,
    14.39 -> vc143), hence 'MAJOR.<first digit of MINOR>' from VCToolsVersion.
    """
    ver = os.environ.get( "VCToolsVersion", "" )
    parts = ver.split( "." )
    if len( parts ) < 2 or not parts[ 0 ].isdigit() or not parts[ 1 ][ :1 ].isdigit():
        nc.abort_op(
            f"Cannot derive the MSVC toolset version from VCToolsVersion={ ver!r }. "
            "Is the MSVC environment loaded (vcvars)?"
        )
    return f"{ parts[ 0 ] }.{ parts[ 1 ][ 0 ] }"

def build_and_install():
    nc.create_install_dir()
    
    print( "Running bootstrap..." )
    if nc.is_linux():
        nc.run_command(
            [ "./bootstrap.sh", f"--prefix={ nc.install_dir }" ],
            "Running bootstrap",
            nc.work_dir
        )
    elif nc.is_windows():
        boost_arch, host_subdir = boost_msvc_arch()

        nc.run_command(
            [ "cmd.exe", "/c" "bootstrap.bat", f"--prefix={ nc.install_dir }" ],
            "Running bootstrap",
            nc.work_dir
        )
    else:
        nc.abort_op( f"Unkown target platform: {sys.platform}" )

    if nc.is_windows():
        print( "Fixing project-config.jam..." )
        msvc_version = boost_msvc_toolset_version()
        print( f"Using MSVC toolset { msvc_version } "
               f"(libs will be tagged vc{ msvc_version.replace( '.', '' ) })" )
        # Jam treats a backslash inside a quoted string as an escape, so a raw
        # Windows path silently collapses ("C:\Program Files\..." arrives as
        # "C:Program Files...") and b2 then warns "Did not find command for MSVC
        # toolset" and falls back to whatever cl.exe is on PATH. jam_path()
        # doubles the separators, which is exactly what it exists for.
        # Built outside the f-string so no backslash appears in an f-string
        # expression (only allowed from Python 3.12 on).
        cl_path = jam_path(
            Path( os.environ[ "VCToolsInstallDir" ] ) / "bin" / host_subdir / "cl.exe"
        )
        content = f"""
# Boost.Build Configuration
# Generated by nc-build.py

import option ;

using msvc : { msvc_version } : "{ cl_path }";

option.set keep-going : false ;

"""
        ( Path( nc.work_dir ) / "project-config.jam" ).write_text( content )

    build_cmd = [ ( nc.work_dir / "b2.exe" ) if nc.is_windows() else "./b2" ]
    for module in modules_needed:
        build_cmd.append( f"--with-{ module }" )
    build_cmd.append( "variant=release" )
    build_cmd.append( "link=static" )
    if not nc.is_windows():
        build_cmd.append( "cflags=-fPIC" )
        build_cmd.append( "cxxflags=-fPIC" )
    build_cmd.append( f"--prefix={ nc.install_dir }" )
    if nc.is_windows():
        build_cmd.append( "address-model=64" )
        build_cmd.append( f"architecture={ boost_arch }" )
    build_cmd.append( "install" )

    print( "Running b2..." )
    nc.run_command(
        build_cmd,
        "Build and install boost libs",
        nc.work_dir
    )

    nc.create_install_dir_ok_marker()
    
    print( "Build and install completed" )

def build_all():
    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    if not nc.install_dir_looks_ok():
        if nc.is_windows() and shutil.which("nmake") is None:
            raise RuntimeError(
                "MSVC environment is not set up: 'nmake' not found in PATH.\n"
                "Run 'vcvarsx86_amd64.bat' or use 'x64 Native Tools Command Prompt'."
            )
        build_and_install()

nc.ensure_dep( build_all )