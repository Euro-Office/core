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

nc.init_for_dep(
    depname = "V8",
    workdir = Path( sys.argv[1] ).resolve(),
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"
)

depot_tools_path = nc.work_dir / "depot_tools"
v8_root_path = nc.work_dir / "v8"
v8_src_path = v8_root_path / "v8"

gn_source_path = nc.work_dir / "gn-source"

def check_prequisites():
    tools_needed = [ "git", "python3" ]
    if nc.is_linux() or nc.is_apple_silicon():
        tools_needed.append( "clang" )
        tools_needed.append( "ninja" )
    for tool in tools_needed:
        if shutil.which( tool ) is None:
            nc.abort_op( f"Tool not found: {tool}" )

def _has_pkg_resources( python_bin : str ) -> bool:
    try:
        nc.capture_process_output( [ python_bin, "-c", "import pkg_resources" ] )
        return True
    except Exception:
        return False

def get_gn_hermetic_check_python() -> str:
    # GN's mac build graph (build/mac/should_use_hermetic_xcode.py, pulled in
    # transitively from BUILD.gn) does a module-level "import pkg_resources",
    # which fails on Python installs (e.g. Homebrew's) that don't ship
    # setuptools by default. Rather than installing into the system/Homebrew
    # python (which refuses it anyway as an "externally managed
    # environment"), create a small local venv -- an ordinary "pip install"
    # works fine inside one -- and hand back its python so callers can put it
    # on PATH just for the "gn gen" step.
    base_python = shutil.which( "python" ) or shutil.which( "python3" )
    if base_python is None:
        nc.abort_op( "No python/python3 found in PATH" )

    if _has_pkg_resources( base_python ):
        return base_python

    venv_dir = script_dir / ".mac_pkg_resources_venv"
    venv_python = venv_dir / "bin" / "python3"

    if not venv_python.exists():
        print( "System python is missing pkg_resources (setuptools); creating a local venv for GN's hermetic-xcode check (not touching the system install)..." )
        nc.run_command( [ base_python, "-m", "venv", venv_dir ], "Creating pkg_resources venv" )

    if not _has_pkg_resources( str( venv_python ) ):
        # pkg_resources was removed in setuptools 82.0.0 (released 2026-02-08),
        # so grabbing whatever is "latest" no longer provides it -- pin to a
        # pre-removal version instead.
        nc.run_command(
            [ str( venv_python ), "-m", "pip", "install", "--quiet", "setuptools<82" ],
            "Installing setuptools<82 into venv"
        )

    if not _has_pkg_resources( str( venv_python ) ):
        nc.abort_op( "Failed to set up a python with pkg_resources (setuptools) available for GN's mac build check." )

    return str( venv_python )

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
                f"Applying patch: { patch[ 'name' ] }",
                patch[ "dir" ]
            )
        else:
            print( f"[WARNING] cannot apply patch ({ patch[ 'name' ] }) because dir doesn't exist!" )

    if nc.is_windows():
        for name in ( "win_toolchain.patch", "vs_toolchain.patch" ):
            nc.run_command(
                [ "git", "apply", script_dir / "tools" / "8.9" / "x64-windows" / name ],
                f"Applying patch: {name}",
                v8_src_path / "build"
            )

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


def capture_msvc_env( arch : str ) -> dict:
    # Capture the MSVC environment for `arch` by running vcvarsall.bat in a
    # fresh cmd. Used to build host tools (gn) for x64 even when the job's
    # ambient environment targets arm64.
    vsdir = os.environ.get( "VSINSTALLDIR", "" )
    if not vsdir:
        pf86 = os.environ.get( "ProgramFiles(x86)", r"C:\Program Files (x86)" )
        vswhere = Path( pf86 ) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
        vsdir = nc.capture_process_output(
            [ str( vswhere ), "-latest", "-property", "installationPath" ]
        ).strip()
    vcvarsall = Path( vsdir ) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    if not vcvarsall.exists():
        nc.abort_op( f"vcvarsall.bat not found at { vcvarsall }" )

    marker = "___MSVC_ENV_BELOW___"
    bat = nc.work_dir / f"_capture_env_{ arch }.bat"
    bat.write_text( "\r\n".join( [
        "@echo off",
        'set "VSCMD_VER="',           # force vcvarsall to re-init, not no-op
        'set "INCLUDE="',             # drop the inherited arm64 paths
        'set "LIB="',
        'set "LIBPATH="',
        f'call "{ vcvarsall }" { arch } >nul',
        f"echo { marker }",
        "set",
        "",
    ] ) )

    out = nc.capture_process_output( [ "cmd.exe", "/d", "/c", str( bat ) ] )
    env, seen = {}, False
    for line in out.splitlines():
        if not seen:
            seen = ( line.strip() == marker )
            continue
        if "=" in line:
            k, v = line.split( "=", 1 )
            env[ k ] = v
    return env


