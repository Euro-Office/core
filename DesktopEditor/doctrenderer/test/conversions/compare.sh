#!/bin/bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
# Converts documents to PDF with the binaries of a DocumentServer image and with
# a core build, and compares the results. See README.md.
#
#   compare.sh <core-package-dir> [extra-corpus-dir]
#
# Environment: IMAGE (default ghcr.io/euro-office/documentserver:latest-dev),
# OUT (default ./conversion-results), TOLERANCE (see compare-pdfs.sh).
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
CORE_ROOT=$(cd "$HERE/../../../.." && pwd)
PKG=$(cd "$1" && pwd)
EXTRA=${2:-}
IMAGE=${IMAGE:-ghcr.io/euro-office/documentserver:latest-dev}
OUT=$(mkdir -p "${OUT:-conversion-results}" && cd "${OUT:-conversion-results}" && pwd)

[ -x "$PKG/bin/x2t" ] || { echo "no x2t in $PKG/bin, pass the core package dir (EO_CORE_OUTPUT_DIR/..)"; exit 2; }

CORPUS="$OUT/corpus"
rm -rf "$CORPUS"; mkdir -p "$CORPUS"
grep -v '^\s*\(#\|$\)' "$HERE/corpus.txt" | while read -r doc; do cp "$CORE_ROOT/$doc" "$CORPUS/"; done
[ -n "$EXTRA" ] && cp "$EXTRA"/* "$CORPUS/"
echo "corpus: $(ls "$CORPUS" | wc -l) documents"

for mode in image build; do
    echo "converting with the $mode binaries..."
    docker run --rm --entrypoint bash \
        -v "$HERE:/h:ro" -v "$CORPUS:/corpus:ro" -v "$PKG:/pkg:ro" -v "$OUT:/out" \
        "$IMAGE" /h/run-in-image.sh $mode > "$OUT/$mode.log" 2>&1
    echo "  JS engine $(cat "$OUT/$mode/v8-version.txt" 2>/dev/null)"
done

echo
echo "== failed steps (exit code != 0)"
failed=$(awk -F'\t' 'FNR > 1 && $3 != 0 { print FILENAME ": " $1 " " $2 " exit " $3 }' "$OUT"/image/results.tsv "$OUT"/build/results.tsv | sed "s|$OUT/||")
echo "${failed:-none}"

echo
echo "== conversion time (warm), seconds"
join -t $'\t' \
    <(awk -F'\t' '$1 == "pdf-warm" { print $2 "\t" $4 }' "$OUT/image/results.tsv" | sort) \
    <(awk -F'\t' '$1 == "pdf-warm" { print $2 "\t" $4 }' "$OUT/build/results.tsv" | sort) |
    awk -F'\t' '{ a += $2; b += $3; printf "%-36s %6s %6s\n", $1, $2, $3 } END { printf "%-36s %6.2f %6.2f\n", "total (image, build)", a, b }'

echo
compare() {
    docker run --rm -v "$HERE:/h:ro" -v "$OUT:/out" ubuntu:24.04 bash -c \
        "apt-get update -qq >/dev/null && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends poppler-utils imagemagick >/dev/null 2>&1 && TOLERANCE=${TOLERANCE:-0.001} /h/compare-pdfs.sh $1 $2"
}
status=0
echo "== PDFs: image vs build"
compare /out/image/pdf /out/build/pdf || status=1
if [ -d "$OUT/build/pdf-old-snapshots" ]; then
    echo
    echo "== PDFs: build vs build with the image's startup snapshots left in sdkjs/"
    compare /out/build/pdf /out/build/pdf-old-snapshots || status=1
fi
[ -z "$failed" ] || status=1

echo
[ $status = 0 ] && echo "RESULT: no differences" || echo "RESULT: differences found, see above and $OUT"
exit $status
