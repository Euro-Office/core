#!/usr/bin/env python3

import sys
import shutil
import os
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent
icu_tag = "release-74-2"

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.init_for_dep(
    depname = "ICU",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "1"
)

def fetch_and_patch():
    nc.create_workdir()
    
    nc.run_command(
        [ "git", "-c", "core.autocrlf=false", "-c", "core.eol=lf", "clone",
          "--depth", "1", "--branch", icu_tag,
          "https://github.com/unicode-org/icu.git", str(nc.work_dir / "icu2")
        ],
        "Checkout git repo",
    )

    try:
        shutil.copytree(
            nc.work_dir / "icu2" / "icu4c",
            nc.work_dir / "icu",
            copy_function=shutil.copy2
        )
    except Exception as e:
        nc.abort_op( f"Copy failed: {e}" )

    try:
        shutil.copy2( nc.work_dir / "icu2" / "LICENSE", nc.work_dir / "LICENSE" )
    except Exception as e:
        nc.abort_op( f"License copy failed: {e}" )

    try:
        shutil.rmtree( nc.work_dir / "icu2" )
    except FileNotFoundError:
        pass

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

def fix_macos_install_names( install_dir ):
    # ICU's own Darwin makefile (icu/source/config/mh-darwin) bakes an
    # absolute path into each dylib's install name when --enable-rpath is
    # passed to configure (its "else" branch isn't @rpath-based either, just
    # a bare filename) - there's no ./configure flag that produces a proper
    # @rpath install name. Rewrite them here instead, same idea as the CEF
    # framework restructuring done for the Xcode build.
    lib_dir = install_dir / "lib"
    dylibs = [ p for p in lib_dir.glob( "*.dylib" ) if not p.is_symlink() ]

    old_to_new = {}
    for dylib in dylibs:
        output = nc.capture_process_output( [ "otool", "-D", str( dylib ) ] )
        lines = [ l.strip() for l in output.splitlines() if l.strip() and not l.strip().endswith( ":" ) ]
        if not lines:
            continue
        old_name = lines[0]
        new_name = f"@rpath/{ Path( old_name ).name }"
        old_to_new[ old_name ] = new_name
        nc.run_command(
            [ "install_name_tool", "-id", new_name, str( dylib ) ],
            f"Fix install name for {dylib.name}"
        )

    for dylib in dylibs:
        output = nc.capture_process_output( [ "otool", "-L", str( dylib ) ] )
        for line in output.splitlines()[1:]:
            dep = line.strip().split( " (" )[0]
            if dep in old_to_new:
                nc.run_command(
                    [ "install_name_tool", "-change", dep, old_to_new[ dep ], str( dylib ) ],
                    f"Fix {Path(dep).name} reference in {dylib.name}"
                )

def build_android():
    # A cross build needs a host build of ICU for its data tools. Both are
    # built out of source so they don't collide in icu/source.
    icu_source_dir = nc.work_dir / "icu" / "source"
    host_build_dir = nc.work_dir / "build-host"
    target_build_dir = nc.work_dir / f"build-android-{ nc.android_abi() }"

    if not ( host_build_dir / "bin" / "icupkg" ).exists():
        nc.ensure_directory_exists( host_build_dir )
        nc.run_command(
            [   str( icu_source_dir / "configure" ),
                "--disable-tests",
                "--disable-samples",
                "CXXFLAGS=-std=c++11",
            ],
            "Configure (host)",
            host_build_dir
        )
        nc.run_command(
            [ "make", f"-j{os.cpu_count()}" ],
            "Build (host)",
            host_build_dir
        )

    if target_build_dir.exists():
        shutil.rmtree( target_build_dir )
    nc.ensure_directory_exists( target_build_dir )

    # Static archives first; the shared libraries are linked from them below
    tc = nc.android_toolchain_env()
    nc.run_command(
        [   str( icu_source_dir / "configure" ),
            f"--host={ nc.android_triple() }",
            f"--with-cross-build={ host_build_dir }",
            f"--prefix={ nc.install_dir }",
            "--enable-static",
            "--disable-shared",
            "--with-data-packaging=static",
            "--disable-tools",
            "--disable-tests",
            "--disable-samples",
            "--disable-extras",
            f"CC={ tc[ 'CC' ] }",
            f"CXX={ tc[ 'CXX' ] }",
            f"AR={ tc[ 'AR' ] }",
            f"RANLIB={ tc[ 'RANLIB' ] }",
            "CFLAGS=-fPIC -O2",
            "CXXFLAGS=-fPIC -O2",
        ],
        "Configure",
        target_build_dir
    )

    nc.run_command(
        [ "make", f"-j{os.cpu_count()}" ],
        "Build",
        target_build_dir
    )

    nc.run_command(
        [ "make", "install" ],
        "Install",
        target_build_dir
    )

    link_android_shared_libs( nc.install_dir / "lib", tc[ "CXX" ] )

