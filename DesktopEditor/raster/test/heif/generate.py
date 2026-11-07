#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

"""Generates the HEIF fixtures for raster_formats_test.

Core only decodes HEIF, so these are encoded here instead of in the test.
Needs Pillow and ImageMagick 7 with libheif (x265 and aom encoders); made
with Pillow 10.2.0, ImageMagick 7.1.1-43 and libheif 1.17.6:

    python3 generate.py

Writes next to this script:
    quad.heic        HEVC, the four quadrant colors of the test, 4:4:4
    quad_alpha.avif  AV1 with an alpha plane: 255, 128, 255, 0 per quadrant
    quad_10bit.avif  AV1 at 10 bits per channel
    wide_rot.heic    a 32x16 image (QUADRANTS[0] left, QUADRANTS[1] right),
                     stored with a 90 degree clockwise rotation (irot)
"""

import subprocess
import tempfile
from pathlib import Path

from PIL import Image

OUT_DIR = Path(__file__).resolve().parent

# c_arQuadrants in test.cpp: top left, top right, bottom left, bottom right
QUADRANTS = [(220, 30, 40), (30, 200, 60), (20, 40, 210), (128, 128, 128)]
ALPHA = [255, 128, 255, 0]
SIZE = 32


def quadrant(x: int, y: int) -> int:
    return (2 if y >= SIZE // 2 else 0) + (1 if x >= SIZE // 2 else 0)


def quad_image(with_alpha: bool) -> Image.Image:
    image = Image.new("RGBA", (SIZE, SIZE))
    for y in range(SIZE):
        for x in range(SIZE):
            i = quadrant(x, y)
            image.putpixel((x, y), QUADRANTS[i] + (ALPHA[i] if with_alpha else 255,))
    return image if with_alpha else image.convert("RGB")


def wide_image() -> Image.Image:
    image = Image.new("RGB", (SIZE, SIZE // 2))
    for y in range(SIZE // 2):
        for x in range(SIZE):
            image.putpixel((x, y), QUADRANTS[0] if x < SIZE // 2 else QUADRANTS[1])
    return image


def magick(source: Image.Image, target: str, *options: str) -> None:
    with tempfile.TemporaryDirectory() as tmp:
        png = Path(tmp) / "source.png"
        source.save(png)
        subprocess.run(
            ["magick", str(png), "-quality", "100", "-define", "heic:chroma=444", *options, str(OUT_DIR / target)],
            check=True,
        )


magick(quad_image(False), "quad.heic")
magick(quad_image(True), "quad_alpha.avif")
magick(quad_image(False), "quad_10bit.avif", "-depth", "10")
# RightTop: the stored image is shown rotated by 90 degrees clockwise
magick(wide_image(), "wide_rot.heic", "-orient", "RightTop")
