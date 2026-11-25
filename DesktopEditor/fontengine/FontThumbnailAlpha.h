/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef _BUILD_FONT_THUMBNAIL_ALPHA_H_
#define _BUILD_FONT_THUMBNAIL_ALPHA_H_

#include "../common/Types.h"

namespace NSFontThumbnail
{
	// Most bytes EncodeAlpha() writes for nPixels pixels. A transparent pixel
	// between opaque ones takes two bytes (0 and a run length of 1).
	inline LONG GetEncodedAlphaMaxSize(LONG nPixels)
	{
		return 2 * nPixels;
	}

	// Writes the alpha channel of nPixels BGRA pixels to pDst, as read by
	// the editors' font list (fonts_thumbnail*.png.bin): an opaque pixel is
	// its alpha byte, a run of up to 255 transparent pixels is 0 followed by
	// the run length. Reads only the nPixels pixels of pBgra.
	// Returns the end of the written data.
	inline BYTE* EncodeAlpha(const BYTE* pBgra, LONG nPixels, BYTE* pDst)
	{
		const BYTE* pAlpha = pBgra + 3;
		LONG nIndexCur = 0;
		while (nIndexCur < nPixels)
		{
			*pDst++ = *pAlpha;

			if (0 != *pAlpha)
			{
				++nIndexCur;
				pAlpha += 4;
			}
			else
			{
				LONG nIndexCur0 = nIndexCur;
				LONG nIndexCurLimit = nIndexCur0 + 255;
				if (nIndexCurLimit > nPixels)
					nIndexCurLimit = nPixels;

				while (nIndexCur < nIndexCurLimit)
				{
					++nIndexCur;
					pAlpha += 4;

					// Don't read the pixel past the end of the run or the image.
					if (nIndexCur >= nIndexCurLimit || 0 != *pAlpha)
						break;
				}

				*pDst++ = (BYTE)(nIndexCur - nIndexCur0);
			}
		}
		return pDst;
	}
}

#endif // _BUILD_FONT_THUMBNAIL_ALPHA_H_
