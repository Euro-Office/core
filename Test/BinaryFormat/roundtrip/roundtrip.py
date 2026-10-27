#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Round-trip documents through x2t and real sdkjs with docbuilder.

Each case is one docbuilder run of three passes:

  pass 1  OpenFile(source)  x2t writes Editor.bin, sdkjs reads it   (C++ writer, JS reader)
          facts1            structural facts via the builder API
          SaveFile(out1)    sdkjs writes Editor.bin, x2t reads it   (JS writer, C++ reader)
  pass 2  OpenFile(out1) -> facts2 -> SaveFile(out2)
  pass 3  OpenFile(out2) -> facts3

Checks, per case:
  * docbuilder succeeds and every pass writes its facts;
  * facts1 matches the committed expectation in expected/ (what the first load sees);
  * facts2 == facts1 and facts3 == facts2 (nothing is lost or invented on the way
    through both serializers);
  * out1 and out2 have the same OOXML parts and content, apart from docProps
    (the second trip is stable). Reported as a warning unless --stability fail.

Cases: the x2tTester input documents and the Common/empty templates (wrapped as
.doct/.xlst/.pptt so docbuilder opens the committed Editor.bin as is).

Runs where docbuilder, x2t and sdkjs live together, e.g. inside the
DocumentServer image (CI: build.yml, e2e leg). Python 3.9+, standard library only.

Usage:
  roundtrip.py --out DIR [--docbuilder PATH] [--case GLOB] [--update]
               [--missing fail|warn] [--stability fail|warn]
"""

import argparse
import difflib
import fnmatch
import glob
import json
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
EXPECTED = HERE / "expected"
FACTS_JS = HERE / "facts.js"
TESTER_DIR = ROOT / "Test/Applications/x2tTester"
TEMPLATES = ROOT / "Common/empty"
IN_CI = os.environ.get("GITHUB_ACTIONS") == "true"
TIMEOUT = 600

# Source extension -> (editor kind for facts.js, save format)
KINDS = {
    "docx": ("word", "docx"), "odt": ("word", "docx"), "rtf": ("word", "docx"), "doct": ("word", "docx"),
    "xlsx": ("cell", "xlsx"), "ods": ("cell", "xlsx"), "xlst": ("cell", "xlsx"),
    "pptx": ("slide", "pptx"), "odp": ("slide", "pptx"), "pptt": ("slide", "pptx"),
}
TEMPLATE_PACKAGES = {"docx.bin": "doct", "xlsx.bin": "xlst", "pptx.bin": "pptt"}
# Parts that legitimately change on every save.
VOLATILE_PARTS = {"docProps/core.xml", "docProps/app.xml", "docProps/custom.xml"}


class Case:
    def __init__(self, name, source, prepare=None):
        self.name = name  # e.g. "inputs/medium.docx", also the expectation path
        self.source = source
        self.prepare = prepare  # callable(dest_dir) -> path to open, or None to copy source
        ext = source.suffix.lstrip(".") if prepare is None else TEMPLATE_PACKAGES[source.name]
        self.kind, self.save_ext = KINDS[ext]


def all_cases():
    cases = []
    for sub in ("input_word_proc", "input_sheets", "input_slides"):
        for f in sorted((TESTER_DIR / sub).iterdir()):
            if f.is_file() and f.suffix.lstrip(".") in KINDS:
                cases.append(Case(f"inputs/{f.name}", f))
    for f in sorted(TEMPLATES.glob("*.bin")):
        if f.name in TEMPLATE_PACKAGES:
            cases.append(Case(f"templates/{f.name}", f, prepare=_wrap_template(f)))
    return cases


def _wrap_template(bin_path):
    def prepare(dest):
        # A .doct/.xlst/.pptt is a zip with Editor.bin; x2t opens it with TCD_T2BIN (a copy),
        # so sdkjs reads exactly the committed bytes.
        target = dest / f"{bin_path.stem}.{TEMPLATE_PACKAGES[bin_path.name]}"
        with zipfile.ZipFile(target, "w") as z:
            z.write(bin_path, "Editor.bin")
        return target
    return prepare


def builder_script(case, src, work):
    """The .docbuilder script for one case (three passes, see module docstring)."""
    facts_js = FACTS_JS.read_text(encoding="utf-8")
    for line in facts_js.splitlines():
        stripped = line.lstrip()
        if stripped.startswith("builder") or stripped.startswith("#"):
            raise ValueError(f"facts.js line would be taken as a builder command or comment: {line!r}")
    out = [f'builder.SetTmpFolder("{work / "tmp"}")']
    opened = src
    for n in (1, 2, 3):
        out.append(f'builder.OpenFile("{opened}", "")')
        out.append(facts_js)
        out.append(f'var sFacts = rtFacts("{case.kind}");')
        out.append(f'builder.WriteData("{work / f"facts{n}.json"}", "jsValue(sFacts)", "false")')
        if n < 3:
            saved = work / f"pass{n}.{case.save_ext}"
            out.append(f'builder.SaveFile("{case.save_ext}", "{saved}")')
            opened = saved
        out.append("builder.CloseFile()")
    return "\n".join(out) + "\n"


def find_docbuilder(explicit):
    if explicit:
        return Path(explicit)
    hits = sorted(glob.glob("/var/www/*/documentserver/server/FileConverter/bin/docbuilder"))
    if not hits:
        sys.exit("docbuilder not found; pass --docbuilder")
    return Path(hits[0])


def docbuilder_env(docbuilder):
    """Mirror the converter's library path (DocumentServer supervisor: bin + lib)."""
    env = dict(os.environ)
    bin_dir = docbuilder.parent
    paths = [str(bin_dir), str(bin_dir.parent / "lib")]
    if env.get("LD_LIBRARY_PATH"):
        paths.append(env["LD_LIBRARY_PATH"])
    env["LD_LIBRARY_PATH"] = ":".join(paths)
    return env


