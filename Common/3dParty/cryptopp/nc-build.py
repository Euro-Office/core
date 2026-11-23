#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

import sys
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

third_party_root = ( script_dir / ".." ).resolve()
if str( third_party_root ) not in sys.path:
    sys.path.insert( 0, str( third_party_root ) )
import build_3rdparty_common as nc

# Sources only: core compiles Crypto++ itself (cryptopp/project for the native
# builds, the WASM modules), with CRYPTOPP_DISABLE_ASM from common.cmake.
nc.init_for_dep(
    depname = "CryptoPP",
    workdir = Path( sys.argv[2] ).resolve(), # Work dir is intentionally the same as install dir
    installdir = Path( sys.argv[2] ).resolve(),
    forceredo = len(sys.argv) > 3 and sys.argv[3] == "force-redo",
    version = "2"
)

def patch_msvc_stdext():
    # MSVC 14.51 (Visual Studio 18) removed the stdext namespace, which
    # Crypto++ 8.9.0 still names under CRYPTOPP_MSC_VERSION, so core stops at
    # "'stdext': is not a class or namespace name". Each use only wrapped a raw
    # pointer to quiet C4996; give it the pointer, as every other compiler
    # gets. Bytes, so that a checkout with CRLF endings keeps them.
    for name, old, new in (
        ( "integer.cpp",
          b"stdext::make_checked_array_iterator(block.begin(), block.size())",
          b"block.begin()" ),
        ( "zdeflate.cpp",
          b"(stdext::make_unchecked_array_iterator(scan)+3, stdext::make_unchecked_array_iterator(scanEnd), stdext::make_unchecked_array_iterator(match)+3).first - stdext::make_unchecked_array_iterator(scan));",
          b"(scan+3, scanEnd, match+3).first - scan);" ),
    ):
        path = nc.work_dir / "cryptopp" / name
        text = path.read_bytes()
        if text.count( old ) != 1:
            nc.abort_op( f"Could not find the stdext use in { path }. Did Crypto++ change?" )
        path.write_bytes( text.replace( old, new ) )

def fetch_and_patch():
    nc.create_workdir()

    # CRYPTOPP_8_9_0, in a "cryptopp" subfolder so that the install dir is the
    # include root for <cryptopp/...>
    nc.shallow_checkout( nc.work_dir / "cryptopp", "https://github.com/weidai11/cryptopp.git", "843d74c7c97f9e19a615b8ff3c0ca06599ca501b" )
    patch_msvc_stdext()

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.work_dir_looks_ok():
    fetch_and_patch()
