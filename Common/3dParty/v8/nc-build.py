#!/usr/bin/env python3

import sys
import shutil
import os
import re
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

# The V8 release Node.js ships in its current LTS line. Node builds V8 the way
# we need it (system C++ standard library, no sandbox, not part of Chromium),
# so its version is the one with the best track record for such builds.
V8_VERSION = "14.6.202.34"

# depot_tools moves fast and occasionally breaks older V8 checkouts, so it is
# pinned. 2026-10-30, verified with V8_VERSION above. Bump together with it.
DEPOT_TOOLS_REVISION = "a20039648c0de69d8156906b18052bd47e9bf230"

nc.init_for_dep(
    depname = "V8",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = V8_VERSION
)

depot_tools_path = nc.work_dir / "depot_tools"
v8_root_path = nc.work_dir / "v8"
v8_src_path = v8_root_path / "v8"

# Small helper dirs generated into the work dir (see the functions using them)
python_shim_path = nc.work_dir / "python-shim"
clang_shim_path = nc.work_dir / "clang-base"

def check_prequisites():
    tools_needed = [ "git" ]
    if nc.is_linux() or nc.is_apple_silicon():
        tools_needed.append( "clang" )
    for tool in tools_needed:
        if shutil.which( tool ) is None:
            nc.abort_op( f"Tool not found: {tool}" )

    # depot_tools (gclient) needs Python >= 3.11
    if sys.version_info < ( 3, 11 ):
        nc.abort_op( f"Python >= 3.11 required to build V8, running { sys.version.split()[0] }" )

def ensure_python_shim():
    # depot_tools and V8's build scripts call a bare "python3". On Rocky 9
    # that's the system 3.9, which current depot_tools rejects, so put the
    # interpreter running this script first on PATH under that name.
    # Windows doesn't need this: depot_tools brings its own python3.bat there.
    if nc.is_windows():
        return
    nc.ensure_directory_exists( python_shim_path )
    for name in ( "python3", "python" ):
        link = python_shim_path / name
        link.unlink( missing_ok = True )
        link.symlink_to( sys.executable )

def ensure_windows_git_shim():
    # depot_tools on Windows normally bootstraps its own bundled git the first
    # time gclient runs. DEPOT_TOOLS_UPDATE=0 (set in depot_env() for
    # reproducibility) suppresses that bootstrap, but gclient's git_cache
    # invokes "git.bat" and only catches CalledProcessError - the missing shim
    # crashes the sync with an unhandled FileNotFoundError. Point the shim at
    # the system git this script already depends on. depot_tools gitignores
    # /git.bat, so `git clean -fd` leaves it alone.
    if not nc.is_windows():
        return
    shim = depot_tools_path / "git.bat"
    if shim.exists():
        return
    git_exe = shutil.which("git")
    if git_exe is None:
        nc.abort_op("Tool not found: git")
    shim.write_text("@echo off\r\n\"" + git_exe + "\" %*\r\n")

def depot_env() -> dict:
    env = os.environ.copy()
    path = [ str( depot_tools_path ) ]
    if not nc.is_windows():
        path.insert( 0, str( python_shim_path ) )
    env[ "PATH" ] = os.pathsep.join( path + [ env[ "PATH" ] ] )
    env[ "GCLIENT_SUPPRESS_GIT_VERSION_WARNING" ] = "1"
    env[ "DEPOT_TOOLS_WIN_TOOLCHAIN" ] = "0"
    # Keep depot_tools from self-updating to HEAD, which would undo the pin
    env[ "DEPOT_TOOLS_UPDATE" ] = "0"
    env[ "VPYTHON_BYPASS" ] = "manually managed python not supported by chrome operations"
    return env

def disable_android_test_runner():
    # The Android test runner pulls in Chromium's //build/android and catapult,
    # which are only checked out for Chromium itself. Not needed for v8_monolith.
    tools_gn_path = v8_src_path / "tools" / "BUILD.gn"
    text = tools_gn_path.read_text()
    test_runner_dep = 'data_deps = [ "//build/android:test_runner_py" ]'
    if test_runner_dep not in text:
        return
    tools_gn_path.write_text( text.replace( test_runner_dep, "" ) )

