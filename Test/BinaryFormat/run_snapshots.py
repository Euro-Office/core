#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Snapshot test for the committed new-document templates (Common/empty/*.bin).

Each template is rendered with binformat_dump.py and compared with a committed
text snapshot under Test/BinaryFormat/snapshots/templates/. Needs no build.
Catches changes to the templates, and label changes caused by edits to the
readers or enum headers.

What x2t writes is covered by the conversion test instead
(Test/Applications/x2tTester/conversionTest.sh): it converts its inputs to
doct/xlst/pptt too and compares their dumps with out_*_snap.

On a mismatch the unified diff is printed and the exit code is 1. To accept an
intended change, rerun with --update and commit the snapshots.

Usage: run_snapshots.py templates [--update]
"""

import argparse
import difflib
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
from binformat_dump import dump_file  # noqa: E402

SNAPSHOTS = HERE / "snapshots"
DIFF_LINES = 200
IN_CI = os.environ.get("GITHUB_ACTIONS") == "true"


def template_cases():
    return [(f"templates/{p.name}.txt", p) for p in sorted((ROOT / "Common/empty").glob("*.bin"))]


def run(cases, update):
    failed = 0
    for rel, binary in cases:
        snap = SNAPSHOTS / rel
        try:
            text = dump_file(binary, ROOT)
        except Exception as e:  # a dump that cannot be produced is a failure, not a crash
            print(f"FAIL {rel}: cannot dump {binary}: {e}")
            failed += 1
            continue
        if update:
            snap.parent.mkdir(parents=True, exist_ok=True)
            if not snap.exists() or snap.read_text(encoding="utf-8") != text:
                snap.write_text(text, encoding="utf-8")
                print(f"updated {rel}")
            continue
        if not snap.exists():
            failed += 1
            print(f"FAIL {rel}: no snapshot yet ({len(text.splitlines())} lines); run with --update and commit it")
            continue
        expected = snap.read_text(encoding="utf-8")
        if expected == text:
            print(f"ok   {rel}")
            continue
        failed += 1
        diff = list(difflib.unified_diff(expected.splitlines(), text.splitlines(),
                                         f"a/{rel}", f"b/{rel}", lineterm="", n=4))
        print(f"FAIL {rel}: binary output changed")
        if IN_CI:
            print(f"::error file=Test/BinaryFormat/snapshots/{rel}::Binary format output changed for {rel}")
        print("\n".join(diff[:DIFF_LINES]))
        if len(diff) > DIFF_LINES:
            print(f"... {len(diff) - DIFF_LINES} more diff lines")
    if failed:
        print(f"\n{failed} binary format snapshot(s) differ. If the change is intended, "
              "update with: Test/BinaryFormat/run_snapshots.py templates --update")
    return 1 if failed else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("suite", choices=["templates"])
    ap.add_argument("--update", action="store_true", help="rewrite the snapshots instead of comparing")
    args = ap.parse_args()
    return run(template_cases(), args.update)


if __name__ == "__main__":
    sys.exit(main())