def docbuilder_args(docbuilder):
    args = [str(docbuilder)]
    bin_dir = docbuilder.parent
    if (bin_dir / "AllFonts.js").exists() and (bin_dir / "font_selection.bin").exists():
        args.append("--check-fonts=0")  # use the font set the image was built with
    return args


def run_case(case, out, docbuilder):
    work = (out / case.name).resolve()
    if work.exists():
        shutil.rmtree(work)
    (work / "tmp").mkdir(parents=True)
    src = case.prepare(work) if case.prepare else Path(shutil.copy2(case.source, work / case.source.name))
    script = work / "script.docbuilder"
    script.write_text(builder_script(case, src, work), encoding="utf-8")
    # x2t gets ./sdkjs/slide/themes as its theme dir, so run from the DocumentServer root.
    cwd = docbuilder.parent.parent.parent.parent
    try:
        r = subprocess.run(docbuilder_args(docbuilder) + [str(script)], cwd=str(cwd if cwd.is_dir() else work),
                           env=docbuilder_env(docbuilder), capture_output=True, text=True,
                           errors="replace", timeout=TIMEOUT)
        log, code = r.stdout + r.stderr, r.returncode
    except subprocess.TimeoutExpired as e:
        partial = [s.decode("utf-8", "replace") if isinstance(s, bytes) else (s or "") for s in (e.stdout, e.stderr)]
        log, code = f"timeout after {TIMEOUT}s\n" + "".join(partial), -1
    (work / "docbuilder.log").write_text(log, encoding="utf-8")
    facts = []
    for n in (1, 2, 3):
        f = work / f"facts{n}.json"
        try:
            facts.append(json.loads(f.read_text(encoding="utf-8")) if f.exists() else None)
        except ValueError as e:
            facts.append({"unparsable": str(e)})
    return work, code, log, facts


def flatten(value, prefix=""):
    """{"a": {"b": [1]}} -> {"a.b[0]": 1}, so differences can be named."""
    out = {}
    if isinstance(value, dict):
        for k in sorted(value):
            out.update(flatten(value[k], f"{prefix}.{k}" if prefix else k))
    elif isinstance(value, list):
        out[f"{prefix}.length"] = len(value)
        for i, v in enumerate(value):
            out.update(flatten(v, f"{prefix}[{i}]"))
    else:
        out[prefix] = value
    return out


def fact_diff(expected, actual, limit=25):
    a, b = flatten(expected), flatten(actual)
    lines = []
    for key in sorted(set(a) | set(b)):
        if a.get(key, "<missing>") != b.get(key, "<missing>"):
            lines.append(f"    {key}: expected {json.dumps(a.get(key, '<missing>'))}, "
                         f"got {json.dumps(b.get(key, '<missing>'))}")
    if len(lines) > limit:
        lines = lines[:limit] + [f"    ... {len(lines) - limit} more"]
    return lines


def package_diff(first, second, limit=10):
    """Differences between two OOXML packages, ignoring parts that change on every save."""
    try:
        with zipfile.ZipFile(first) as za, zipfile.ZipFile(second) as zb:
            na = {n for n in za.namelist() if not n.endswith("/")} - VOLATILE_PARTS
            nb = {n for n in zb.namelist() if not n.endswith("/")} - VOLATILE_PARTS
            lines = [f"    part only in {first.name}: {n}" for n in sorted(na - nb)]
            lines += [f"    part only in {second.name}: {n}" for n in sorted(nb - na)]
            for n in sorted(na & nb):
                da, db = za.read(n), zb.read(n)
                if da == db:
                    continue
                lines.append(f"    {n} differs")
                if n.endswith((".xml", ".rels")):
                    ta = da.decode("utf-8", "replace").replace("><", ">\n<").splitlines()
                    tb = db.decode("utf-8", "replace").replace("><", ">\n<").splitlines()
                    diff = [d for d in difflib.unified_diff(ta, tb, lineterm="", n=0)
                            if not d.startswith(("---", "+++", "@@"))]
                    lines += [f"      {d[:200]}" for d in diff[:6]]
    except (OSError, zipfile.BadZipFile) as e:
        return [f"    cannot compare packages: {e}"]
    if len(lines) > limit * 7:
        lines = lines[:limit * 7] + ["    ..."]
    return lines


