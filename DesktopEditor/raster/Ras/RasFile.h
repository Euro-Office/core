/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef RAS_FILE_H
#define RAS_FILE_H

#include "../BgraFrame.h"

// Sun Raster images: 1, 8, 24 and 32 bits, uncompressed or RLE, with or
// without color map
namespace NSRas
{
	bool Open(CBgraFrame* pFrame, const BYTE* pBuffer, size_t nSize, bool bIsRGBA, bool& bIsGrayScale);
	bool Open(CBgraFrame* pFrame, const std::wstring& sFileName, bool bIsRGBA, bool& bIsGrayScale);
}

#endif // RAS_FILE_H
