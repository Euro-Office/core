#!/usr/bin/env python3

# import os
import sys
import subprocess
import shutil
from pathlib import Path

import build_3rdparty_common as nc

script_path = Path(sys.argv[0]).resolve()
script_dir = script_path.parent


if len( sys.argv ) < 3:
    print( "Needs 2 arguments: work_dir_abs install_dir_abs" )
    sys.exit(1)

work_dir = Path( sys.argv[1] )
install_dir = Path( sys.argv[2] )

if sys.platform == "win32" and shutil.which("nmake") is None:
    raise RuntimeError(
        "MSVC environment is not set up: 'nmake' not found in PATH.\n"
        "Run 'vcvarsx86_amd64.bat' or use 'x64 Native Tools Command Prompt'."
    )

subfolders = [
    'apple',
    'brotli',
    'harfbuzz',
    'html',
    'hyphen',
    'icu',
    'md',
    'openssl',
    'socketio',
    'hunspell',
    'v8',
]

total_time = nc.MeasurementObj( "Total" )

for subfolder in subfolders:
    print(  "---------------------------------------------------------------------------" )
    print( f"Working on {subfolder}..." )
    sub_script = Path( script_dir / subfolder / "nc-build.py" )
    if sub_script.exists():
        try:
            time_meas = nc.MeasurementObj( subfolder )

            subprocess.run(
                [sys.executable, sub_script, work_dir / subfolder, install_dir / subfolder],
                check=True,
                text=True,
                stdout=None,
                stderr=None,
            )
            
        except subprocess.CalledProcessError as e:
            print( f"  ❌ {subfolder} failed with code {e.returncode}" )
            print( f"  { time_meas.elapsed_string() }" )
            time_meas.report()
            sys.exit(e.returncode)

        print( f"  ✅ {subfolder} ready" )
        print( f"  { time_meas.elapsed_string() }" )

    else:
        print( f"❌ { subfolder } build script cannot be found ({ sub_script })" )

print( "" )
total_time.report()
