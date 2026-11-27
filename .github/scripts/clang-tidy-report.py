#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

"""Turns clang-tidy output into GitHub annotations, a job summary and a PR comment.

Usage: clang-tidy-report.py OUT_DIR

Reads OUT_DIR/clang-tidy.log (paths are reported relative to the current
directory) and writes annotations.txt, summary.md, comment.md and count, see
clang-tidy-diff.sh. Exits 1 when clang-tidy didn't run properly (OUT_DIR/failure,
or errors that point at no file), so that isn't reported as "no findings".
"""

import os
import re
import sys

MARKER = "<!-- clang-tidy-report -->"
MAX_COMMENT_ROWS = 50
# The job summary takes up to 1 MiB
MAX_SUMMARY_ROWS = 2000
DIAG = re.compile(
    r"^(?P<file>[^\s:][^:]*):(?P<line>\d+):(?P<col>\d+): "
    r"(?P<sev>warning|error): (?P<msg>.*?)(?: \[(?P<check>[^\]]+)\])?$"
)
# Lines that mean clang-tidy itself failed, not the code it checked: errors
# with no location (e.g. "error: unknown argument"), crashes, a missing tool
FAILURE = re.compile(r"^(error: |Error: |Terminated by signal |Traceback |.*command not found)")


def read(out, name):
    try:
        with open(os.path.join(out, name)) as f:
            return f.read().strip()
    except FileNotFoundError:
        return ""


def escape_data(text):
    return text.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def escape_property(text):
    return escape_data(text).replace(":", "%3A").replace(",", "%2C")


def code(text):
    # A code span keeps <...>, * and @ literal; | still ends a table cell
    return "`" + text.replace("`", "'").replace("|", "\\|") + "`"


def table(rows):
    lines = ["| Location | Check | Message |", "|---|---|---|"]
    for (path, line, _col, check), msg in rows:
        lines.append(f"| {code(f'{path}:{line}')} | {code(check)} | {code(msg)} |")
    return lines


def capped(rows, limit, run_url):
    lines = table(rows[:limit])
    if len(rows) > limit:
        more = f"...and {len(rows) - limit} more"
        lines += ["", f"{more} in the [job summary]({run_url})." if run_url else f"{more}."]
    return lines


def main():
    out = sys.argv[1]
    root = os.getcwd() + os.sep
    run_url = read(out, "run-url")
    head_sha = read(out, "head-sha")
    findings = {}
    unparsed = set()
    failures = [read(out, "failure")] if read(out, "failure") else []

    with open(os.path.join(out, "clang-tidy.log"), errors="replace") as log:
        for raw in log:
            text = raw.rstrip("\n")
            if FAILURE.match(text):
                failures.append(text)
                continue
            m = DIAG.match(text)
            if not m:
                continue
            path = os.path.normpath(m["file"])
            if path.startswith(root):
                path = path[len(root):]
            if m["sev"] == "error":
                # Usually a header that doesn't compile on its own: not a finding
                unparsed.add(path)
                continue
            key = (path, int(m["line"]), int(m["col"]), m["check"] or "clang-tidy")
            findings.setdefault(key, m["msg"])

    rows = sorted(findings.items())
    count = len(rows)

    with open(os.path.join(out, "annotations.txt"), "w") as f:
        for (path, line, col, check), msg in rows:
            f.write(
                f"::warning file={escape_property(path)},line={line},col={col},"
                f"title={escape_property('clang-tidy: ' + check)}::{escape_data(msg)}\n"
            )

    title = "### clang-tidy (informational)"
    commit = f" (as of {head_sha[:10]})" if head_sha else ""
    if count:
        intro = (f"{count} finding{'s' if count != 1 else ''} on the lines changed by "
                 f"this pull request{commit}. They don't block the merge.")
    elif failures:
        intro = f"No findings, but clang-tidy didn't run properly{commit}, see below."
    else:
        intro = f"No findings on the lines changed by this pull request{commit}."
    notes = []
    if failures:
        log_link = f" in the [job log]({run_url})" if run_url else " in the job log"
        notes += ["", f"**clang-tidy didn't run properly**, so findings may be missing. "
                      f"First error{log_link}: {code(failures[0])}"]
    if unparsed:
        notes += ["", f"{len(unparsed)} file(s) had compile errors under clang-tidy (often a "
                      "header that doesn't build on its own), so findings may be incomplete."]
    footer = ["", "Check list: root `.clang-tidy`, clang-tidy 22.1.8. A false positive can be "
                  "silenced with `// NOLINT(check-name)` and the reason."]

    head = [title, "", intro]
    with open(os.path.join(out, "summary.md"), "w") as f:
        body = [""] + capped(rows, MAX_SUMMARY_ROWS, "") if rows else []
        f.write("\n".join(head + body + notes + footer) + "\n")

    with open(os.path.join(out, "comment.md"), "w") as f:
        body = [""] + capped(rows, MAX_COMMENT_ROWS, run_url) if rows else []
        f.write("\n".join([MARKER] + head + body + notes + footer) + "\n")

    with open(os.path.join(out, "count"), "w") as f:
        f.write(f"{count}\n")

    print(f"clang-tidy: {count} finding(s) on changed lines")
    if failures:
        print("clang-tidy didn't run properly:", *failures[:20], sep="\n  ")
        sys.exit(1)


if __name__ == "__main__":
    main()
