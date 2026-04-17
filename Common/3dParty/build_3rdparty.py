#!/usr/bin/env python3

# import os
import sys
import subprocess
from pathlib import Path

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent

if len( sys.argv ) < 3:
    print( "Needs 2 arguments: work_dir_abs install_dir_abs" )
    sys.exit(1)

work_dir = Path( sys.argv[1] )
install_dir = Path( sys.argv[2] )

subfolders = [
    # 'apple',
    # 'brotli',
    # 'harfbuzz',
    # 'html',
    # 'hyphen'
    'icu'
    # 'md',
    # 'openssl',
    # 'socket-io'
]

for subfolder in subfolders:
    print( f"Working on {subfolder}..." )
    print( f"  Invoking {str(Path( script_dir / subfolder / "nc-build.py" ))}" )
    sub_script = Path( script_dir / subfolder / "nc-build.py" )
    if sub_script.exists():
        try:
            subprocess.run(
                [sys.executable, sub_script, work_dir / subfolder, install_dir / subfolder],
                check=True,
                text=True
            )
        except subprocess.CalledProcessError as e:
            print(f"\n❌ {subfolder} failed with code {e.returncode}")
            sys.exit(e.returncode)

        print( f"✅ {subfolder} ready" )

    else:
        print( "Not good" )
