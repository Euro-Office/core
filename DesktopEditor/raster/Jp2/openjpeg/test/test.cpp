/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for openjpeg::GetData (opj_bgraframe.cpp), the JPEG 2000 decoder
// PdfFile uses for JPXDecode streams. The images are encoded in memory with
// OpenJPEG's own encoder, losslessly, so decoding must give them back exactly.

#include "gtest/gtest.h"

#include "../opj_bgraframe.h"
#include "openjpeg.h"

#include <cstring>
#include <vector>

namespace
{
	struct CImage
	{
		int nWidth = 0;
		int nHeight = 0;
		int nComponents = 0;
		std::vector<BYTE> arPixels; // interleaved, top row first
	};

	CImage CreateImage(int nWidth, int nHeight, int nComponents)
	{
		CImage oImage;
		oImage.nWidth = nWidth;
		oImage.nHeight = nHeight;
		oImage.nComponents = nComponents;
		oImage.arPixels.resize((size_t)nWidth * nHeight * nComponents);

		size_t nIndex = 0;
		for (int nY = 0; nY < nHeight; ++nY)
			for (int nX = 0; nX < nWidth; ++nX)
				for (int nC = 0; nC < nComponents; ++nC)
					oImage.arPixels[nIndex++] = (BYTE)((nX * 7 + nY * 13 + nC * 61 + (nX * nY) % 23) & 0xFF);
		return oImage;
	}

	CImage FlipVertically(const CImage& oImage)
	{
		CImage oResult = oImage;
		const size_t nRow = (size_t)oImage.nWidth * oImage.nComponents;
		for (int nY = 0; nY < oImage.nHeight; ++nY)
			memcpy(oResult.arPixels.data() + nY * nRow, oImage.arPixels.data() + (oImage.nHeight - 1 - nY) * nRow, nRow);
		return oResult;
	}

	// Growable in-memory output for the encoder
	struct CMemoryOutput
	{
		std::vector<BYTE> arData;
		size_t nPos = 0;

		void Reserve(size_t nEnd)
		{
			if (nEnd > arData.size())
				arData.resize(nEnd);
		}
	};

	OPJ_SIZE_T WriteToMemory(void* pBuffer, OPJ_SIZE_T nBytes, void* pUserData)
	{
		CMemoryOutput* pOutput = (CMemoryOutput*)pUserData;
		pOutput->Reserve(pOutput->nPos + nBytes);
		memcpy(pOutput->arData.data() + pOutput->nPos, pBuffer, nBytes);
		pOutput->nPos += nBytes;
		return nBytes;
	}

	OPJ_OFF_T SkipInMemory(OPJ_OFF_T nBytes, void* pUserData)
	{
		CMemoryOutput* pOutput = (CMemoryOutput*)pUserData;
		if (nBytes < 0)
			return -1;
		pOutput->nPos += (size_t)nBytes;
		pOutput->Reserve(pOutput->nPos);
		return nBytes;
	}

	OPJ_BOOL SeekInMemory(OPJ_OFF_T nPos, void* pUserData)
	{
		CMemoryOutput* pOutput = (CMemoryOutput*)pUserData;
		if (nPos < 0)
			return OPJ_FALSE;
		pOutput->nPos = (size_t)nPos;
		pOutput->Reserve(pOutput->nPos);
		return OPJ_TRUE;
	}