def disable_crel():
    # On Linux with lld, Chromium's build assembles everything with ELF CREL
    # (compact relocations). libv8_monolith.a is linked into doctrenderer by
    # the system GNU ld, which can't read CREL (binutils 2.35 on Rocky 9), so
    # turn it off. There's no gn arg for it, and use_lld=false isn't an
    # option: the host tools' thin archives have no index GNU ld could use.
    compiler_gn_path = v8_src_path / "build" / "config" / "compiler" / "BUILD.gn"
    text = compiler_gn_path.read_text()
    crel_flags = 'cflags += [ "-Wa,--crel,--allow-experimental-crel" ]'
    if crel_flags not in text:
        nc.abort_op( f"Could not find the CREL flags in { compiler_gn_path }. Did the V8 build config change?" )
    compiler_gn_path.write_text( text.replace( crel_flags, "" ) )

def get_cpu() -> str:
    # The arch we are *building for*; detection (incl. the Windows MSVC target
    # arch and the amd64_arm64 cross prompt) lives in build_3rdparty_common.
    arch = nc.target_arch()
    if arch not in ( "x64", "arm64" ):
        nc.abort_op( f"Unsupported architecture for V8: {arch!r}" )
    return arch

def _find_clang_bin_dir() -> Path:
    # Search PATH for the first real clang++ installation, skipping wrappers
    # like ccache (/usr/lib64/ccache/clang++ resolves to /usr/bin/ccache).
    for dir_str in os.environ.get( "PATH", "" ).split( os.pathsep ):
        candidate = Path( dir_str ) / "clang++"
        if not candidate.exists():
            continue
        if "clang" in candidate.resolve().name.lower():
            return Path( dir_str )
    nc.abort_op( "No real clang++ installation found in PATH (checked for non-wrapper binary)" )

def _clang_major_version( clang : Path ) -> str:
    version = nc.capture_process_output( [ str( clang ), "-dumpversion" ] ).strip()
    if not re.fullmatch( r"\d+(\.\d+)*", version ):
        nc.abort_op( f"Could not read clang version from { clang }: { version!r}" )
    return version.split( "." )[ 0 ]

def prepare_clang_base() -> tuple[ Path, str ]:
    # Chromium's build links clang's builtins library from
    # <clang_base_path>/lib/clang/<major>/lib/<triple>/libclang_rt.builtins.a,
    # with Chromium's own spelling of the triple. Distro clangs keep it
    # elsewhere (Rocky: x86_64-redhat-linux-gnu/, apt.llvm.org:
    # linux/libclang_rt.builtins-x86_64.a), so hand gn a small base dir whose
    # bin/ is the real one and whose builtins link to where this clang has them.
    bin_dir = _find_clang_bin_dir()
    clang = bin_dir / "clang"
    major = _clang_major_version( clang )

    if nc.is_apple_silicon():
        triple_dir, lib_name = "darwin", "libclang_rt.osx.a"
    else:
        triple_dir = { "x64": "x86_64-unknown-linux-gnu", "arm64": "aarch64-unknown-linux-gnu" }[ get_cpu() ]
        lib_name = "libclang_rt.builtins.a"

    builtins = Path( nc.capture_process_output(
        [ str( clang ), "-print-libgcc-file-name", "--rtlib=compiler-rt" ] ).strip() )
    if not builtins.is_file():
        nc.abort_op( f"clang builtins { builtins } not found. Is compiler-rt installed?" )

    if clang_shim_path.exists():
        shutil.rmtree( clang_shim_path )
    lib_dir = clang_shim_path / "lib" / "clang" / major / "lib" / triple_dir
    nc.ensure_directory_exists( lib_dir )
    ( clang_shim_path / "bin" ).symlink_to( bin_dir, target_is_directory = True )
    ( lib_dir / lib_name ).symlink_to( builtins )

    return clang_shim_path, major

def _find_windows_llvm_dir() -> Path:
    # V8 no longer supports MSVC, so it is built with clang-cl. Linking the
    # result into MSVC-built code is fine: with use_custom_libcxx=false it
    # uses the MSVC STL. Any LLVM install with the standard layout works
    # (lib\clang\<major>\lib\windows\ holds the builtins gn wants).
    candidates = []
    if os.environ.get( "LLVM_DIR" ):
        candidates.append( Path( os.environ[ "LLVM_DIR" ] ) )
    if os.environ.get( "VSINSTALLDIR" ):
        candidates.append( Path( os.environ[ "VSINSTALLDIR" ] ) / "VC" / "Tools" / "Llvm" / "x64" )
    candidates.append( Path( os.environ.get( "ProgramFiles", r"C:\Program Files" ) ) / "LLVM" )
    for candidate in candidates:
        if ( candidate / "bin" / "clang-cl.exe" ).exists():
            return candidate
    nc.abort_op( "No LLVM/clang-cl found. Install the VS 'C++ Clang tools' component or set LLVM_DIR." )

