# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# XlsFormatLib's -Werror=return-type guard must survive EO_NO_WARNINGS (#421).
# Configures the real XlsFormatLib CMake project (third-party build skipped,
# Boost stubbed, nothing is compiled) and checks the flags it would compile with.
# Run: python3 -m unittest discover -s scripts

import json
import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

CORE_ROOT = Path(os.environ.get("EO_CORE_ROOT", Path(__file__).resolve().parent.parent))
XLS_PROJECT = CORE_ROOT / "MsBinaryFile" / "Projects" / "XlsFormatLib" / "Linux" / "cmake"

# Configure only needs find_package(Boost) to succeed.
FIND_BOOST_STUB = """
set(Boost_FOUND TRUE)
foreach(component system filesystem regex date_time)
    if(NOT TARGET Boost::${component})
        add_library(Boost::${component} INTERFACE IMPORTED)
    endif()
endforeach()
"""

# Runs off the end of a non-void function, like the old PIVOTLI::saveContent.
MISSING_RETURN = "int f(int x) { if (x) return 1; }\n"


def compilers():
    found = []
    for cc, cxx in (("gcc", "g++"), ("clang", "clang++")):
        if shutil.which(cc) and shutil.which(cxx):
            found.append((cc, cxx))
    return found


@unittest.skipUnless(shutil.which("cmake"), "cmake not found")
class XlsWerrorGuardTest(unittest.TestCase):
    def configure(self, tmp, cc, cxx, no_warnings):
        stub = tmp / "stub"
        stub.mkdir()
        (stub / "FindBoost.cmake").write_text(FIND_BOOST_STUB)
        build = tmp / "build"
        result = subprocess.run(
            [
                "cmake", "-S", str(XLS_PROJECT), "-B", str(build),
                "-DCMAKE_C_COMPILER=" + cc, "-DCMAKE_CXX_COMPILER=" + cxx,
                "-DEO_NO_WARNINGS=" + ("ON" if no_warnings else "OFF"),
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                "-DTHIRD_PARTY_PREPARED=ON",
                "-DCMAKE_MODULE_PATH=" + str(stub),
            ],
            capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        commands = json.loads((build / "compile_commands.json").read_text())
        self.assertTrue(commands, "no compile commands")
        return [shlex.split(c["command"]) if "command" in c else c["arguments"] for c in commands]

    def missing_return_fails(self, tmp, cxx, args):
        """Compile MISSING_RETURN with the target's warning flags, in order."""
        flags = [a for a in args if a == "-w" or a.startswith("-W") or a.startswith("-std=")]
        source = tmp / "missing_return.cpp"
        source.write_text(MISSING_RETURN)
        # Not -fsyntax-only: GCC checks return-type after parsing.
        result = subprocess.run(
            [cxx, *flags, "-c", str(source), "-o", os.devnull],
            capture_output=True, text=True,
        )
        return result.returncode != 0 and "return-type" in result.stderr, flags, result.stderr

    def check(self, no_warnings):
        if not compilers():
            self.skipTest("no gcc/g++ or clang/clang++ found")
        for cc, cxx in compilers():
            with self.subTest(compiler=cxx), tempfile.TemporaryDirectory() as tmp:
                tmp = Path(tmp)
                commands = self.configure(tmp, cc, cxx, no_warnings)
                for args in commands:
                    source = args[-1]
                    self.assertFalse("-w" in args, f"-w cancels -Werror=return-type: {source}")
                    self.assertTrue("-Werror=return-type" in args, f"-Werror=return-type missing: {source}")
                    if "-Wno-everything" in args:
                        self.assertLess(
                            args.index("-Wno-everything"), args.index("-Werror=return-type"),
                            f"-Wno-everything after -Werror=return-type turns the guard off: {source}",
                        )
                # Every source gets the same warning flags, so one is enough.
                fails, flags, stderr = self.missing_return_fails(tmp, cxx, commands[0])
                self.assertTrue(fails, f"missing return compiled with {flags}:\n{stderr}")

    def test_guard_survives_no_warnings(self):
        self.check(no_warnings=True)

    def test_guard_on_default_build(self):
        self.check(no_warnings=False)


if __name__ == "__main__":
    unittest.main()
