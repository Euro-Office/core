#!/usr/bin/env python3

import sys
import shutil
import os
import platform
import re
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

nc.work_dir = Path( sys.argv[1] )
nc.install_dir = Path( sys.argv[2] )
nc.force_redo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
nc.dep_name = "V8"
nc.debug_mode = True

depot_tools_path = nc.work_dir / "depot_tools"
v8_root_path = nc.work_dir / "v8"
v8_src_path = v8_root_path / "v8"

gn_source_path = nc.work_dir / "gn-source"

def check_prequisites():
    for tool in [ "git", "python3", "clang", "ninja" ]:
        if shutil.which( tool ) is None:
            nc.abort_op( f"Tool not found: {tool}" )

def apply_patches():
    patches_dir = script_dir / "tools" / "8.9" / "x64-linux-dynamic"

    patches = [
        { "name": "gclient_paths.patch", "dir": depot_tools_path },
        { "name": "jinja2.patch", "dir": v8_src_path / "third_party" / "jinja2" },
        { "name": "buildgn.patch", "dir": v8_src_path },
    ]

    for patch in patches:
        if patch[ "dir" ].is_dir():
            nc.run_command(
                [ "git", "apply", patches_dir / patch[ "name" ] ],
                f"Applying patch: { patch[ "name" ] }",
                patch[ "dir" ]
            )
        else:
            print( f"[WARNING] cannot apply patch ({ patch[ "name" ] }) because dir doesn't exist!" )

def disable_gmock():
    gmock_gn_file_path = v8_src_path / "testing" / "gmock" / "BUILD.gn"
    content = """
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Disabled to avoid visibility issues with gtest_config
# V8 monolithic build doesn't need gmock

import("//build_overrides/build.gni")

group("gmock") {
testonly = true
}

group("gmock_main") {
testonly = true
}
"""

    gmock_gn_file_path.write_text( content )

def disable_cppgc():
    cppgc_gn_file_path = v8_src_path / "src" / "heap" / "cppgc" / "BUILD.gn"
    content = """
# Copyright 2020 the V8 project authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Disabled to avoid ARM64 toolchain issues

import("//build/config/sanitizers/sanitizers.gni")
import("../../gni/v8.gni")

group("cppgc_base") {
  visibility = [ ":*" ]
}

group("cppgc_base_for_testing") {
  testonly = true
  visibility = [ ":*" ]
}
"""

    cppgc_gn_file_path.write_text( content )

def build_gn() -> Path:
    print( "Fetching and building gn" )
    nc.shallow_checkout( gn_source_path, "https://gn.googlesource.com/gn", "281ba2c91861b10fec7407c4b6172ec3d4661243" )

    nc.ensure_directory_exists( gn_source_path / "out" )

    nc.run_command(
        [ "python", "build/gen.py", "--no-last-commit-position" ],
        "Generate GN build",
        cwd = gn_source_path,
        env = {
            "CC": "clang",
            "CXX": "clang++"
        }
    )

    content = """
#pragma once
#define LAST_COMMIT_POSITION_NUM 0
#define LAST_COMMIT_POSITION "0 (unknown)"
"""
    ( gn_source_path / "out" / "last_commit_position.h" ).write_text( content )

    nc.run_command(
        [ "ninja", "-C", "out" ],
        "Building GN",
        cwd = gn_source_path,
        env = {
            "CC": "clang",
            "CXX": "clang++"
        }
    )

    gn_bin_path = v8_src_path / "buildtools" / "linux64" / "gn-built"

    nc.ensure_directory_exists( gn_bin_path )
    try:
        shutil.copy2( gn_source_path / "out" / "gn", gn_bin_path )
    except Exception as e:
        nc.abort_op( f"Copy failed: {e}" )

    return gn_bin_path / "gn"

def get_cpu() -> str:
    targetarch = "Unknown"
    arch = platform.machine()
    if arch == "x86_64":
        targetarch = "x64"
    elif arch == "aarch64" or arch == "arm64":
        targetarch = "arm64"
    else:
        nc.abort_op( f"Unsupported architecture: {arch}" )
    return targetarch