def get_gn_args_file_content() -> str:
    targetarch = get_cpu()

    # Shared by all platforms. The defines that embedders must match end up in
    # the generated v8-gn.h (v8_generate_external_defines_header), which is
    # installed next to the public headers.
    gn_args = f"""
target_cpu="{targetarch}"
v8_target_cpu="{targetarch}"

is_debug=false
is_component_build=false
is_official_build=false
dcheck_always_on=false

is_clang=true
clang_use_chrome_plugins=false
use_custom_libcxx=false
enable_rust=false

# Symbol and debug settings
symbol_level=0
strip_debug_info=true
treat_warnings_as_errors=false

# V8 core settings
v8_monolithic=true
v8_monolithic_for_shared_library=true
v8_generate_external_defines_header=true
v8_use_external_startup_data=false
v8_enable_i18n_support=false
v8_enable_temporal_support=false
v8_enable_webassembly=false
v8_enable_pointer_compression=true
# Must stay off: doctrenderer hands V8 externally allocated ArrayBuffer memory
v8_enable_sandbox=false

v8_enable_test_features=false
v8_enable_backtrace=false
v8_enable_disassembler=false
v8_enable_object_print=false
v8_enable_gdbjit=false
v8_enable_vtunejit=false
"""

    if nc.is_android():
        # Built with the NDK's clang (also used for the host tools such as
        # mksnapshot), against the NDK's libc++ like the rest of core.
        ndk_clang_base = nc.android_toolchain_bin().parent
        gn_args += f"""
target_os="android"
android_ndk_root="{nc.android_ndk_root()}"
android64_ndk_api_level={nc.ANDROID_API_LEVEL}
clang_base_path="{ndk_clang_base}"
clang_version="{_clang_major_version( ndk_clang_base / "bin" / "clang" )}"
use_sysroot=false
use_glib=false
use_lld=true
"""

    elif nc.is_linux():
        clang_base, clang_major = prepare_clang_base()
        gn_args += f"""
target_os="linux"
clang_base_path="{clang_base}"
clang_version="{clang_major}"
use_sysroot=false
use_glib=false
use_lld=true
"""

    elif nc.is_apple_silicon():
        # Uses the system (Xcode command line tools) clang
        clang_base, clang_major = prepare_clang_base()
        gn_args += f"""
target_os="mac"
clang_base_path="{clang_base}"
clang_version="{clang_major}"
"""

    elif nc.is_windows():
        llvm_dir = _find_windows_llvm_dir()
        gn_args += f"""
target_os="win"
clang_base_path="{llvm_dir.as_posix()}"
clang_version="{_clang_major_version( llvm_dir / "bin" / "clang.exe" )}"
"""

    else:
        nc.abort_op( "No gn args prepared for os." )

    return gn_args

def fetch_and_patch():
    nc.create_workdir()
    ensure_python_shim()

    print( "Fetching depot_tools" )
    nc.run_command(
        [ "git", "clone", "https://chromium.googlesource.com/chromium/tools/depot_tools.git", depot_tools_path ],
        "Clone depot_tools"
    )
    nc.run_command(
        [ "git", "checkout", "--force", "--detach", DEPOT_TOOLS_REVISION ],
        "Pin depot_tools",
        depot_tools_path
    )
    ensure_windows_git_shim()

    # Fetch V8 and its DEPS (build/, buildtools/ with gn, third_party/ incl.
    # ninja) at the release tag
    print( f"Fetching v8 { V8_VERSION }" )
    nc.ensure_directory_exists( v8_root_path )
    ( v8_root_path / ".gclient" ).write_text( f"""
solutions = [
    {{
        "name": "v8",
        "url": "https://chromium.googlesource.com/v8/v8.git@refs/tags/{ V8_VERSION }",
        "deps_file": "DEPS",
        "managed": False,
        "custom_deps": {{}},
    }},
]
""" )

    gclient = [ "cmd.exe", "/c", "gclient.bat" ] if nc.is_windows() else [ "gclient" ]
    nc.run_command(
        gclient + [ "sync", "--no-history", "--shallow" ],
        "GClient sync",
        v8_root_path,
        env = depot_env(),
    )

    if nc.is_android():
        disable_android_test_runner()
    elif nc.is_linux():
        disable_crel()

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