class Report:
    def __init__(self):
        self.failed = 0
        self.warned = 0

    def fail(self, case, msg, details=()):
        self.failed += 1
        print(f"FAIL {case.name}: {msg}")
        if IN_CI:
            print(f"::error title=sdkjs round-trip::{case.name}: {msg}")
        for d in details:
            print(d)

    def warn(self, case, msg, details=()):
        self.warned += 1
        print(f"WARN {case.name}: {msg}")
        if IN_CI:
            print(f"::warning title=sdkjs round-trip::{case.name}: {msg}")
        for d in details:
            print(d)


def check_case(case, work, code, log, facts, report, args):
    names = ("pass 1 (source -> x2t -> sdkjs)", "pass 2 (sdkjs -> x2t -> OOXML -> x2t -> sdkjs)",
             "pass 3 (second trip)")
    missing = [names[i] for i, f in enumerate(facts) if f is None]
    if code != 0 or missing:
        tail = [f"    | {line}" for line in log.strip().splitlines()[-20:]]
        what = f"docbuilder exited with {code}" if code != 0 else "docbuilder exited 0"
        if missing:
            what += f"; no facts from {', '.join(missing)}"
        report.fail(case, what, tail)
        return

    # Keep what the first load saw next to the outputs, so CI artifacts can seed expected/.
    seen = Path(args.out) / "expected" / f"{case.name}.json"
    seen.parent.mkdir(parents=True, exist_ok=True)
    seen.write_text(json.dumps(facts[0], indent=1, sort_keys=True, ensure_ascii=False) + "\n", encoding="utf-8")

    ok = True
    expected_file = EXPECTED / f"{case.name}.json"
    if args.update:
        expected_file.parent.mkdir(parents=True, exist_ok=True)
        expected_file.write_text(seen.read_text(encoding="utf-8"), encoding="utf-8")
        print(f"updated expected/{case.name}.json")
    elif expected_file.exists():
        expected = json.loads(expected_file.read_text(encoding="utf-8"))
        if expected != facts[0]:
            ok = False
            report.fail(case, "first load differs from expected/" + case.name + ".json",
                        fact_diff(expected, facts[0]))
    elif args.missing == "fail":
        ok = False
        report.fail(case, f"no expected/{case.name}.json; run with --update and commit it")
    else:
        report.warn(case, f"no expected/{case.name}.json yet; first-load facts not checked")

    for n in (1, 2):
        if facts[n] != facts[n - 1]:
            ok = False
            report.fail(case, f"facts changed between pass {n} and pass {n + 1}: "
                              f"{'sdkjs -> x2t -> OOXML -> x2t -> sdkjs' if n == 1 else 'second trip'} lost or changed content",
                        fact_diff(facts[n - 1], facts[n]))

    out1, out2 = work / f"pass1.{case.save_ext}", work / f"pass2.{case.save_ext}"
    diff = package_diff(out1, out2)
    if diff:
        msg = f"second save is not stable ({out1.name} vs {out2.name})"
        if args.stability == "fail":
            ok = False
            report.fail(case, msg, diff)
        else:
            report.warn(case, msg, diff)
    if ok:
        print(f"ok   {case.name}")


def chown_tree(path):
    """Hand files back to the CI user when this runs as root in a container."""
    uid, gid = os.environ.get("HOST_UID"), os.environ.get("HOST_GID")
    if not uid or not hasattr(os, "chown"):
        return
    for p in [path] + list(Path(path).rglob("*")):
        try:
            os.chown(p, int(uid), int(gid or uid), follow_symlinks=False)
        except OSError:
            pass


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True, type=Path, help="work and result folder (upload it from CI)")
    ap.add_argument("--docbuilder", help="docbuilder binary (default: the DocumentServer install)")
    ap.add_argument("--case", default="*", help="only cases whose name matches this glob")
    ap.add_argument("--update", action="store_true", help="rewrite expected/ from this run")
    ap.add_argument("--missing", choices=["fail", "warn"], default="fail",
                    help="what a case without an expectation does (default: fail)")
    ap.add_argument("--stability", choices=["fail", "warn"], default="warn",
                    help="whether an unstable second save fails (default: warn)")
    args = ap.parse_args()

    docbuilder = find_docbuilder(args.docbuilder)
    args.out.mkdir(parents=True, exist_ok=True)
    cases = [c for c in all_cases() if fnmatch.fnmatch(c.name, args.case)]
    report = Report()
    try:
        for case in cases:
            check_case(case, *run_case(case, args.out, docbuilder), report, args)
    finally:
        chown_tree(args.out)
    print(f"\n{len(cases)} case(s), {report.failed} failed, {report.warned} warning(s). "
          f"Outputs, logs and first-load facts are in {args.out}.")
    if report.failed:
        print("A failure names the pass and the fact that changed. If a first-load change is intended, "
              "rerun with --update (or copy <out>/expected/ from the CI artifact) and commit it.")
    return 1 if report.failed else 0


if __name__ == "__main__":
    sys.exit(main())
