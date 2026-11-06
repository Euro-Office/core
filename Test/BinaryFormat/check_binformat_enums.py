#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Fail when a change breaks the numeric contract of the internal binary format.

Compares every enumerator, integer #define and integer const in the
BinReaderWriterDefines.h headers between a base git revision and the working
tree. The binary format is append-only:

  * an existing name must keep its value,
  * an existing name must not be removed,
  * a new name must not reuse a value already taken in the same enum,
  * format version constants may only go up (reported as a notice).

Adding new names with fresh values is always fine. A deliberate break (with a
version bump and a matching sdkjs change) can be let through with
--allow-breaking, which CI sets when the PR carries the "binary-format-break"
label; the report is printed either way.

Usage: check_binformat_enums.py --base origin/main [--root <core>] [--allow-breaking]
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from binformat_defines import DEFINES_HEADERS, VERSION_CONSTANTS, ParseError, parse_defines  # noqa: E402

IN_CI = os.environ.get("GITHUB_ACTIONS") == "true"


def git_show(root, ref, path):
    r = subprocess.run(["git", "-C", str(root), "show", f"{ref}:{path}"],
                       capture_output=True, text=True, encoding="utf-8", errors="replace")
    return r.stdout if r.returncode == 0 else None


class Report:
    def __init__(self):
        self.errors = 0

    def _emit(self, level, path, line, msg):
        if IN_CI:
            loc = f" file={path},line={line}" if line else f" file={path}"
            print(f"::{level}{loc}::{msg}")
        else:
            print(f"{level.upper()}: {path}:{line or '-'}: {msg}")

    def error(self, path, line, msg):
        self.errors += 1
        self._emit("error", path, line, msg)

    def notice(self, path, line, msg):
        self._emit("notice", path, line, msg)


def compare(fmt, path, base, head, report):
    for gname, bg in base.items():
        hg = head.get(gname)
        for name, bval in bg.members.items():
            ident = f"{fmt} {gname}::{name}" if bg.kind == "enum" else f"{fmt} {name}"
            if hg is None or name not in hg.members:
                report.error(path, None, f"{ident} (= {bval}) was removed. Binary format values are "
                                         "append-only: stored files and sdkjs still use it.")
                continue
            hval = hg.members[name]
            if hval == bval:
                continue
            line = hg.lines.get(name)
            if name in VERSION_CONSTANTS:
                if hval > bval:
                    report.notice(path, line, f"{ident} bumped {bval} -> {hval}. Keep a reader branch "
                                              f"for v{bval} files and land the matching sdkjs change.")
                else:
                    report.error(path, line, f"{ident} went down {bval} -> {hval}.")
                continue
            report.error(path, line, f"{ident} changed value {bval} -> {hval}. Existing values must not "
                                     "change: files written with the old value would be misread.")

    for gname, hg in head.items():
        if hg.kind != "enum":
            continue  # #define/const groups mix unrelated constants; duplicates are normal there.
        bg = base.get(gname)
        old_names = set(bg.members) if bg else set()
        taken = {}
        for name, val in hg.members.items():
            if name in old_names:
                taken.setdefault(val, name)
        for name, val in hg.members.items():
            if name in old_names:
                continue
            if val in taken:
                report.error(path, hg.lines.get(name),
                             f"{fmt} {gname}::{name} (new) reuses value {val} of {taken[val]}. "
                             "Readers cannot tell the two apart; pick an unused value.")
            else:
                taken[val] = name


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base", required=True, help="git revision to compare against, e.g. origin/main")
    ap.add_argument("--root", default=Path(__file__).resolve().parents[2], type=Path,
                    help="core checkout (default: this script's repository)")
    ap.add_argument("--allow-breaking", action="store_true",
                    help="report breaking changes but exit 0 (deliberate, versioned format change)")
    args = ap.parse_args()

    report = Report()
    total = 0
    for fmt, rel in DEFINES_HEADERS.items():
        head_file = args.root / rel
        base_text = git_show(args.root, args.base, rel)
        if not head_file.exists():
            if base_text is not None:
                report.error(rel, None, f"{fmt} format header was removed or moved; update DEFINES_HEADERS "
                                        "in Test/BinaryFormat/binformat_defines.py if it moved.")
            continue
        try:
            head = parse_defines(head_file.read_text(encoding="utf-8", errors="replace"))
        except ParseError as e:
            report.error(rel, None, f"cannot parse header: {e}")
            continue
        total += sum(len(g.members) for g in head.values())
        if base_text is None:
            print(f"{rel}: not present in {args.base}, nothing to compare")
            continue
        try:
            base = parse_defines(base_text)
        except ParseError as e:
            print(f"{rel}: base revision does not parse ({e}); skipping comparison")
            continue
        compare(fmt, rel, base, head, report)

    if report.errors:
        print(f"\n{report.errors} breaking change(s) to the binary format constants "
              f"({total} constants checked against {args.base}).")
        if args.allow_breaking:
            print("Allowed by --allow-breaking (PR label 'binary-format-break').")
            return 0
        print("If this break is intended: bump the format version, keep reading the old version, "
              "land the matching sdkjs change, and label the PR 'binary-format-break'.")
        return 1
    print(f"OK: {total} binary format constants checked against {args.base}, no breaking changes.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