def disable_zlib_mac_fdopen_macro():
    # third_party/zlib/zutil.h (pinned to a ~2020 chromium/zlib commit) has a
    # pre-OS X compatibility block gated on "defined(MACOS) ||
    # defined(TARGET_OS_MAC)". TARGET_OS_MAC is true on every Apple platform,
    # including plain macOS, so it always fires there and #defines "fdopen"
    # as a macro that expands to NULL. fdopen genuinely exists on macOS, so
    # this is simply wrong -- and once the macro exists, the *next* time
    # <stdio.h> is parsed in the same translation unit (zutil.c -> zutil.h ->
    # ... -> gzguts.h -> <stdio.h>), the preprocessor mangles the SDK's real
    # "fdopen(int, const char *)" declaration into nonsense, aborting the
    # build with "expected identifier or '('". Strip out just the broken
    # macro definition; leave OS_CODE and everything else untouched.
    zutil_h_path = v8_src_path / "third_party" / "zlib" / "zutil.h"
    text = zutil_h_path.read_text()

    broken_block = """#if defined(MACOS) || defined(TARGET_OS_MAC)
#  define OS_CODE  7
#  ifndef Z_SOLO
#    if defined(__MWERKS__) && __dest_os != __be_os && __dest_os != __win32_os
#      include <unix.h> /* for fdopen */
#    else
#      ifndef fdopen
#        define fdopen(fd,mode) NULL /* No fdopen() */
#      endif
#    endif
#  endif
#endif"""

    fixed_block = """#if defined(MACOS) || defined(TARGET_OS_MAC)
#  define OS_CODE  7
#endif"""

    if broken_block not in text:
        print( "[WARNING] could not find the expected fdopen macro block in zutil.h to remove" )
        return

    zutil_h_path.write_text( text.replace( broken_block, fixed_block ) )

def fix_location_operand_bitfield_ub():
    # src/compiler/backend/instruction.h declares:
    #   enum LocationKind { REGISTER, STACK_SLOT };  // only 2 values, needs 1 bit
    #   using LocationKindField = base::BitField64<LocationKind, 3, 2>;  // allocates 2
    # LocationKind has no fixed underlying type, so its only representable
    # values are those needing <= the minimum number of bits for its
    # enumerators (1 bit here). BitField's kMax for a 2-bit field is 3, and
    # casting 3 to LocationKind is undefined behavior since it's outside that
    # 1-bit range. Newer clang correctly rejects this at compile time
    # ("constexpr variable 'kMax' must be initialized by a constant
    # expression"); older clang silently let it through. V8 fixed this
    # upstream in 2022 (commit d15d49b, "Make bitfields only as wide as
    # necessary for enums") by shrinking several such bitfields -- our v8 is
    # from 2020 and predates that fix. Applying the same targeted correction
    # here (verified against our checkout's actual enum, not just copied
    # from the newer diff, since the surrounding code has since diverged).
    instruction_h_path = v8_src_path / "src" / "compiler" / "backend" / "instruction.h"
    text = instruction_h_path.read_text()

    broken_line = "using LocationKindField = base::BitField64<LocationKind, 3, 2>;"
    fixed_line = "using LocationKindField = base::BitField64<LocationKind, 3, 1>;"

    if broken_line not in text:
        print( "[WARNING] could not find the expected LocationKindField declaration in instruction.h" )
        return

    instruction_h_path.write_text( text.replace( broken_line, fixed_line ) )

