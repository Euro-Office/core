#!/bin/bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
#
# Open every document in DIR with LibreOffice and convert it to PDF. A reader
# that is not core rejects files whose container or XML core wrote wrongly, so
# a document that does not convert is a writer bug.
#
#   office_smoke.sh DIR        SOFFICE overrides the soffice binary

dir="$1"
soffice="${SOFFICE:-soffice}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
failed=0
count=0

for file in "$dir"/*; do
	[ -f "$file" ] || continue
	count=$((count + 1))
	out="$work/out$count"
	mkdir "$out"
	timeout 180 "$soffice" -env:UserInstallation="file://$work/profile" --headless --convert-to pdf --outdir "$out" "$file" > "$out/log" 2>&1
	if [ -z "$(find "$out" -name '*.pdf' -size +0 2>/dev/null)" ]; then
		echo "FAIL: LibreOffice cannot open $(basename "$file")" >&2
		sed 's/^/    /' "$out/log" >&2
		failed=1
	fi
done

if [ "$count" -eq 0 ]; then
	echo "FAIL: no documents in $dir" >&2
	exit 1
fi
echo "Converted $count documents, $([ "$failed" = 0 ] && echo 'all opened' || echo 'some failed')"
exit "$failed"
