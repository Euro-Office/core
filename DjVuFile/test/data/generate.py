#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

"""Generates the DjVu fixtures of djvu_test.

Needs DjVuLibre's command line tools (cjb2, c44, djvused, djvm) on PATH, e.g.
from the djvulibre-bin package. The pages are simple shapes whose pixels the
test knows exactly:

- bilevel.djvu: 240x120 at 300 dpi, JB2 (lossless). A black rectangle at
  x 20..119, y 10..99 (from the top) with a white square at x 50..89,
  y 30..69 inside. A text layer with the words "Hello" and "DjVu" and a link
  to https://example.com/ over the rectangle.
- color.djvu: 128x96 at 100 dpi, IW44 (lossy). Quadrants red, green (top),
  blue, white (bottom).
- document.djvu: both pages in one bundled document, with bookmarks
  "Black page" (containing "Inner") on page 1 and "Color page" on page 2.
"""

import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent


def run(*args):
    subprocess.run([str(a) for a in args], check=True)


def write_pbm(path, w, h, black):
    rows = []
    for y in range(h):
        bits = [1 if black(x, y) else 0 for x in range(w)]
        row = bytearray()
        for i in range(0, w, 8):
            byte = 0
            for j in range(8):
                byte = (byte << 1) | (bits[i + j] if i + j < w else 0)
            row.append(byte)
        rows.append(bytes(row))
    path.write_bytes(b"P4\n%d %d\n" % (w, h) + b"".join(rows))


def write_ppm(path, w, h, color):
    data = bytearray()
    for y in range(h):
        for x in range(w):
            data += bytes(color(x, y))
    path.write_bytes(b"P6\n%d %d\n255\n" % (w, h) + bytes(data))


def bilevel_black(x, y):
    in_rect = 20 <= x < 120 and 10 <= y < 100
    in_hole = 50 <= x < 90 and 30 <= y < 70
    return in_rect and not in_hole


def quadrant_color(x, y):
    left, top = x < 64, y < 48
    if top:
        return (255, 0, 0) if left else (0, 255, 0)
    return (0, 0, 255) if left else (255, 255, 255)


# djvused coordinates start at the bottom left of the page
BILEVEL_SED = """
select 1
set-txt
(page 0 0 240 120
  (line 20 60 200 100
    (word 20 60 100 100 "Hello")
    (word 110 60 200 100 "DjVu")))
.
set-ant
(maparea "https://example.com/" "" (rect 20 20 100 90))
.
"""

OUTLINE_SED = """
set-outline
(bookmarks
  ("Black page" "#1"
    ("Inner" "#1"))
  ("Color page" "#2"))
.
"""


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)

        pbm = tmp / "bilevel.pbm"
        write_pbm(pbm, 240, 120, bilevel_black)
        bilevel = HERE / "bilevel.djvu"
        run("cjb2", "-dpi", "300", "-lossless", pbm, bilevel)
        run("djvused", bilevel, "-s", "-e", BILEVEL_SED)

        ppm = tmp / "color.ppm"
        write_ppm(ppm, 128, 96, quadrant_color)
        color = HERE / "color.djvu"
        run("c44", "-dpi", "100", ppm, color)

        document = HERE / "document.djvu"
        run("djvm", "-c", document, bilevel, color)
        run("djvused", document, "-s", "-e", OUTLINE_SED)


if __name__ == "__main__":
    main()