def fix_oversized_enum_bitfields():
    # Same underlying issue as fix_location_operand_bitfield_ub(), found by
    # scanning every BitField<Enum, shift, size> declaration in the checkout
    # and cross-checking "size" against each enum's actual value range (none
    # of these enums have a fixed underlying type, so the valid range for
    # casting an integer to them is only as wide as their largest enumerator
    # requires -- not whatever the declared bitfield width happens to be).
    # Each entry below was individually verified against this checkout's real
    # enum definitions, not copied from a diff:
    #   - wasm-code-manager.h Kind: 4 values (kFunction..kJumpTable) need 2
    #     bits, field declared with 3 -- this is the one from the actual
    #     ninja error.
    #   - instruction-codes.h AddressingMode: this enum's value list is
    #     entirely architecture-specific (ADDRESSING_MODE_LIST = kMode_None +
    #     TARGET_ADDRESSING_MODE_LIST, and each arch's instruction-codes-*.h
    #     defines its own TARGET_ADDRESSING_MODE_LIST). On arm64 there are 13
    #     total values (needs 4 bits) but the shared field is declared with 5
    #     -- a real bug there. On x64 there are 20 total values (19 target
    #     modes + kMode_None), which needs exactly 5 bits, so the declared
    #     width is already correct on x64 and must NOT be narrowed to 4 there
    #     (doing so would silently break x64 codegen instead of failing to
    #     compile, since 20 values don't fit in 4 bits). So this one specific
    #     fix is gated on the target architecture; the other two below are
    #     architecture-independent and always safe to apply.
    #   - profile-generator.h CodeEventListener::LogEventsAndTags: 22 list
    #     entries + the NUMBER_OF_LOG_EVENTS sentinel = 23 values, needing 5
    #     bits, field declared with 8. Lower confidence this template
    #     actually gets instantiated in our build, but the fix is isolated
    #     and free either way, and this enum's value list doesn't vary by
    #     architecture.
    # In every case only the bitfield's "size" template argument is narrowed;
    # "shift" is left untouched, so no other field's bit position moves.
    targetarch = get_cpu()

    fixes = [
        {
            "path": v8_src_path / "src" / "wasm" / "wasm-code-manager.h",
            "broken": "using KindField = base::BitField8<Kind, 0, 3>;",
            "fixed": "using KindField = base::BitField8<Kind, 0, 2>;",
        },
        {
            "path": v8_src_path / "src" / "profiler" / "profile-generator.h",
            "broken": "using TagField = base::BitField<CodeEventListener::LogEventsAndTags, 0, 8>;",
            "fixed": "using TagField = base::BitField<CodeEventListener::LogEventsAndTags, 0, 5>;",
        },
    ]

    if targetarch == "arm64":
        fixes.append( {
            "path": v8_src_path / "src" / "compiler" / "backend" / "instruction-codes.h",
            "broken": "using AddressingModeField = base::BitField<AddressingMode, 9, 5>;",
            "fixed": "using AddressingModeField = base::BitField<AddressingMode, 9, 4>;",
        } )

    for fix in fixes:
        text = fix[ "path" ].read_text()
        if fix[ "broken" ] not in text:
            print( f"[WARNING] could not find expected bitfield declaration in { fix[ 'path' ] }" )
            continue
        fix[ "path" ].write_text( text.replace( fix[ "broken" ], fix[ "fixed" ] ) )