	// Lossless (reversible 5/3 wavelet, reversible colour transform)
	std::vector<BYTE> Encode(const CImage& oImage, OPJ_CODEC_FORMAT eFormat, int nResolutions, int nTileSize = 0)
	{
		opj_cparameters_t oParams;
		opj_set_default_encoder_parameters(&oParams);
		oParams.tcp_numlayers = 1;
		oParams.tcp_rates[0] = 0;
		oParams.cp_disto_alloc = 1;
		oParams.irreversible = 0;
		oParams.numresolution = nResolutions;
		oParams.tcp_mct = oImage.nComponents >= 3 ? 1 : 0;
		if (nTileSize > 0)
		{
			oParams.tile_size_on = OPJ_TRUE;
			oParams.cp_tdx = nTileSize;
			oParams.cp_tdy = nTileSize;
		}

		std::vector<opj_image_cmptparm_t> arComponents(oImage.nComponents);
		for (opj_image_cmptparm_t& oComponent : arComponents)
		{
			memset(&oComponent, 0, sizeof(oComponent));
			oComponent.dx = 1;
			oComponent.dy = 1;
			oComponent.w = oImage.nWidth;
			oComponent.h = oImage.nHeight;
			oComponent.prec = 8;
			oComponent.sgnd = 0;
		}

		opj_image_t* pImage = opj_image_create(oImage.nComponents, arComponents.data(), oImage.nComponents >= 3 ? OPJ_CLRSPC_SRGB : OPJ_CLRSPC_GRAY);
		if (!pImage)
			return {};
		pImage->x0 = 0;
		pImage->y0 = 0;
		pImage->x1 = oImage.nWidth;
		pImage->y1 = oImage.nHeight;
		if (4 == oImage.nComponents)
			pImage->comps[3].alpha = 1;

		for (int nC = 0; nC < oImage.nComponents; ++nC)
			for (size_t nI = 0, nCount = (size_t)oImage.nWidth * oImage.nHeight; nI < nCount; ++nI)
				pImage->comps[nC].data[nI] = oImage.arPixels[nI * oImage.nComponents + nC];

		CMemoryOutput oOutput;
		opj_codec_t* pCodec = opj_create_compress(eFormat);
		opj_stream_t* pStream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, OPJ_FALSE);
		opj_stream_set_user_data(pStream, &oOutput, NULL);
		opj_stream_set_write_function(pStream, WriteToMemory);
		opj_stream_set_skip_function(pStream, SkipInMemory);
		opj_stream_set_seek_function(pStream, SeekInMemory);

		const bool bResult = opj_setup_encoder(pCodec, &oParams, pImage)
			&& opj_start_compress(pCodec, pImage, pStream)
			&& opj_encode(pCodec, pStream)
			&& opj_end_compress(pCodec, pStream);

		opj_stream_destroy(pStream);
		opj_destroy_codec(pCodec);
		opj_image_destroy(pImage);

		return bResult ? oOutput.arData : std::vector<BYTE>();
	}

	bool Decode(std::vector<BYTE> arFile, bool bFlip, CImage& oResult)
	{
		BYTE* pData = NULL;
		int nComponents = 0, nWidth = 0, nHeight = 0;
		const bool bResult = openjpeg::GetData(arFile.data(), (unsigned int)arFile.size(), pData, nComponents, nWidth, nHeight, bFlip);
		if (bResult)
		{
			oResult.nWidth = nWidth;
			oResult.nHeight = nHeight;
			oResult.nComponents = nComponents;
			oResult.arPixels.assign(pData, pData + (size_t)nWidth * nHeight * nComponents);
		}
		openjpeg::DestroyData(pData);
		return bResult;
	}

	void ExpectEqual(const CImage& oExpected, const CImage& oActual)
	{
		ASSERT_EQ(oExpected.nWidth, oActual.nWidth);
		ASSERT_EQ(oExpected.nHeight, oActual.nHeight);
		ASSERT_EQ(oExpected.nComponents, oActual.nComponents);
		EXPECT_TRUE(oExpected.arPixels == oActual.arPixels);
	}
}

struct CRoundTripParam
{
	OPJ_CODEC_FORMAT eFormat;
	int nComponents;
};

class COpenJpegRoundTrip : public ::testing::TestWithParam<CRoundTripParam> {};

TEST_P(COpenJpegRoundTrip, DecodesLosslessImageExactly)
{
	const CImage oImage = CreateImage(37, 23, GetParam().nComponents);
	const std::vector<BYTE> arFile = Encode(oImage, GetParam().eFormat, 3);
	ASSERT_FALSE(arFile.empty());

	CImage oDecoded;
	ASSERT_TRUE(Decode(arFile, true, oDecoded));
	ExpectEqual(oImage, oDecoded);
}

INSTANTIATE_TEST_SUITE_P(Formats, COpenJpegRoundTrip, ::testing::Values(
	CRoundTripParam{OPJ_CODEC_J2K, 1},
	CRoundTripParam{OPJ_CODEC_J2K, 3},
	CRoundTripParam{OPJ_CODEC_J2K, 4},
	CRoundTripParam{OPJ_CODEC_JP2, 1},
	CRoundTripParam{OPJ_CODEC_JP2, 3},
	CRoundTripParam{OPJ_CODEC_JP2, 4}),
	[](const ::testing::TestParamInfo<CRoundTripParam>& oInfo) {
		return std::string(OPJ_CODEC_JP2 == oInfo.param.eFormat ? "Jp2" : "J2k") + "_" + std::to_string(oInfo.param.nComponents) + "Components";
	});

