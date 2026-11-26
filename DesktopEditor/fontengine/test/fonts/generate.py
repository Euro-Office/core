#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

"""Generates the fixture fonts for fontengine_test.

Tiny fonts with simple geometric glyphs and exactly known metrics, so the
tests can compare against fixed values. Needs fontTools and brotli:

    pip install fonttools==4.60.1 brotli==1.1.0
    python3 generate.py

Writes EOTestTrueType.ttf, EOTestTrueType.woff2, EOTestCFF.otf and
EOTestColrV1.ttf next to this script.
"""

from pathlib import Path

from fontTools.colorLib.builder import buildCOLR, buildCPAL
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
# U+2764 and U+1F600 are also in the COLRv1 font, for the emoji fallback test
CMAP = {0x20: "space", 0x41: "A", 0x42: "B", 0x4E00: "uni4E00", 0x2764: "B", 0x1F600: "A"}
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


# COLRv1: full-em squares, palette 0 = red, 1 = blue, 2 = green
# Hostile paint graphs the renderer must refuse quickly: a cycle, 2^40 paths
# through shared children, and nested composites that each hold two layers
FANOUT_DEPTH = 40
COMPOSITE_DEPTH = 20
FANOUT = ["fan%d" % i for i in range(FANOUT_DEPTH)]
COMPOSITES = ["comp%d" % i for i in range(COMPOSITE_DEPTH)]
COLR_GLYPHS = [".notdef", "square", "inner", "colrSolid", "colrLinear", "colrForeground",
               "colrCycle"] + FANOUT + COMPOSITES
COLR_CMAP = {0xE000: "colrSolid", 0xE001: "colrLinear", 0xE002: "colrForeground",
             0xE003: "colrCycle", 0xE004: FANOUT[0], 0xE005: COMPOSITES[0],
             0x2764: "colrSolid", 0x1F600: "colrSolid"}
COLR_CONTOURS = {name: [] for name in COLR_GLYPHS}
COLR_CONTOURS["square"] = [[(0, 0), (1000, 0), (1000, 1000), (0, 1000)]]
COLR_CONTOURS["inner"] = [[(250, 250), (750, 250), (750, 750), (250, 750)]]


def solid(palette_index):
    return {"Format": 2, "PaletteIndex": palette_index, "Alpha": 1.0}


def paint_glyph(glyph, paint):
    return {"Format": 10, "Glyph": glyph, "Paint": paint}


def colr_glyph(glyph):
    return {"Format": 11, "Glyph": glyph}


def hostile_paints():
    paints = {"colrCycle": colr_glyph("colrCycle")}
    for i, name in enumerate(FANOUT):
        if i + 1 < len(FANOUT):
            child = colr_glyph(FANOUT[i + 1])
            paints[name] = {"Format": 1, "Layers": [child, child]}
        else:
            paints[name] = paint_glyph("square", solid(0))
    for i, name in enumerate(COMPOSITES):
        if i + 1 < len(COMPOSITES):
            paints[name] = {"Format": 32, "CompositeMode": 3,  # src_over
                            "SourcePaint": colr_glyph(COMPOSITES[i + 1]),
                            "BackdropPaint": paint_glyph("square", solid(1))}
        else:
            paints[name] = paint_glyph("square", solid(0))
    return paints


def build_colr():
    fb = FontBuilder(UNITS_PER_EM, isTTF=True)
    fb.setupGlyphOrder(COLR_GLYPHS)
    fb.setupCharacterMap(COLR_CMAP)
    glyphs = {}
    for name in COLR_GLYPHS:
        pen = TTGlyphPen(None)
        draw(pen, COLR_CONTOURS[name])
        glyphs[name] = pen.glyph()
    fb.setupGlyf(glyphs)
    fb.setupHorizontalMetrics({name: (1000, bounds(COLR_CONTOURS[name])) for name in COLR_GLYPHS})
    fb.setupHorizontalHeader(ascent=1000, descent=0)
    # the description pads the file: like real emoji fonts it must be larger than the
    # text fixtures, so allfontsgen's size ranking alone would prefer the text font
    fb.setupNameTable({"familyName": "EO Test COLRv1", "styleName": "Regular",
                       "description": "COLRv1 fixture " * 16}, mac=False)
    fb.setupOS2(sTypoAscender=1000, sTypoDescender=0, usWinAscent=1000, usWinDescent=0, fsType=0)
    fb.setupPost()
    fb.font["head"].created = TIMESTAMP
    fb.font["head"].modified = TIMESTAMP
    fb.font["CPAL"] = buildCPAL([[(1.0, 0.0, 0.0, 1.0), (0.0, 0.0, 1.0, 1.0), (0.0, 1.0, 0.0, 1.0)]])
    fb.font["COLR"] = buildCOLR({
        "colrSolid": paint_glyph("square", solid(0)),
        # red at x = 0 to blue at x = 1000
        "colrLinear": paint_glyph("square", {
            "Format": 4,
            "ColorLine": {"ColorStop": [{"StopOffset": 0.0, "PaletteIndex": 0, "Alpha": 1.0},
                                        {"StopOffset": 1.0, "PaletteIndex": 1, "Alpha": 1.0}]},
            "x0": 0, "y0": 0, "x1": 1000, "y1": 0, "x2": 0, "y2": 1000,
        }),
        # green square, inner square in the text color (palette index 0xFFFF)
        "colrForeground": {"Format": 1, "Layers": [
            paint_glyph("square", solid(2)),
            paint_glyph("inner", solid(0xFFFF)),
        ]},
        **hostile_paints(),
    }, version=1)
    return fb.font


def main():
    truetype = build("EO Test TrueType", "EO Testschrift TT", cff=False)
    truetype.save(OUT_DIR / "EOTestTrueType.ttf")

    woff2 = TTFont(OUT_DIR / "EOTestTrueType.ttf")
    woff2.flavor = "woff2"
    woff2.save(OUT_DIR / "EOTestTrueType.woff2")

    build("EO Test CFF", "EO Testschrift CFF", cff=True).save(OUT_DIR / "EOTestCFF.otf")

    build_colr().save(OUT_DIR / "EOTestColrV1.ttf")


if __name__ == "__main__":
    main()