def link_android_shared_libs( lib_dir : Path, cxx : str ):
    # Shared libraries, so UnicodeConverter and x2t share one copy of ICU and its
    # data. Their names must differ from the system's ICU (libicuuc.so), which is
    # already loaded in every app process and would be used instead, and Android
    # only loads unversioned lib<name>.so files: libicuuc.a -> libicuuc74.so.
    major = icu_major
    libraries = [
        ( "icudata", [] ),
        ( "icuuc", [ "icudata" ] ),
        ( "icui18n", [ "icuuc", "icudata" ] ),
    ]
    for name, dependencies in libraries:
        output = lib_dir / f"lib{ name }{ major }.so"
        nc.run_command(
            [   cxx, "-shared", "-o", str( output ),
                f"-Wl,-soname,{ output.name }",
                "-Wl,--whole-archive", str( lib_dir / f"lib{ name }.a" ), "-Wl,--no-whole-archive",
                f"-L{ lib_dir }",
            ] + [ f"-l{ dependency }{ major }" for dependency in dependencies ],
            f"Link { output.name }"
        )

    for archive in lib_dir.glob( "*.a" ):
        archive.unlink()

def build_and_install():
    nc.create_install_dir()

    if nc.is_android():
        build_android()

    elif nc.is_linux():
        nc.run_command(
            [   "./configure",
                f"--prefix={nc.install_dir}",
                "--enable-rpath",
                "CC=gcc",
                "CXX=g++",
                "AR=ar",
                "RANLIB=ranlib",
                "CXXFLAGS=-static-libstdc++ -static-libgcc -std=c++11",
                "LDFLAGS=-Wl,-rpath,$ORIGIN"
            ],
            "Configure",
            nc.work_dir / "icu" / "source"
        )

        nc.run_command(
            [ "make", f"-j{os.cpu_count()}" ],
            "Build",
            nc.work_dir / "icu" / "source"
        )

        nc.run_command(
            [ "make", "install" ],
            "Install",
            nc.work_dir / "icu" / "source"
        )

    elif nc.is_apple_silicon():
        nc.run_command(
            [   "./configure",
                f"--prefix={nc.install_dir}",
                "--enable-rpath",
                "CC=clang",
                "CXX=clang++",
                "AR=ar",
                "RANLIB=ranlib",
                "CXXFLAGS=-std=c++11",
                "LDFLAGS=-Wl,-rpath,@loader_path"
            ],
            "Configure",
            nc.work_dir / "icu" / "source"
        )

        nc.run_command(
            [ "make", f"-j{os.cpu_count()}" ],
            "Build",
            nc.work_dir / "icu" / "source"
        )

        nc.run_command(
            [ "make", "install" ],
            "Install",
            nc.work_dir / "icu" / "source"
        )

        fix_macos_install_names( nc.install_dir )

    elif nc.is_windows():
        icu_source_dir = nc.work_dir / "icu" / "source"
        bat_path = script_dir / "nc-build.bat"
        nc.run_command(
            [   "cmd.exe",
                "/c",
                "call",
                str(bat_path),
                "", "",
                str(icu_source_dir),
                str(nc.install_dir)
            ],
            "Cygwin build",
            nc.work_dir / "icu" / "source"
        )

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