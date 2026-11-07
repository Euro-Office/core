#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

"""Generates the fixture fonts for fontengine_test.

Tiny fonts with simple geometric glyphs and exactly known metrics, so the
tests can compare against fixed values. Needs fontTools and brotli:

    pip install fonttools==4.60.1 brotli==1.1.0
    python3 generate.py

Writes EOTestTrueType.ttf, EOTestTrueType.woff2 and EOTestCFF.otf next to
this script.
"""

from pathlib import Path

from fontTools.fontBuilder import FontBuilder
from fontTools.pens.t2CharStringPen import T2CharStringPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont

OUT_DIR = Path(__file__).resolve().parent

UNITS_PER_EM = 1000
ASCENDER = 800      # hhea / OS/2 typo
DESCENDER = -200
WIN_ASCENT = 900    # OS/2 win metrics differ on purpose
WIN_DESCENT = 250
TIMESTAMP = 0x7C259DC0  # fixed head.created/modified for reproducible output

GLYPH_ORDER = [".notdef", "space", "A", "B", "uni4E00"]
CMAP = {0x20: "space", 0x41: "A", 0x42: "B", 0x4E00: "uni4E00"}
ADVANCES = {".notdef": 500, "space": 250, "A": 600, "B": 700, "uni4E00": 1000}

# Closed polygons; together they span y = -100 .. 750 (head yMin / yMax)
CONTOURS = {
    ".notdef": [[(50, 0), (450, 0), (450, 700), (50, 700)]],
    "space": [],
    "A": [[(100, 0), (500, 0), (500, 700), (100, 700)]],
    "B": [[(50, -100), (650, -100), (350, 750)]],
    "uni4E00": [[(50, 300), (950, 300), (950, 400), (50, 400)]],
}


def draw(pen, contours):
    for contour in contours:
        pen.moveTo(contour[0])
        for point in contour[1:]:
            pen.lineTo(point)
        pen.closePath()


def bounds(contours):
    points = [p for contour in contours for p in contour]
    if not points:
        return 0
    return min(p[0] for p in points)


def build(family, german_family, cff):
    fb = FontBuilder(UNITS_PER_EM, isTTF=not cff)
    fb.setupGlyphOrder(GLYPH_ORDER)
    fb.setupCharacterMap(CMAP)

    if cff:
        charstrings = {}
        for name in GLYPH_ORDER:
            pen = T2CharStringPen(ADVANCES[name], None)
            draw(pen, CONTOURS[name])
            charstrings[name] = pen.getCharString()
        fb.setupCFF(family.replace(" ", "") + "-Regular", {"FullName": family}, charstrings, {})
    else:
        glyphs = {}
        for name in GLYPH_ORDER:
            pen = TTGlyphPen(None)
            draw(pen, CONTOURS[name])
            glyphs[name] = pen.glyph()
        fb.setupGlyf(glyphs)

    fb.setupHorizontalMetrics({name: (ADVANCES[name], bounds(CONTOURS[name])) for name in GLYPH_ORDER})
    fb.setupHorizontalHeader(ascent=ASCENDER, descent=DESCENDER)
    fb.setupNameTable({
        "familyName": {"en": family, "de": german_family},
        "styleName": "Regular",
        "uniqueFontIdentifier": family + " Regular",
        "fullName": family,
        "psName": family.replace(" ", "") + "-Regular",
        "version": "Version 1.000",
    }, mac=False)
    fb.setupOS2(sTypoAscender=ASCENDER, sTypoDescender=DESCENDER, sTypoLineGap=0,
                usWinAscent=WIN_ASCENT, usWinDescent=WIN_DESCENT,
                achVendID="EOFF", fsType=0)
    fb.setupPost()
    fb.font["head"].created = TIMESTAMP
    fb.font["head"].modified = TIMESTAMP
    return fb.font


def main():
    truetype = build("EO Test TrueType", "EO Testschrift TT", cff=False)
    truetype.save(OUT_DIR / "EOTestTrueType.ttf")

    woff2 = TTFont(OUT_DIR / "EOTestTrueType.ttf")
    woff2.flavor = "woff2"
    woff2.save(OUT_DIR / "EOTestTrueType.woff2")

    build("EO Test CFF", "EO Testschrift CFF", cff=True).save(OUT_DIR / "EOTestCFF.otf")


if __name__ == "__main__":
    main()
