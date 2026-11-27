#!/bin/bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Usage: .github/scripts/clang-tidy-diff.sh REV BUILD_DIR OUT_DIR
#
# Runs clang-tidy (root .clang-tidy) only on the C/C++ lines that
# "git diff REV" changes, so findings in code the change doesn't touch never
# show up. In CI REV is HEAD^1..HEAD: HEAD is the pull request's merge commit
# and HEAD^1 the base branch. Locally a branch name like main also takes the
# uncommitted changes. BUILD_DIR must hold compile_commands.json.
#
# Optional inputs in OUT_DIR: run-url (linked from the comment) and head-sha
# (the commit the comment names). Writes to OUT_DIR:
#   clang-tidy.log   raw clang-tidy output
#   annotations.txt  workflow commands, one ::warning per finding
#   summary.md       the findings, for the job summary
#   comment.md       the pull request comment (capped, links to run-url)
#   count            number of findings
# Exits non-zero when clang-tidy couldn't run properly, after writing the
# report, which then says so instead of "no findings".

set -euo pipefail

rev="$1"
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$3"
out=$(cd "$3" && pwd)
build=$(cd "$2" 2> /dev/null && pwd || echo "$2")
# git diff paths and the report's paths are relative to the repository root
cd "$(git rev-parse --show-toplevel)"
rm -f "$out/failure"

git diff -U0 --no-color "$rev" -- \
    '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' > "$out/changes.diff"

: > "$out/clang-tidy.log"
if [ ! -s "$out/changes.diff" ]; then
    :
elif ! command -v clang-tidy-diff.py > /dev/null; then
    echo "clang-tidy-diff.py is not installed" > "$out/failure"
elif [ ! -f "$build/compile_commands.json" ]; then
    echo "$build/compile_commands.json is missing" > "$out/failure"
else
    # Exits 1 for any file clang-tidy fails on, compile errors included: the
    # report tells those apart from real failures
    clang-tidy-diff.py -p1 -path "$build" -quiet -j "$(nproc)" \
        < "$out/changes.diff" > "$out/clang-tidy.log" 2>&1 || true
fi

"${CORE_PYTHON:-python3}" "$here/clang-tidy-report.py" "$out"