TEST(COpenJpegTest, DecodesTiledMultiResolutionImage)
{
	const CImage oImage = CreateImage(300, 200, 3);
	const std::vector<BYTE> arFile = Encode(oImage, OPJ_CODEC_JP2, 5, 64);
	ASSERT_FALSE(arFile.empty());

	CImage oDecoded;
	ASSERT_TRUE(Decode(arFile, true, oDecoded));
	ExpectEqual(oImage, oDecoded);
}

TEST(COpenJpegTest, WithoutFlipReturnsRowsBottomUp)
{
	const CImage oImage = CreateImage(40, 30, 3);
	const std::vector<BYTE> arFile = Encode(oImage, OPJ_CODEC_J2K, 3);
	ASSERT_FALSE(arFile.empty());

	CImage oDecoded;
	ASSERT_TRUE(Decode(arFile, false, oDecoded));
	ExpectEqual(FlipVertically(oImage), oDecoded);
}

// More than MEMORY_LIMIT_FOR_TILE (5,000,000) pixels: GetData decodes the
// image in horizontal strips with opj_set_decode_area.
TEST(COpenJpegTest, DecodesLargeImageInStrips)
{
	const CImage oImage = CreateImage(2600, 2000, 1);
	const std::vector<BYTE> arFile = Encode(oImage, OPJ_CODEC_J2K, 6, 512);
	ASSERT_FALSE(arFile.empty());

	CImage oFlipped;
	ASSERT_TRUE(Decode(arFile, true, oFlipped));
	ExpectEqual(oImage, oFlipped);

	CImage oUnflipped;
	ASSERT_TRUE(Decode(arFile, false, oUnflipped));
	ExpectEqual(FlipVertically(oImage), oUnflipped);
}

TEST(COpenJpegTest, RejectsDataThatIsNotJpeg2000)
{
	CImage oDecoded;
	EXPECT_FALSE(Decode({}, true, oDecoded));
	EXPECT_FALSE(Decode({0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A}, true, oDecoded));

	BYTE* pData = NULL;
	int nComponents = 0, nWidth = 0, nHeight = 0;
	EXPECT_FALSE(openjpeg::GetData(NULL, 0, pData, nComponents, nWidth, nHeight, true));
	EXPECT_EQ(NULL, pData);
}

// Broken streams must fail cleanly (or decode partially), never crash.
TEST(COpenJpegTest, HandlesTruncatedAndCorruptedStreams)
{
	const CImage oImage = CreateImage(64, 48, 3);
	for (OPJ_CODEC_FORMAT eFormat : {OPJ_CODEC_J2K, OPJ_CODEC_JP2})
	{
		const std::vector<BYTE> arFile = Encode(oImage, eFormat, 4, 32);
		ASSERT_FALSE(arFile.empty());

		CImage oDecoded;
		EXPECT_FALSE(Decode(std::vector<BYTE>(arFile.begin(), arFile.begin() + 4), true, oDecoded));
		EXPECT_FALSE(Decode(std::vector<BYTE>(arFile.begin(), arFile.begin() + 40), true, oDecoded));

		// The main header is intact, so a partial decode still has the right size
		for (size_t nSize : {arFile.size() / 2, arFile.size() - 1})
		{
			if (Decode(std::vector<BYTE>(arFile.begin(), arFile.begin() + nSize), true, oDecoded))
			{
				EXPECT_EQ(oImage.nWidth, oDecoded.nWidth);
				EXPECT_EQ(oImage.nHeight, oDecoded.nHeight);
			}
		}

		// Deterministic byte corruption; any result is fine as long as it returns
		for (unsigned int nSeed = 1; nSeed <= 50; ++nSeed)
		{
			std::vector<BYTE> arCorrupted = arFile;
			unsigned int nState = nSeed;
			for (int nI = 0; nI < 8; ++nI)
			{
				nState = nState * 1103515245u + 12345u;
				const size_t nPos = 16 + (nState >> 8) % (arCorrupted.size() - 16);
				arCorrupted[nPos] ^= (BYTE)(nState >> 24 | 1);
			}
			Decode(arCorrupted, true, oDecoded);
		}
	}
}
