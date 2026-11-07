/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef WEBP_FILE_H
#define WEBP_FILE_H

#include "../BgraFrame.h"

// WebP images, lossy and lossless, with alpha. Animations decode to their first frame.
namespace NSWebP
{
	bool Open(CBgraFrame* pFrame, const BYTE* pBuffer, size_t nSize, bool bIsRGBA);
	bool Open(CBgraFrame* pFrame, const std::wstring& sFileName, bool bIsRGBA);
}

#endif // WEBP_FILE_H
