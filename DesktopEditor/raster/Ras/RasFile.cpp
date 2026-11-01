/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "RasFile.h"
#include "../../common/File.h"

#include <vector>

namespace NSRas
{
	// Same limit as CxImage (CXIMAGE_MAX_MEMORY) for the decoded BGRA buffer
	static const unsigned long long c_nMaxFrameBytes = 1073741824;

	static const unsigned int c_nMagic = 0x59a66a95;
	static const size_t c_nHeaderSize = 32;

	enum ERasType
	{
		rasTypeOld = 0,
		rasTypeStandard = 1,
		rasTypeByteEncoded = 2, // RLE
		rasTypeRGB = 3          // like standard, but RGB instead of BGR
	};

	enum ERasMapType
	{
		rasMapNone = 0,
		rasMapEqualRGB = 1,
		rasMapRaw = 2
	};

	static unsigned int ReadUInt32BE(const BYTE* p)
	{
		return ((unsigned int)p[0] << 24) | ((unsigned int)p[1] << 16) | ((unsigned int)p[2] << 8) | p[3];
	}

	// 0x80 0x00 is a single 0x80, 0x80 n v is n + 1 times v
	static bool DecodeRle(const BYTE* pSrc, size_t nSrcSize, std::vector<BYTE>& arDst)
	{
		size_t nSrc = 0, nDst = 0;
		while (nDst < arDst.size())
		{
			if (nSrc >= nSrcSize)
				return false;

			BYTE c = pSrc[nSrc++];
			if (0x80 != c)
			{
				arDst[nDst++] = c;
				continue;
			}

			if (nSrc >= nSrcSize)
				return false;
			BYTE nCount = pSrc[nSrc++];
			if (0 == nCount)
			{
				arDst[nDst++] = 0x80;
				continue;
			}

			if (nSrc >= nSrcSize)
				return false;
			BYTE nValue = pSrc[nSrc++];
			for (int i = 0; i <= nCount && nDst < arDst.size(); ++i)
				arDst[nDst++] = nValue;
		}
		return true;
	}

	bool Open(CBgraFrame* pFrame, const BYTE* pBuffer, size_t nSize, bool bIsRGBA, bool& bIsGrayScale)
	{
		if (!pFrame || !pBuffer || nSize < c_nHeaderSize || c_nMagic != ReadUInt32BE(pBuffer))
			return false;

		const unsigned int nWidth     = ReadUInt32BE(pBuffer + 4);
		const unsigned int nHeight    = ReadUInt32BE(pBuffer + 8);
		const unsigned int nDepth     = ReadUInt32BE(pBuffer + 12);
		const unsigned int nType      = ReadUInt32BE(pBuffer + 20);
		const unsigned int nMapType   = ReadUInt32BE(pBuffer + 24);
		const unsigned int nMapLength = ReadUInt32BE(pBuffer + 28);

		if (0 == nWidth || 0 == nHeight || 4ULL * nWidth * nHeight > c_nMaxFrameBytes)
			return false;
		if (1 != nDepth && 8 != nDepth && 24 != nDepth && 32 != nDepth)
			return false;
		if (nType > rasTypeRGB || nMapType > rasMapRaw)
			return false;
		if (nMapLength > nSize - c_nHeaderSize)
			return false;

		// Color map: all red values, then all green, then all blue
		std::vector<BYTE> arMap;
		size_t nMapColors = 0;
		if (rasMapEqualRGB == nMapType && nDepth <= 8)
		{
			nMapColors = nMapLength / 3;
			if (nMapColors > (size_t)1 << nDepth)
				nMapColors = (size_t)1 << nDepth;
			arMap.assign(pBuffer + c_nHeaderSize, pBuffer + c_nHeaderSize + 3 * nMapColors);
		}

		// Rows are padded to 16 bits
		const size_t nRowBytes = (((size_t)nWidth * nDepth + 15) / 16) * 2;
		const size_t nRasterSize = nRowBytes * nHeight;

		const BYTE* pData = pBuffer + c_nHeaderSize + nMapLength;
		const size_t nDataSize = nSize - c_nHeaderSize - nMapLength;

		std::vector<BYTE> arDecoded;
		const BYTE* pRaster = pData;
		if (rasTypeByteEncoded == nType)
		{
			arDecoded.resize(nRasterSize);
			if (!DecodeRle(pData, nDataSize, arDecoded))
				return false;
			pRaster = arDecoded.data();
		}
		else if (nDataSize < nRasterSize)
			return false;

		BYTE* pFrameData = new BYTE[4 * (size_t)nWidth * nHeight];

		for (unsigned int nY = 0; nY < nHeight; ++nY)
		{
			const BYTE* pRow = pRaster + nRowBytes * nY;
			BYTE* pDst = pFrameData + 4 * (size_t)nWidth * nY;

			for (unsigned int nX = 0; nX < nWidth; ++nX, pDst += 4)
			{
				BYTE nR = 0, nG = 0, nB = 0;
				if (nDepth <= 8)
				{
					unsigned int nIndex = (1 == nDepth) ? ((pRow[nX / 8] >> (7 - nX % 8)) & 1) : pRow[nX];
					if (nMapColors)
					{
						if (nIndex < nMapColors)
						{
							nR = arMap[nIndex];
							nG = arMap[nMapColors + nIndex];
							nB = arMap[2 * nMapColors + nIndex];
						}
					}
					else if (1 == nDepth)
						nR = nG = nB = nIndex ? 0 : 255; // 1 is black
					else
						nR = nG = nB = (BYTE)nIndex;
				}
				else
				{
					// 32 bit pixels start with an unused byte
					const BYTE* pPixel = pRow + (size_t)nX * (nDepth / 8) + (32 == nDepth ? 1 : 0);
					if (rasTypeRGB == nType)
					{
						nR = pPixel[0];
						nG = pPixel[1];
						nB = pPixel[2];
					}
					else
					{
						nB = pPixel[0];
						nG = pPixel[1];
						nR = pPixel[2];
					}
				}

				pDst[bIsRGBA ? 0 : 2] = nR;
				pDst[1] = nG;
				pDst[bIsRGBA ? 2 : 0] = nB;
				pDst[3] = 255;
			}
		}

		pFrame->put_Width((int)nWidth);
		pFrame->put_Height((int)nHeight);
		pFrame->put_Stride(4 * (int)nWidth);
		pFrame->put_Data(pFrameData);
		bIsGrayScale = nDepth <= 8 && 0 == nMapColors;
		return true;
	}

	bool Open(CBgraFrame* pFrame, const std::wstring& sFileName, bool bIsRGBA, bool& bIsGrayScale)
	{
		BYTE* pBuffer = NULL;
		DWORD nSize = 0;
		if (!NSFile::CFileBinary::ReadAllBytes(sFileName, &pBuffer, nSize))
			return false;

		const bool bResult = Open(pFrame, pBuffer, nSize, bIsRGBA, bIsGrayScale);
		delete[] pBuffer;
		return bResult;
	}
}