def build_gn() -> Path:
    print( "Fetching and building gn" )
    nc.shallow_checkout( gn_source_path, "https://gn.googlesource.com/gn", "281ba2c91861b10fec7407c4b6172ec3d4661243" )

    nc.ensure_directory_exists( gn_source_path / "out" )

    env = {
        "CC": "clang",
        "CXX": "clang++"
    } if nc.is_linux() or nc.is_apple_silicon() else {
        "CXXFLAGS": "/FIstring",
        "CFLAGS": "/FIstring",
    }

    if nc.is_windows():
        # gn is a HOST tool -> build it with the x64 host toolchain even inside
        # an arm64 cross job. Otherwise cl.exe targets arm64, gn's build_config.h
        # #errors, and the binary couldn't run on the x64 host anyway.
        env = capture_msvc_env( "x64" ) | env

    nc.run_command(
        [ "python", "build/gen.py", "--no-last-commit-position" ],
        "Generate GN build",
        cwd = gn_source_path,
        env = env
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
        env = env
    )

    gn_bin_name = "gn.exe" if nc.is_windows() else "gn"
    gn_bin_path = v8_src_path / "buildtools" / "linux64" / "gn-built"

    nc.ensure_directory_exists( gn_bin_path )

    try:
        shutil.copy2( gn_source_path / "out" / gn_bin_name, gn_bin_path )
    except Exception as e:
        nc.abort_op( f"Failed to copy gn binary: {e}" )

    return gn_bin_path / gn_bin_name

def get_cpu() -> str:
    # The arch we are *building for*; detection (incl. the Windows MSVC target
    # arch and the amd64_arm64 cross prompt) lives in build_3rdparty_common.
    arch = nc.target_arch()
    if arch not in ( "x64", "arm64" ):
        nc.abort_op( f"Unsupported architecture for V8: {arch!r}" )
    return arch

def get_gn_args_file_content() -> str:
    targetarch = get_cpu()

    if nc.is_linux():
        clang_path = Path(shutil.which("clang"))
        clang_dir = clang_path.parent.parent
        use_bundled_clang = os.environ.get("V8_USE_BUNDLED_CLANG", "false").lower() == "true"

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

        if targetarch == "arm64" or ( not use_bundled_clang ):
            gn_args += f"""
clang_base_path="{clang_dir}"
cc="clang"
cxx="clang++"
"""

        return gn_args

    elif nc.is_apple_silicon():
        clang_path = Path(shutil.which("clang"))
        clang_dir = clang_path.parent.parent

        gn_args=f"""
target_os="mac"
target_cpu="{targetarch}"
v8_target_cpu="{targetarch}"

is_debug=false
is_component_build=false
is_official_build=false

is_clang=true
clang_use_chrome_plugins=false

use_custom_libcxx=false

# Symbol and debug settings
symbol_level=0
strip_debug_info=true
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

# Use system (Xcode command line tools) clang
clang_base_path="{clang_dir}"
cc="clang"
cxx="clang++"
"""

        return gn_args

    elif nc.is_windows():
        gn_args=f"""
target_os="win"
target_cpu="{targetarch}"
v8_target_cpu="{targetarch}"

is_debug=false
is_component_build=false
is_official_build=false

is_clang=false

use_custom_libcxx=false

# Symbol and debug settings
symbol_level=0
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
"""
        return gn_args
    
    else:
        nc.abort_op( "No gn args prepared for os." )
        return "No gn args prepared for os."
    
def _remove_deps_entry( deps_text : str, key : str ) -> str:
    # Removes a "'key': { ... }," entry from a DEPS file's deps dict by
    # bracket-matching (rather than regex), since the value is a nested dict
    # that may itself contain "}," substrings.
    marker = f"'{key}':"
    start = deps_text.find( marker )
    if start == -1:
        return deps_text

    brace_start = deps_text.find( "{", start )
    depth = 0
    i = brace_start
    while i < len( deps_text ):
        if deps_text[ i ] == "{":
            depth += 1
        elif deps_text[ i ] == "}":
            depth -= 1
            if depth == 0:
                break
        i += 1
    end = i + 1

    while end < len( deps_text ) and deps_text[ end ] in ",\n \t":
        end += 1

    line_start = deps_text.rfind( "\n", 0, start ) + 1
    return deps_text[ :line_start ] + deps_text[ end: ]

