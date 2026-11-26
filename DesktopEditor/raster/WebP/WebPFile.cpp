/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "WebPFile.h"
#include "../../common/File.h"

#include <webp/decode.h>
#include <webp/demux.h>

#include <cstring>

namespace NSWebP
{
	// Same limit as CxImage (CXIMAGE_MAX_MEMORY) for the decoded BGRA buffer
	static const unsigned long long c_nMaxFrameBytes = 1073741824;

	// The first frame as shown, composited onto the canvas
	static BYTE* DecodeFirstFrame(const WebPData& oData, bool bIsRGBA, int nWidth, int nHeight)
	{
		WebPAnimDecoderOptions oOptions;
		if (!WebPAnimDecoderOptionsInit(&oOptions))
			return NULL;
		oOptions.color_mode = bIsRGBA ? MODE_RGBA : MODE_BGRA;

		WebPAnimDecoder* pDecoder = WebPAnimDecoderNew(&oData, &oOptions);
		if (!pDecoder)
			return NULL;

		BYTE* pData = NULL;
		uint8_t* pCanvas = NULL;
		int nTimestamp = 0;
		if (WebPAnimDecoderGetNext(pDecoder, &pCanvas, &nTimestamp))
		{
			const size_t nBytes = 4 * (size_t)nWidth * nHeight;
			pData = new BYTE[nBytes];
			memcpy(pData, pCanvas, nBytes);
		}

		WebPAnimDecoderDelete(pDecoder);
		return pData;
	}

	bool Open(CBgraFrame* pFrame, const BYTE* pBuffer, size_t nSize, bool bIsRGBA)
	{
		WebPBitstreamFeatures oFeatures;
		if (!pBuffer || VP8_STATUS_OK != WebPGetFeatures(pBuffer, nSize, &oFeatures))
			return false;

		const int nWidth = oFeatures.width;
		const int nHeight = oFeatures.height;
		if (nWidth <= 0 || nHeight <= 0 || 4ULL * nWidth * nHeight > c_nMaxFrameBytes)
			return false;

		BYTE* pData = NULL;
		if (oFeatures.has_animation)
		{
			WebPData oData = {pBuffer, nSize};
			pData = DecodeFirstFrame(oData, bIsRGBA, nWidth, nHeight);
			if (!pData)
				return false;
		}
		else
		{
			const size_t nBytes = 4 * (size_t)nWidth * nHeight;
			pData = new BYTE[nBytes];
			const uint8_t* pResult = bIsRGBA ? WebPDecodeRGBAInto(pBuffer, nSize, pData, nBytes, 4 * nWidth)
											 : WebPDecodeBGRAInto(pBuffer, nSize, pData, nBytes, 4 * nWidth);
			if (!pResult)
			{
				delete[] pData;
				return false;
			}
		}

		pFrame->put_Width(nWidth);
		pFrame->put_Height(nHeight);
		pFrame->put_Stride(4 * nWidth);
		pFrame->put_Data(pData);
		return true;
	}

	bool Open(CBgraFrame* pFrame, const std::wstring& sFileName, bool bIsRGBA)
	{
		BYTE* pBuffer = NULL;
		DWORD nSize = 0;
		if (!NSFile::CFileBinary::ReadAllBytes(sFileName, &pBuffer, nSize))
			return false;

		const bool bResult = Open(pFrame, pBuffer, nSize, bIsRGBA);
		delete[] pBuffer;
		return bResult;
	}
}
