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
    version = "1"
)

def fetch_and_patch():
    nc.create_workdir()

    # CRYPTOPP_8_9_0, in a "cryptopp" subfolder so that the install dir is the
    # include root for <cryptopp/...>
    nc.shallow_checkout( nc.work_dir / "cryptopp", "https://github.com/weidai11/cryptopp.git", "843d74c7c97f9e19a615b8ff3c0ca06599ca501b" )

    nc.create_work_dir_ok_marker()

    print( "Fetch & patch completed" )

if not nc.work_dir_looks_ok():
    fetch_and_patch()
