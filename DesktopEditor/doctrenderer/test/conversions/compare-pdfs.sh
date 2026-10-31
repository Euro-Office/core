#!/bin/bash
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
# Compares the PDFs of two conversion runs: page count, text and rendered pixels.
# Usage (in a container with poppler-utils and imagemagick): compare-pdfs.sh <dir-a> <dir-b>
# Prints one line per document and exits 1 if any document differs beyond the tolerance.
A="$1"; B="$2"
# share of differing pixels per page that still counts as equal (antialiasing etc.)
TOLERANCE=${TOLERANCE:-0.001}
WORK=$(mktemp -d)
status=0
printf '%-36s %6s %6s %-10s %s\n' document pages-a pages-b text worst-page-pixel-diff
for pdfA in "$A"/*.pdf; do
    name=$(basename "$pdfA"); pdfB="$B/$name"
    if [ ! -f "$pdfB" ]; then printf '%-36s missing in %s\n' "$name" "$B"; status=1; continue; fi
    pagesA=$(pdfinfo "$pdfA" 2>/dev/null | awk '/^Pages:/{print $2}')
    pagesB=$(pdfinfo "$pdfB" 2>/dev/null | awk '/^Pages:/{print $2}')
    pdftotext -layout "$pdfA" "$WORK/a.txt" 2>/dev/null; pdftotext -layout "$pdfB" "$WORK/b.txt" 2>/dev/null
    if cmp -s "$WORK/a.txt" "$WORK/b.txt"; then text=same; else text=DIFFERENT; fi
    rm -f "$WORK"/a-*.png "$WORK"/b-*.png
    pdftoppm -r 50 -png "$pdfA" "$WORK/a" 2>/dev/null; pdftoppm -r 50 -png "$pdfB" "$WORK/b" 2>/dev/null
    worst=0
    for pngA in "$WORK"/a-*.png; do
        pngB="$WORK/b-${pngA##*/a-}"
        [ -f "$pngB" ] || continue
        diff=$(compare -metric AE -fuzz 2% "$pngA" "$pngB" null: 2>&1 | awk '{print $1}')
        total=$(identify -format '%[fx:w*h]' "$pngA")
        ratio=$(awk -v d="$diff" -v t="$total" 'BEGIN{printf "%.5f", d/t}')
        worst=$(awk -v a="$worst" -v b="$ratio" 'BEGIN{print (b > a) ? b : a}')
    done
    printf '%-36s %6s %6s %-10s %s\n' "$name" "$pagesA" "$pagesB" "$text" "$worst"
    if [ "$pagesA" != "$pagesB" ] || [ "$text" != same ] || awk -v w="$worst" -v t="$TOLERANCE" 'BEGIN{exit !(w > t)}'; then
        status=1
    fi
done
rm -rf "$WORK"
exit $status