def _remove_hook_entry( deps_text : str, hook_name : str ) -> str:
    # Same idea as _remove_deps_entry, but for an entry in the "hooks" list
    # (matched by its "'name': '<hook_name>'" line, then walking outward to
    # find that entry's enclosing "{ ... }," dict).
    marker = f"'name': '{hook_name}'"
    marker_pos = deps_text.find( marker )
    if marker_pos == -1:
        return deps_text

    open_pos = deps_text.rfind( "{", 0, marker_pos )
    if open_pos == -1:
        return deps_text

    depth = 0
    i = open_pos
    while i < len( deps_text ):
        if deps_text[ i ] == "{":
            depth += 1
        elif deps_text[ i ] == "}":
            depth -= 1
            if depth == 0:
                break
        i += 1
    end = i + 1

    while end < len( deps_text ) and deps_text[ end ] in ",\n \t":
        end += 1

    line_start = deps_text.rfind( "\n", 0, open_pos ) + 1
    return deps_text[ :line_start ] + deps_text[ end: ]

def disable_mac_toolchain_hook():
    # v8's DEPS runs a "mac_toolchain" hook (build/mac_toolchain.py) to fetch
    # Google's own hermetic/pinned Xcode. That script does "import
    # pkg_resources" at module scope, which fails on newer Python installs
    # that don't ship setuptools by default. We're intentionally using the
    # already-installed system Xcode Command Line Tools clang (see
    # clang_base_path in get_gn_args_file_content()), so this hook isn't
    # needed at all here -- strip it out rather than fight the Python env
    # depot_tools' hook runner uses.
    deps_file_path = v8_src_path / "DEPS"
    text = deps_file_path.read_text()
    new_text = _remove_hook_entry( text, "mac_toolchain" )
    if new_text == text:
        print( "[WARNING] could not find mac_toolchain hook in DEPS to remove" )
    else:
        deps_file_path.write_text( new_text )

def disable_luci_go_cipd_dep():
    # v8's DEPS pins the "tools/luci-go" cipd dep (isolate/isolated/swarming
    # binaries, only used to run V8's test suite on Swarming/Isolate infra) to
    # a git_revision that predates mac-arm64 builds of those packages, so
    # "gclient sync" aborts trying to resolve it on Apple Silicon
    # ("no such tag"). Not needed to build v8_monolith, so strip it out of
    # DEPS before syncing, rather than relying on custom_deps (which doesn't
    # seem to suppress this particular cipd-type dependency).
    deps_file_path = v8_src_path / "DEPS"
    text = deps_file_path.read_text()
    new_text = _remove_deps_entry( text, "tools/luci-go" )
    if new_text == text:
        print( "[WARNING] could not find tools/luci-go dep in DEPS to remove" )
    else:
        deps_file_path.write_text( new_text )

def create_fake_pipes_shim() -> Path:
    shims_path = nc.work_dir / "win_python_shims"
    shim_file_path = shims_path / "sitecustomize.py"
    nc.ensure_directory_exists( shims_path )
    content = """
import sys
import shlex

# Fake the missing Unix module
class PipesModule:
    @staticmethod
    def quote(s):
        return shlex.quote(s)

sys.modules["pipes"] = PipesModule()
"""
    shim_file_path.write_text( content )
    return shims_path

def ensure_windows_git_shim():
    # depot_tools on Windows normally bootstraps its own bundled git the first
    # time gclient runs. DEPOT_TOOLS_UPDATE=0 (set in the depot env below for
    # reproducibility) suppresses that bootstrap, but gclient's git_cache
    # invokes "git.bat" and only catches CalledProcessError - the missing shim
    # crashes the sync with an unhandled FileNotFoundError. Point the shim at
    # the system git this script already depends on.
    #
    # The load-bearing call is the one in build_and_install(): that is where
    # gclient sync actually runs, and build_all() reaches it on every build,
    # whereas fetch_and_patch() is skipped whenever the work dir already looks
    # usable. The fetch_and_patch() call simply leaves a freshly cloned and
    # pinned depot_tools self-consistent.
    #
    # It is not about `git clean -fd` undoing the shim: depot_tools gitignores
    # /git.bat ("Ignore the batch files produced by the Windows bootstrapping"),
    # and clean leaves ignored files alone without -x.
    if not nc.is_windows():
        return
    shim = depot_tools_path / "git.bat"
    if shim.exists():
        return
    git_exe = shutil.which("git")
    if git_exe is None:
        nc.abort_op("Tool not found: git")
    shim.write_text("@echo off\r\n\"" + git_exe + "\" %*\r\n")