def build_and_install():
    nc.create_install_dir()
    ensure_python_shim()

    targetarch = get_cpu()
    output_path = v8_src_path / "out.gn" / f"{targetarch}.release"

    # ensure out dir is clean
    if output_path.exists():
        shutil.rmtree( output_path )
    nc.ensure_directory_exists( output_path )
    ( output_path / "args.gn" ).write_text( get_gn_args_file_content() )

    # gn and ninja come prebuilt with V8's DEPS (CIPD packages for the host)
    exe = ".exe" if nc.is_windows() else ""
    gn_host_dir = "win" if nc.is_windows() else ( "mac" if sys.platform == "darwin" else "linux64" )
    gn_bin_path = v8_src_path / "buildtools" / gn_host_dir / f"gn{exe}"
    ninja_bin_path = v8_src_path / "third_party" / "ninja" / f"ninja{exe}"

    gn_env = depot_env()
    if nc.is_windows():
        gn_env[ "vs2022_install" ] = str( Path( os.environ[ "VSINSTALLDIR" ] ) )

    print( "Running gn gen" )
    nc.run_command(
        [ gn_bin_path, "gen", output_path ],
        "Running gn",
        v8_src_path,
        env = gn_env
    )

    if not ( output_path / "build.ninja" ).exists():
        nc.abort_op( "build.ninja not generated!" )

    job_count = max( os.cpu_count() or 1, 4 ) # at least 4 jobs

    print( "Building v8" )
    nc.run_command(
        [ ninja_bin_path, "-C", output_path, f"-j{job_count}", "v8_monolith" ],
        "Building v8",
        v8_src_path,
        env = gn_env
    )

    # Verify final artifact
    artifact_name = "v8_monolith.lib" if nc.is_windows() else "libv8_monolith.a"
    if not ( output_path / "obj" / artifact_name ).exists():
        nc.abort_op( f"Build completed but { artifact_name } not found" )

    print( "Installing v8" )
    try:
        shutil.copy2( output_path / "obj" / artifact_name, nc.install_dir / artifact_name )
    except Exception:
        nc.abort_op( f"Failed to install { artifact_name }" )

    include_dir = nc.install_dir / "v8" / "include"
    nc.ensure_directory_exists( include_dir )
    try:
        shutil.copytree( v8_src_path / "include", include_dir, dirs_exist_ok = True )
        # The build configuration embedders must match; used with -DV8_GN_HEADER
        shutil.copy2( output_path / "gen" / "include" / "v8-gn.h", include_dir / "v8-gn.h" )
    except Exception as e:
        nc.abort_op( f"Failed to install public headers ({ e })" )

    src = v8_src_path / "src"
    dst = nc.install_dir / "v8" / "src"
    try:
        for file in src.rglob( "*.h" ):
            relative = file.relative_to( src )
            target = dst / relative
            nc.ensure_directory_exists( target.parent )
            shutil.copy2( file, target )
    except Exception as e:
        nc.abort_op( f"Failed to install private headers ({e})" )

    # Create pkg-config file
    pkg_cfg_file = f"prefix={ nc.install_dir }"
    pkg_cfg_file = pkg_cfg_file + f"""
libdir=${{prefix}}
includedir=${{prefix}}/v8/include

Name: V8
Description: V8 JavaScript Engine
Version: { V8_VERSION }
Libs: -L${{libdir}} -lv8_monolith -pthread
Cflags: -I${{includedir}} -DV8_GN_HEADER
"""
    ( nc.install_dir / "v8.pc" ).write_text( pkg_cfg_file )

    nc.create_install_dir_ok_marker()

    print( "Build and install completed" )

def build_all():
    # Everything required to produce the install dir locally. ensure_dep() only
    # calls this when the install dir is neither present locally nor on the
    # remote, so prerequisites and the large, host-specific work-dir fetch are
    # skipped entirely on a cache hit (a download-only machine needs no compiler).
    check_prequisites()

    if not nc.work_dir_looks_ok():
        fetch_and_patch()

    build_and_install()   # ends by creating the install-dir ok-marker


nc.ensure_dep( build_all )
