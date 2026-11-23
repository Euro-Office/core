/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef PNM_FILE_H
#define PNM_FILE_H

#include "../BgraFrame.h"

// Netpbm images: PBM, PGM and PPM, ASCII (P1-P3) and binary (P4-P6)
namespace NSPnm
{
	bool IsPnm(const BYTE* pBuffer, size_t nSize);

	bool Open(CBgraFrame* pFrame, const BYTE* pBuffer, size_t nSize, bool bIsRGBA, bool& bIsGrayScale);
	bool Open(CBgraFrame* pFrame, const std::wstring& sFileName, bool bIsRGBA, bool& bIsGrayScale);
}

#endif // PNM_FILE_H
