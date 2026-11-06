#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Snapshot tests for the internal binary format (DOCY/XLSY/PPTY, "Editor.bin").

Each case is a binary file rendered with binformat_dump.py and compared with
a committed text snapshot under Test/BinaryFormat/snapshots/.

  templates   the committed new-document templates in Common/empty/*.bin.
              Needs no build. Catches changes to the templates, and label
              changes caused by edits to the readers or enum headers.

  x2t         the x2tTester input documents (docx/odt/rtf, xlsx/ods,
              pptx/odp), converted by x2ttester/x2t from a build package to
              doct/xlst/pptt (a zip of Editor.bin plus media). Catches any
              change in what x2t writes for sdkjs.

On a mismatch the unified diff is printed and the exit code is 1. New dumps are
always written to --out, so CI can upload them. To accept an intended change,
rerun with --update, or copy the uploaded dumps into the snapshot folder.

Usage:
  run_snapshots.py templates [--update]
  run_snapshots.py x2t --package build/package [--update] [--missing fail|warn] [--print-new]
  run_snapshots.py --import-log job-log.txt     # take printed snapshots from a CI log
"""

import argparse
import base64
import difflib
import hashlib
import io
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
from binformat_dump import dump  # noqa: E402

SNAPSHOTS = HERE / "snapshots"
TESTER_DIR = ROOT / "Test/Applications/x2tTester"
X2T_INPUTS = ["input_word_proc", "input_sheets", "input_slides"]
X2T_OUTPUT_EXTS = ("doct", "xlst", "pptt")
DIFF_LINES = 200
IN_CI = os.environ.get("GITHUB_ACTIONS") == "true"
LOG_BEGIN = "----- BEGIN binformat snapshots (tar.gz, base64) -----"
LOG_END = "----- END binformat snapshots -----"


def template_cases():
    return [(f"templates/{p.name}.txt", p) for p in sorted((ROOT / "Common/empty").glob("*.bin"))]


def x2t_cases(package, work):
    """Convert the x2tTester inputs to Editor.bin with x2ttester; return (snapshot, file) pairs."""
    package = Path(package).resolve()
    tester = package / "x2ttester"
    if not tester.exists():
        sys.exit(f"x2ttester not found in {package}")
    inputs = work / "input"
    output = work / "output"
    inputs.mkdir(parents=True)
    names = {}
    for sub in X2T_INPUTS:
        for f in sorted((TESTER_DIR / sub).iterdir()):
            if f.is_file():
                shutil.copy2(f, inputs / f.name)
                names[f.name] = sub.removeprefix("input_")
    config = work / "config.xml"
    config.write_text(
        '<?xml version="1.0" encoding="utf-8"?>\n<settings>\n'
        f"  <reportPath>{work / 'report'}</reportPath>\n"
        f"  <inputDirectory>{inputs}</inputDirectory>\n"
        f"  <outputDirectory>{output}</outputDirectory>\n"
        f"  <x2tPath>{package}/</x2tPath>\n"
        f"  <output>{' '.join(X2T_OUTPUT_EXTS)}</output>\n"
        "  <cores>2</cores>\n  <timestamp>0</timestamp>\n</settings>\n", encoding="utf-8")
    r = subprocess.run([str(tester), str(config)], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-4000:], r.stderr[-4000:], sep="\n")
        sys.exit(f"x2ttester failed with exit code {r.returncode}")
    produced = {}
    for ext in X2T_OUTPUT_EXTS:
        for f in output.rglob(f"*.{ext}"):
            # x2ttester writes <output>/<input file name>/<stem>.<ext>
            produced[f.parent.name] = f
    cases = []
    for name, group in sorted(names.items()):
        cases.append((f"x2t/{group}/{name}.txt", produced.get(name)))
    return cases


def render(path):
    """Dump a .bin, or a .doct/.xlst/.pptt package (zip of Editor.bin plus media)."""
    if path.suffix.lstrip(".") not in X2T_OUTPUT_EXTS:
        return dump(path.read_bytes(), ROOT)
    with zipfile.ZipFile(path) as z:
        text = dump(z.read("Editor.bin"), ROOT)
        others = [i for i in z.infolist() if i.filename != "Editor.bin" and not i.is_dir()]
        if others:
            text += "package files:\n"
            for i in sorted(others, key=lambda i: i.filename):
                digest = hashlib.sha256(z.read(i)).hexdigest()[:16]
                text += f"  {i.filename} len={i.file_size} sha256={digest}\n"
    return text


def normalise(text, work):
    # Paths of this run's scratch folder must not leak into snapshots.
    return text.replace(str(work), "<WORK>") if work else text


def print_bundle(texts):
    """Print snapshots as a base64 tar.gz block, for when CI artifacts cannot be downloaded."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:gz") as tar:
        for rel, text in sorted(texts.items()):
            data = text.encode("utf-8")
            info = tarfile.TarInfo(rel)
            info.size = len(data)
            tar.addfile(info, io.BytesIO(data))
    print(LOG_BEGIN)
    print(base64.encodebytes(buf.getvalue()).decode("ascii"), end="")
    print(LOG_END)


def import_bundle(log_path):
    """Write the snapshots from a block printed by --print-new (a job log may prefix timestamps)."""
    lines, inside = [], False
    for line in Path(log_path).read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.lstrip("\ufeff")  # GitHub starts each stitched log chunk with a BOM
        body = line.split(" ", 1)[1] if line[:4].isdigit() and " " in line else line
        if body == LOG_BEGIN:
            lines, inside = [], True
        elif body == LOG_END and inside:
            break
        elif inside:
            lines.append(body.strip())
    else:
        sys.exit(f"no complete snapshot block in {log_path}")
    with tarfile.open(fileobj=io.BytesIO(base64.b64decode("".join(lines))), mode="r:gz") as tar:
        for member in tar.getmembers():
            target = (SNAPSHOTS / member.name).resolve()
            if not member.isfile() or SNAPSHOTS.resolve() not in target.parents:
                sys.exit(f"refusing to write {member.name}")
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(tar.extractfile(member).read())
            print(f"imported {member.name}")
    return 0


def run(cases, update, missing, out_dir, work=None, print_new=False):
    failed = new = 0
    new_texts = {}
    for rel, binary in cases:
        snap = SNAPSHOTS / rel
        if binary is None:
            print(f"FAIL {rel}: x2t produced no Editor.bin for this input")
            failed += 1
            continue
        try:
            text = normalise(render(binary), work)
        except Exception as e:  # a dump that cannot be produced is a failure, not a crash
            print(f"FAIL {rel}: cannot dump {binary}: {e}")
            failed += 1
            continue
        if out_dir:
            (out_dir / rel).parent.mkdir(parents=True, exist_ok=True)
            (out_dir / rel).write_text(text, encoding="utf-8")
        if update:
            snap.parent.mkdir(parents=True, exist_ok=True)
            if not snap.exists() or snap.read_text(encoding="utf-8") != text:
                snap.write_text(text, encoding="utf-8")
                print(f"updated {rel}")
            continue
        if not snap.exists():
            new += 1
            new_texts[rel] = text
            msg = f"{rel}: no snapshot yet ({len(text.splitlines())} lines); run with --update and commit it"
            if missing == "fail":
                failed += 1
                print(f"FAIL {msg}")
            else:
                print(f"::warning::{msg}" if IN_CI else f"NEW  {msg}")
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
        print(f"\n{failed} binary format snapshot(s) differ. If the change is intended and sdkjs reads it, "
              "update with: Test/BinaryFormat/run_snapshots.py <suite> --update")
    elif new:
        print(f"\n{new} snapshot(s) missing; nothing to compare for them yet.")
    if print_new and new_texts:
        print("New snapshots follow; import them with: run_snapshots.py --import-log <job log>")
        print_bundle(new_texts)
    return 1 if failed else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("suite", nargs="?", choices=["templates", "x2t"])
    ap.add_argument("--package", help="build package folder with x2t and x2ttester (x2t suite)")
    ap.add_argument("--update", action="store_true", help="rewrite the snapshots instead of comparing")
    ap.add_argument("--missing", choices=["fail", "warn"], default="fail",
                    help="what a case without a snapshot does (default: fail)")
    ap.add_argument("--out", type=Path, help="also write every dump here (for CI artifacts)")
    ap.add_argument("--print-new", action="store_true",
                    help="print missing snapshots into the log as a base64 tar.gz block")
    ap.add_argument("--import-log", type=Path, metavar="LOG",
                    help="write the snapshots from a --print-new block in a saved CI log, then exit")
    args = ap.parse_args()
    if args.import_log:
        return import_bundle(args.import_log)
    if not args.suite:
        ap.error("a suite is required")
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)

    if args.suite == "templates":
        return run(template_cases(), args.update, args.missing, args.out, print_new=args.print_new)
    if not args.package:
        ap.error("the x2t suite needs --package")
    work = Path(tempfile.mkdtemp(prefix="binformat-"))
    try:
        return run(x2t_cases(args.package, work), args.update, args.missing, args.out, work, args.print_new)
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