def fetch_and_patch():
    nc.create_workdir()

    # Get depot_tools
    print( "Fetching depot_tool" )
    nc.run_command(
        [ "git", "clone", "https://chromium.googlesource.com/chromium/tools/depot_tools.git", depot_tools_path ],
        "Clone depot_tools"
    )

    # Pin depot_tools to a known-good revision instead of tracking HEAD.
    # depot_tools main moves and gclient_paths.patch is written against one specific
    # revision of gclient_paths.py: f065bb3b0 (2026-07-13, "Add gclient getconfig
    # subcommand") added a fifth @functools.lru_cache site, and 93974d014 (2026-07-21,
    # ruff reformat) switched the file to double-quoted strings. Both invalidate the
    # patch context. f394ab2c9 (2026-07-24) is the newest revision the current patch
    # applies against, verified with `git apply --check`, and predates the depot_tools
    # UV migration (db395c47f, 2026-08-02) which is not yet build-verified for V8 8.9.
    # Keep this revision in sync with tools/8.9/*/nc-build.sh; when refreshing the
    # patch, bump all three together.
    nc.run_command(
        [ "git", "fetch", "--quiet", "origin" ],
        "Fetch depot_tools",
        depot_tools_path,
        error_is_fatal = False
    )
    nc.run_command(
        [ "git", "checkout", "--force", "--detach", "f394ab2c993283e94680ca13db98b99927868e98" ],
        "Pin depot_tools to known-good revision",
        depot_tools_path
    )
    ensure_windows_git_shim()

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

    if nc.is_apple_silicon():
        disable_luci_go_cipd_dep()
        disable_mac_toolchain_hook()

    # Clean jinja state
    if ( v8_src_path / "third_party" / "jinja2" ).is_dir():
        nc.run_command( [ "git", "reset", "--hard" ], "Git reset (jinja)", v8_src_path / "third_party" / "jinja2" )
        nc.run_command( [ "git", "clean", "-fd" ], "Git clean (jinja)", v8_src_path / "third_party" / "jinja2" )

    # Clean depot tools
    nc.run_command( [ "git", "reset", "--hard" ], "Git reset (depot_tools)", depot_tools_path )
    nc.run_command( [ "git", "clean", "-fd" ], "Git clean (depot_tools)", depot_tools_path )
    ensure_windows_git_shim()

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
    """
    Path( v8_root_path / ".gclient" ).write_text(content)

    # Sync v8 dependencies
    print( "Synching v8 dependencies" )
    depot_env = os.environ.copy()
    depot_env["PATH"] = f"{depot_tools_path}{os.pathsep}" + depot_env["PATH"]
    depot_env["GCLIENT_SUPPRESS_GIT_VERSION_WARNING"] = "1"
    depot_env["GYP_CHROMIUM_NO_ACTION"] = "1"
    depot_env["DEPOT_TOOLS_WIN_TOOLCHAIN"] = "0"
    # Keep depot_tools from self-updating to HEAD during sync, which would undo
    # the pin in fetch_and_patch() and re-break gclient_paths.patch.
    depot_env["DEPOT_TOOLS_UPDATE"] = "0"

    if nc.is_windows():
        fake_pipes_shim_path = create_fake_pipes_shim()
        depot_env[ "PYTHONPATH" ] = str( fake_pipes_shim_path )

    
    # Since I'm patching v8/build, I need to reset it before sync (if already exists)
    if ( v8_src_path / "build" ).is_dir():
        nc.run_command(
            [ "git", "reset", "--hard" ],
            "Hard reset v8/build",
            v8_src_path / "build"
        )

    if nc.is_linux() or nc.is_windows() or nc.is_apple_silicon():
        if nc.is_windows():
            nc.run_command(
                [ "cmd.exe", "/c", "gclient.bat", "sync", "--no-history", "--shallow" ],
                "GClient sync",
                v8_root_path,
                env = depot_env,
            )
        else: # linux or apple silicon
            nc.run_command(
                [ "gclient", "sync", "--no-history", "--shallow" ],
                "GClient sync",
                v8_root_path,
                env = depot_env,
            )

        apply_patches()
        disable_gmock()
        disable_cppgc()
        fix_location_operand_bitfield_ub()
        fix_oversized_enum_bitfields()
        if nc.is_apple_silicon():
            disable_zlib_mac_fdopen_macro()
        gn_bin_path = build_gn()

        targetarch = get_cpu()
        output_path = v8_src_path / "out.gn"/ f"{targetarch}.release"

        # ensure out dir is clean
        try:
            shutil.rmtree( output_path )
        except FileNotFoundError:
            pass

        gn_args = get_gn_args_file_content()

        if targetarch == "arm64" and nc.is_linux():
            # Check clang version (it must be 13). This constraint is specific
            # to the arm64-linux-dynamic cross-compile toolchain; it doesn't
            # apply to native Apple Silicon builds, which use whatever
            # (much newer) clang Xcode/Homebrew provides.
            clang_version_output = nc.capture_process_output( [ "clang", "--version" ] )
            match = re.search( r'\d+\.\d+\.\d+', clang_version_output )
            version = match.group() if match else None

            if not version.startswith( "13." ):
                nc.abort_op( f"Need clang 13 in path. Currently it's: { version }" )

        nc.ensure_directory_exists( output_path )
        gn_args_file_path = Path( output_path / "args.gn" )
        gn_args_file_path.write_text( gn_args )

        print( "Running gn gen" )
        gn_rt_env = { "PYTHONPATH": "" }
        if nc.is_windows():
            gn_rt_env[ "PYTHONPATH" ] = str( fake_pipes_shim_path )
            gn_rt_env[ "DEPOT_TOOLS_WIN_TOOLCHAIN" ] = "0"
            gn_rt_env[ "vs2022_install" ] = str( Path( os.environ[ "VSINSTALLDIR" ] ) )

        if nc.is_apple_silicon():
            # Make sure the bare "python"/"python3" that GN's exec_script()
            # calls resolve to an interpreter with pkg_resources available
            # (see get_gn_hermetic_check_python()), without touching the
            # system python.
            hermetic_check_python_dir = Path( get_gn_hermetic_check_python() ).parent
            gn_rt_env[ "PATH" ] = f"{ hermetic_check_python_dir }{ os.pathsep }{ os.environ[ 'PATH' ] }"

        nc.run_command(
            [ gn_bin_path, "gen", output_path ],
            "Running gn",
            v8_src_path,
            env = gn_rt_env
        )

        if not Path( output_path / "build.ninja" ).exists():
            nc.abort_op( "build.ninja not generated!" )

        # Check that the tools actually exist
        for tool in [ "ninja" ]:
            if shutil.which( tool ) is None:
                nc.abort_op( f"Tool not found: {tool}" )

        job_count = max( os.cpu_count() or 1, 4 ) # at least 4 jobs
        if nc.is_windows():
            # On Windows, MSVC is more likely to run out of memory if it uses too many workers. TODO: Maybe we could set an optimal job count based on free memory and cpu core count.
            job_count = 4

        print( "Building v8" )
        env = {
            "CC": "clang",
            "CXX": "clang++"
        } if nc.is_linux() or nc.is_apple_silicon() else {
            # "CXXFLAGS": "/FIstring /Zm300",
            # "CFLAGS": "/FIstring /Zm300",
            "CL": "/FIstring /Zm300",
        }
        nc.run_command(
            [ "ninja", "-C", output_path, f"-j{job_count}", "v8_monolith" ],
            "Building v8",
            v8_src_path,
            env=env
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
        nc.abort_op( f"Unkown target platform: {sys.platform}" )

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

    if nc.is_windows() and shutil.which( "nmake" ) is None:
        raise RuntimeError(
            "MSVC environment is not set up: 'nmake' not found in PATH.\n"
            "Run 'vcvarsx86_amd64.bat' or use 'x64 Native Tools Command Prompt'."
        )

    build_and_install()   # ends by creating the install-dir ok-marker


nc.ensure_dep( build_all )