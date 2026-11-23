/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "PnmFile.h"
#include "../../common/File.h"

namespace NSPnm
{
	// Same limit as CxImage (CXIMAGE_MAX_MEMORY) for the decoded BGRA buffer
	static const unsigned long long c_nMaxFrameBytes = 1073741824;

	static bool IsSpace(BYTE c)
	{
		return ' ' == c || '\t' == c || '\n' == c || '\r' == c || '\v' == c || '\f' == c;
	}

	static bool IsDigit(BYTE c)
	{
		return c >= '0' && c <= '9';
	}

	class CReader
	{
	public:
		CReader(const BYTE* pBuffer, size_t nSize) : m_pBuffer(pBuffer), m_nSize(nSize), m_nPos(0) {}

		// Skips whitespace and '#' comments between header fields and ASCII samples
		void SkipSpace()
		{
			while (m_nPos < m_nSize)
			{
				if ('#' == m_pBuffer[m_nPos])
				{
					while (m_nPos < m_nSize && '\n' != m_pBuffer[m_nPos] && '\r' != m_pBuffer[m_nPos])
						++m_nPos;
				}
				else if (IsSpace(m_pBuffer[m_nPos]))
					++m_nPos;
				else
					break;
			}
		}

		bool ReadNumber(unsigned int& nValue)
		{
			SkipSpace();
			if (m_nPos >= m_nSize || !IsDigit(m_pBuffer[m_nPos]))
				return false;

			unsigned long long nResult = 0;
			while (m_nPos < m_nSize && IsDigit(m_pBuffer[m_nPos]))
			{
				nResult = nResult * 10 + (m_pBuffer[m_nPos++] - '0');
				if (nResult > 0xFFFFFFFF)
					return false;
			}
			nValue = (unsigned int)nResult;
			return true;
		}

		// P1 allows bits without separators ("0110")
		bool ReadBit(unsigned int& nValue)
		{
			SkipSpace();
			if (m_nPos >= m_nSize || ('0' != m_pBuffer[m_nPos] && '1' != m_pBuffer[m_nPos]))
				return false;
			nValue = m_pBuffer[m_nPos++] - '0';
			return true;
		}

		// Binary rasters start after exactly one whitespace character
		bool SkipSingleSpace()
		{
			if (m_nPos >= m_nSize || !IsSpace(m_pBuffer[m_nPos]))
				return false;
			++m_nPos;
			return true;
		}

		const BYTE* Current() const { return m_pBuffer + m_nPos; }
		size_t Remaining() const { return m_nSize - m_nPos; }

	private:
		const BYTE* m_pBuffer;
		size_t m_nSize;
		size_t m_nPos;
	};

	bool IsPnm(const BYTE* pBuffer, size_t nSize)
	{
		if (!pBuffer || nSize < 3)
			return false;
		return 'P' == pBuffer[0] && pBuffer[1] >= '1' && pBuffer[1] <= '6' && IsSpace(pBuffer[2]);
	}

	bool Open(CBgraFrame* pFrame, const BYTE* pBuffer, size_t nSize, bool bIsRGBA, bool& bIsGrayScale)
	{
		if (!pFrame || !IsPnm(pBuffer, nSize))
			return false;

		const int nFormat = pBuffer[1] - '0';
		const bool bIsBinary = nFormat >= 4;
		const bool bIsBitmap = 1 == nFormat || 4 == nFormat;
		const bool bIsColor = 3 == nFormat || 6 == nFormat;

		CReader oReader(pBuffer + 2, nSize - 2);

		unsigned int nWidth = 0, nHeight = 0, nMaxValue = 1;
		if (!oReader.ReadNumber(nWidth) || !oReader.ReadNumber(nHeight))
			return false;
		if (!bIsBitmap && !oReader.ReadNumber(nMaxValue))
			return false;
		if (0 == nWidth || 0 == nHeight || 0 == nMaxValue || nMaxValue > 65535)
			return false;
		if (4ULL * nWidth * nHeight > c_nMaxFrameBytes)
			return false;
		if (bIsBinary && !oReader.SkipSingleSpace())
			return false;

		const int nChannels = bIsColor ? 3 : 1;
		const size_t nSampleBytes = nMaxValue > 255 ? 2 : 1;
		const size_t nBitmapRowBytes = (nWidth + 7) / 8;

		if (bIsBinary)
		{
			unsigned long long nNeeded = bIsBitmap
				? (unsigned long long)nBitmapRowBytes * nHeight
				: (unsigned long long)nWidth * nHeight * nChannels * nSampleBytes;
			if (oReader.Remaining() < nNeeded)
				return false;
		}

		BYTE* pData = new BYTE[4 * (size_t)nWidth * nHeight];
		const BYTE* pRaster = oReader.Current();
		size_t nRasterPos = 0;

		for (unsigned int nY = 0; nY < nHeight; ++nY)
		{
			BYTE* pDst = pData + 4 * (size_t)nWidth * nY;
			for (unsigned int nX = 0; nX < nWidth; ++nX, pDst += 4)
			{
				BYTE arRGB[3];
				if (bIsBitmap)
				{
					unsigned int nBit = 0;
					if (bIsBinary)
						nBit = (pRaster[nBitmapRowBytes * nY + nX / 8] >> (7 - nX % 8)) & 1;
					else if (!oReader.ReadBit(nBit))
					{
						delete[] pData;
						return false;
					}
					// 1 is black
					arRGB[0] = arRGB[1] = arRGB[2] = nBit ? 0 : 255;
				}
				else
				{
					for (int nC = 0; nC < nChannels; ++nC)
					{
						unsigned int nValue = 0;
						if (bIsBinary)
						{
							nValue = pRaster[nRasterPos++];
							if (2 == nSampleBytes)
								nValue = (nValue << 8) | pRaster[nRasterPos++];
						}
						else if (!oReader.ReadNumber(nValue))
						{
							delete[] pData;
							return false;
						}
						if (nValue > nMaxValue)
							nValue = nMaxValue;
						arRGB[nC] = (BYTE)((nValue * 255 + nMaxValue / 2) / nMaxValue);
					}
					if (1 == nChannels)
						arRGB[1] = arRGB[2] = arRGB[0];
				}

				pDst[bIsRGBA ? 0 : 2] = arRGB[0];
				pDst[1] = arRGB[1];
				pDst[bIsRGBA ? 2 : 0] = arRGB[2];
				pDst[3] = 255;
			}
		}

		pFrame->put_Width((int)nWidth);
		pFrame->put_Height((int)nHeight);
		pFrame->put_Stride(4 * (int)nWidth);
		pFrame->put_Data(pData);
		bIsGrayScale = !bIsColor;
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