def fetch_and_patch():

    nc.create_workdir()

    # Get depot_tools
    print( "Fetching depot_tool" )
    nc.run_command(
        [ "git", "clone", "https://chromium.googlesource.com/chromium/tools/depot_tools.git", depot_tools_path ],
        "Clone depot_tools"
    )

    # Update depot_tools
    nc.run_command(
        [ "git", "pull", "origin", "main" ],
        "Update depot_tools",
        depot_tools_path,
        error_is_fatal = False
    )

    # Fetch v8
    print( "Fetching v8" )
    nc.run_command( [ "git", "clone", "https://chromium.googlesource.com/v8/v8.git", v8_src_path ], "Clone v8" )
    nc.run_command( [ "git", "checkout", "8.9.45" ], "Git checkout 8.9.45", v8_src_path )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

def build_and_install():
    nc.create_install_dir()

    # Clean gclient state
    files_to_remove = [ ".gclient", ".gclient_entries", "_bad_scm", "chromium.googlesource.com" ]
    for file in files_to_remove:
        Path( v8_root_path / file ).unlink( missing_ok = True )

    # Clean v8 state
    nc.run_command( [ "git", "reset", "--hard" ], "Git reset", v8_src_path )
    nc.run_command( [ "git", "clean", "-fdx" ], "Git clean", v8_src_path )
    nc.run_command( [ "git", "fetch", "origin" ], "Git fetch", v8_src_path )

    # Clean jinja state
    if ( v8_src_path / "third_party" / "jinja2" ).is_dir():
        nc.run_command( [ "git", "reset", "--hard" ], "Git reset (jinja)", v8_src_path / "third_party" / "jinja2" )
        nc.run_command( [ "git", "clean", "-fd" ], "Git clean (jinja)", v8_src_path / "third_party" / "jinja2" )

    # Clean depot tools
    nc.run_command( [ "git", "reset", "--hard" ], "Git reset (depot_tools)", depot_tools_path )
    nc.run_command( [ "git", "clean", "-fd" ], "Git clean (depot_tools)", depot_tools_path )

    # Create gclient config
    content = """
solutions = [
    {
        "name": "v8",
        "url": "https://chromium.googlesource.com/v8/v8.git",
        "deps_file": "DEPS",
        "managed": False,
        "custom_deps": {},
    },
]
target_os = ["linux"]
    """
    Path( v8_root_path / ".gclient" ).write_text(content)

    # Sync v8 dependencies
    print( "Synching v8 dependencies" )
    depot_env = os.environ.copy()
    depot_env["PATH"] = f"{depot_tools_path}:" + depot_env["PATH"]
    depot_env["GCLIENT_SUPPRESS_GIT_VERSION_WARNING"] = "1"
    depot_env["GYP_CHROMIUM_NO_ACTION"] = "1"
    nc.run_command(
        [ "gclient", "sync", "--no-history", "--shallow" ],
        "GClient sync",
        v8_root_path,
        env = depot_env,
    )    

    if nc.is_linux():
        apply_patches()
        disable_gmock()
        disable_cppgc()
        gn_bin_path = build_gn()

        targetarch = get_cpu()
        clang_path = Path(shutil.which("clang"))
        clang_dir = clang_path.parent.parent

        output_path = v8_src_path / "out.gn"/ f"{targetarch}.release"

        # ensure out dir is clean
        try:
            shutil.rmtree( output_path )
        except FileNotFoundError:
            pass

        gn_args=f"""
target_os="linux"
target_cpu="{targetarch}"
v8_target_cpu="{targetarch}"

is_debug=false
is_component_build=false
is_official_build=false

is_clang=true
clang_use_chrome_plugins=false

use_sysroot=false
use_custom_libcxx=false

# Symbol and debug settings
symbol_level=0
strip_debug_info=true
enable_dsyms=false
treat_warnings_as_errors=false

# V8 core settings
v8_monolithic=true
v8_use_external_startup_data=false
v8_enable_i18n_support=false
v8_enable_webassembly=false
v8_enable_pointer_compression=true
v8_enable_sandbox=false

# Disable cppgc to avoid build issues
cppgc_enable_caged_heap=false
v8_enable_conservative_stack_scanning=false
cppgc_is_standalone=false

# Disable all testing infrastructure - CRITICAL for avoiding gmock/gtest issues
v8_enable_test_features=false
v8_enable_verify_heap=false
v8_enable_verify_predictable=false
build_with_chromium=false

# Explicitly disable test targets
v8_enable_backtrace=false
v8_enable_disassembler=false
v8_enable_object_print=false

# Additional stability flags
v8_use_snapshot=true
v8_enable_lazy_source_positions=false
v8_enable_gdbjit=false
v8_enable_vtunejit=false
v8_enable_handle_zapping=false

# Use system toolchain properly
use_gold=false
use_lld=true
"""

        if targetarch == "arm64":
            # Check clang version (it must be 13)
            clang_version_output = nc.capture_process_output( [ "clang", "--version" ] )
            match = re.search( r'\d+\.\d+\.\d+', clang_version_output )
            version = match.group() if match else None

            if not version.startswith( "13." ):
                nc.abort_op( f"Need clang 13 in path. Currently it's: { version }" )

            gn_args = f"""{ gn_args }
cc="clang"
cxx="clang++"
clang_base_path="{ clang_dir }"
"""

        nc.ensure_directory_exists( output_path )
        gn_args_file_path = Path( output_path / "args.gn" )
        gn_args_file_path.write_text( gn_args )

        print( "Running gn gen" )        
        nc.run_command(
            [ gn_bin_path, "gen", output_path ],
            "Running gn",
            v8_src_path
        )

        if not Path( output_path / "build.ninja" ).exists():
            nc.abort_op( "build.ninja not generated!" )

        # Check that the tools actually exist
        for tool in [ "ninja" ]:
            if shutil.which( tool ) is None:
                nc.abort_op( f"Tool not found: {tool}" )

        build_env = {
            "SHELL": "/bin/bash",
        }

        job_count = max( os.cpu_count() or 1, 4 ) # at least 4 jobs

        print( "Building v8" )
        nc.run_command(
            [ "ninja", "-C", output_path, f"-j{job_count}", "v8_monolith" ],
            "Building v8",
            v8_src_path
        )

        # Verify artifacts
        if not ( output_path / "obj" / "libv8_monolith.a" ).exists():
            nc.abort_op( "Build completed but libv8_monolith.a not found" )

        print( "Installing v8" )
        try:
            shutil.copy2( output_path / "obj" / "libv8_monolith.a", nc.install_dir / "libv8_monolith.a" )
        except Exception:
            nc.abort_op( "Failed to install libv8_monolith.a" )

        nc.ensure_directory_exists( nc.install_dir / "v8" / "include" )
        try:
            shutil.copytree( v8_src_path / "include", nc.install_dir / "v8" / "include", dirs_exist_ok = True )
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
        pkg_cfg_file = pkg_cfg_file + """
libdir=${prefix}
includedir=${prefix}/v8/include

Name: V8
Description: V8 JavaScript Engine
Version: 8.9.45
Libs: -L${libdir} -lv8_monolith -pthread
Cflags: -I${includedir}
"""
        ( nc.install_dir / "v8.pc" ).write_text( pkg_cfg_file )
        
    else:
        abort_op( f"Unkown target platform: {sys.platform}" )

    nc.create_install_dir_ok_marker()
    
    print( "Build and install completed" )

check_prequisites()

if not nc.work_dir_looks_ok():
    fetch_and_patch()

if not nc.install_dir_looks_ok():
    if nc.is_windows() and shutil.which("nmake") is None:
        raise RuntimeError(
            "MSVC environment is not set up: 'nmake' not found in PATH.\n"
            "Run 'vcvarsx86_amd64.bat' or use 'x64 Native Tools Command Prompt'."
        )
    build_and_install()
