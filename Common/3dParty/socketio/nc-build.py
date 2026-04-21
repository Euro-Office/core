#!/usr/bin/env python3

import sys
import shutil
import subprocess
import os
from pathlib import Path

work_dir = Path( sys.argv[1] )
install_dir = Path( sys.argv[2] )
work_dir = install_dir
force_redo = len(sys.argv) > 3 and sys.argv[3] == "force-redo"

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent
patches_dir = script_dir / "patches" / "proper_patches"
debug_mode = True
dep_name = "SocketIO"

def abort_op( message : str, keep_work : bool = False ):
    print( f"Aboring {dep_name}: {message}", file = sys.stderr )
    if not debug_mode:
        try:
            if not keep_work:
                shutil.rmtree( work_dir )
            shutil.rmtree( install_dir )
        except FileNotFoundError:
            pass
    sys.exit( 1 )

def work_dir_looks_ok() -> bool:
    return Path( work_dir / "ok_marker" ).exists()

def fetch_and_patch():
    # If exists and needed, remove work dir
    if work_dir.exists() and ( force_redo or not work_dir_looks_ok() ):
        try:
            shutil.rmtree( work_dir )
        except FileNotFoundError:
            pass

    # Create work dir (if needed)
    if not work_dir.exists():
        try:
            work_dir.mkdir( parents = True )
        except OSError:
            abort_op( "Failed to create work dir" )

    # Init empty git repo
    git_cmd = [
        "git", "init",
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"git init failed: {e.stderr.strip() or e.stdout.strip() or e}" )


    # Add remote
    git_cmd = [
        "git", "remote", "add", "origin",
        "https://github.com/socketio/socket.io-client-cpp.git",
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"Failed to add remote: {e.stderr.strip() or e.stdout.strip() or e}" )

    
    # Fetch
    git_cmd = [
        "git", "fetch",
        "--depth", "1",
        "origin",
        "da779141a7379cc30c870d48295033bc16a23c66",
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"Fetch failed: {e.stderr.strip() or e.stdout.strip() or e}" )



    # Checkout
    git_cmd = [
        "git", "checkout", "FETCH_HEAD",
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"Checkout failed: {e.stderr.strip() or e.stdout.strip() or e}" )


    
    # Update submodules
    git_cmd = [
        "git", "submodule", "update",
        "--init", "--recursive"
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"Submodule update failed: {e.stderr.strip() or e.stdout.strip() or e}" )


    # ASIO checkout
    git_cmd = [
        "git", "checkout", "230c0d2ae035c5ce1292233fcab03cea0d341264",
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir/"lib"/"asio"
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"ASIO checkout failed: {e.stderr.strip() or e.stdout.strip() or e}" )


    # WebSocketPP checkout
    git_cmd = [
        "git", "checkout", "56123c87598f8b1dd471be83ca841ceae07f95ba",
    ]
    try:
        _ = subprocess.run(
            git_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir/"lib"/"websocketpp"
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"WebSocketPP checkout failed: {e.stderr.strip() or e.stdout.strip() or e}" )


    # Patch websocket
    patch_cmd = [
        "git", "apply", patches_dir / "websocketpp.patch",
    ]
    try:
        _ = subprocess.run(
            patch_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir/"lib"/"websocketpp"
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"WebSocketPP patch failed: {e.stderr.strip() or e.stdout.strip() or e}" )


    # Patch internals
    patch_cmds = [
        [ "git", "apply", patches_dir / "sio_client_impl_fail.patch" ],
        [ "git", "apply", patches_dir / "sio_client_impl_open.patch" ],
        [ "git", "apply", patches_dir / "sio_client_impl_close_timeout.patch" ],
    ]
    for i, patch_cmd in enumerate( patch_cmds ):
        try:
            _ = subprocess.run(
                patch_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                cwd=work_dir/"src"/"internal"
            )
        except subprocess.CalledProcessError as e:
            abort_op( f"Internals patch {i} failed: {e.stderr.strip() or e.stdout.strip() or e}" )

    
    # Create no_tls version
    try:
        shutil.copytree( work_dir / "src", work_dir / "src_no_tls" )
    except Exception as e:
        abort_op( f"No TLS copy failed: {e.stderr.strip() or e.stdout.strip() or e}" )

    patch_cmd = [
        "git", "apply", patches_dir / "no_tls.patch",
    ]
    try:
        _ = subprocess.run(
            patch_cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            cwd=work_dir
        )
    except subprocess.CalledProcessError as e:
        abort_op( f"No TLS patch failed: {e.stderr.strip() or e.stdout.strip() or e}" )

    # Create ok marker
    Path( work_dir / "ok_marker" ).touch()

    print( "Fetch & patch completed" )

if not work_dir_looks_ok():
    fetch_and_patch()
