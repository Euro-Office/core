/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#ifndef _BUILD_COLRV1_RENDERER_H_
#define _BUILD_COLRV1_RENDERER_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>

// Rasterizes OpenType COLRv1 color glyphs (e.g. Noto Color Emoji) at the
// face's current size and transform. FreeType only parses the paint graph,
// drawing it is up to the client.
struct CColrV1Bitmap
{
	int Left   = 0; // pixels from the pen position to the left edge
	int Top    = 0; // pixels from the baseline up to the top edge
	int Width  = 0;
	int Height = 0;
	std::vector<unsigned char> Rgba; // straight (non-premultiplied) RGBA, top row first
};

bool ColrV1_HasGlyph(FT_Face face, FT_UInt gid);

// foreground: 0xRRGGBBAA, used for palette index 0xFFFF (text color)
bool ColrV1_RenderGlyph(FT_Face face, FT_UInt gid, unsigned int foreground, CColrV1Bitmap& out);

// All glyph layers of a color glyph as one outline, scaled and transformed like
// FT_Load_Glyph output (26.6): a monochrome stand-in where a bitmap cannot be
// used, e.g. warped WordArt text. Free with FT_Outline_Done.
bool ColrV1_GetSilhouette(FT_Face face, FT_UInt gid, FT_Outline* out);

#endif // _BUILD_COLRV1_RENDERER_H_
