/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for the PNM (raster/Pnm) and Sun Raster (raster/Ras) decoders behind
// CBgraFrame, which replaced CxImage's JasPer based decoding. The images are
// built in memory, so every expected pixel is known.

#include "gtest/gtest.h"

#include "../BgraFrame.h"
#include "../ImageFileFormatChecker.h"
#include "../../common/File.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace
{
	struct CColor
	{
		int r, g, b;
		bool operator==(const CColor& o) const { return r == o.r && g == o.g && b == o.b; }
	};

	void PrintTo(const CColor& c, std::ostream* os)
	{
		*os << "(" << c.r << ", " << c.g << ", " << c.b << ")";
	}

	const CColor c_oBlack = {0, 0, 0};
	const CColor c_oWhite = {255, 255, 255};

	std::vector<BYTE> Bytes(const std::string& s)
	{
		return std::vector<BYTE>(s.begin(), s.end());
	}

	bool Decode(std::vector<BYTE> arData, CBgraFrame& oFrame, bool bIsRGBA = false)
	{
		oFrame.put_IsRGBA(bIsRGBA);
		return oFrame.Decode(arData.data(), (int)arData.size());
	}

	CColor Pixel(CBgraFrame& oFrame, int nX, int nY, bool bIsRGBA = false)
	{
		const int nStride = oFrame.get_Stride();
		const int nRow = nStride >= 0 ? nY : oFrame.get_Height() - 1 - nY;
		const BYTE* p = oFrame.get_Data() + (size_t)nRow * std::abs(nStride) + 4 * nX;
		EXPECT_EQ(255, p[3]);
		return bIsRGBA ? CColor{p[0], p[1], p[2]} : CColor{p[2], p[1], p[0]};
	}

	int FileType(std::vector<BYTE> arData)
	{
		CImageFileFormatChecker oChecker(arData.data(), (DWORD)arData.size());
		return oChecker.eFileType;
	}

	void PutUInt32BE(std::vector<BYTE>& arData, unsigned int nValue)
	{
		arData.push_back((BYTE)(nValue >> 24));
		arData.push_back((BYTE)(nValue >> 16));
		arData.push_back((BYTE)(nValue >> 8));
		arData.push_back((BYTE)nValue);
	}

	std::vector<BYTE> MakeRas(unsigned int nWidth, unsigned int nHeight, unsigned int nDepth, unsigned int nType,
							  const std::vector<BYTE>& arMap, const std::vector<BYTE>& arRaster)
	{
		std::vector<BYTE> arData;
		PutUInt32BE(arData, 0x59a66a95);
		PutUInt32BE(arData, nWidth);
		PutUInt32BE(arData, nHeight);
		PutUInt32BE(arData, nDepth);
		PutUInt32BE(arData, (unsigned int)arRaster.size());
		PutUInt32BE(arData, nType);
		PutUInt32BE(arData, arMap.empty() ? 0 : 1);
		PutUInt32BE(arData, (unsigned int)arMap.size());
		arData.insert(arData.end(), arMap.begin(), arMap.end());
		arData.insert(arData.end(), arRaster.begin(), arRaster.end());
		return arData;
	}
}

// PNM

TEST(CPnmTest, DetectsAllNetpbmVariants)
{
	for (const char* sHeader : {"P1 1 1\n1", "P2 1 1 255\n0", "P3 1 1 255\n0 0 0", "P4 1 1\n", "P5 1 1 255\n", "P6 1 1 255\n"})
		EXPECT_EQ(_CXIMAGE_FORMAT_PNM, FileType(Bytes(sHeader))) << sHeader;

	EXPECT_NE(_CXIMAGE_FORMAT_PNM, FileType(Bytes("P7 1 1 255\n")));
	EXPECT_NE(_CXIMAGE_FORMAT_PNM, FileType(Bytes("PNG image")));
}

TEST(CPnmTest, DecodesAsciiBitmapWithCommentsAndPackedBits)
{
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(Bytes("P1\n# comment\n3 2\n101\n0 1 0\n"), oFrame));
	ASSERT_EQ(3, oFrame.get_Width());
	ASSERT_EQ(2, oFrame.get_Height());
	EXPECT_TRUE(oFrame.IsGrayScale());
	// 1 is black
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 0, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 1, 0));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 2, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 0, 1));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 1, 1));
}

TEST(CPnmTest, DecodesBinaryBitmapWithRowPadding)
{
	// 10 pixels per row: 2 bytes, the last 6 bits are padding
	std::vector<BYTE> arData = Bytes("P4\n10 2\n");
	arData.insert(arData.end(), {0xA0, 0x80, 0x00, 0x40});

	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(arData, oFrame));
	ASSERT_EQ(10, oFrame.get_Width());
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 0, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 1, 0));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 2, 0));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 8, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 9, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 8, 1));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 9, 1));
}

TEST(CPnmTest, ScalesGraySamplesToMaxValue)
{
	CBgraFrame oAscii;
	ASSERT_TRUE(Decode(Bytes("P2 3 1 15\n0 5 15\n"), oAscii));
	EXPECT_TRUE(oAscii.IsGrayScale());
	EXPECT_EQ(c_oBlack, Pixel(oAscii, 0, 0));
	EXPECT_EQ((CColor{85, 85, 85}), Pixel(oAscii, 1, 0));
	EXPECT_EQ(c_oWhite, Pixel(oAscii, 2, 0));

	// 16 bit samples are big-endian
	std::vector<BYTE> arData = Bytes("P5 2 1 65535\n");
	arData.insert(arData.end(), {0x80, 0x00, 0xFF, 0xFF});
	CBgraFrame oBinary;
	ASSERT_TRUE(Decode(arData, oBinary));
	EXPECT_EQ((CColor{128, 128, 128}), Pixel(oBinary, 0, 0));
	EXPECT_EQ(c_oWhite, Pixel(oBinary, 1, 0));
}

TEST(CPnmTest, DecodesColorInBothChannelOrders)
{
	std::vector<BYTE> arBinary = Bytes("P6 2 1 255\n");
	arBinary.insert(arBinary.end(), {10, 20, 30, 200, 150, 100});

	for (bool bIsRGBA : {false, true})
	{
		CBgraFrame oAscii;
		ASSERT_TRUE(Decode(Bytes("P3 2 1 255\n10 20 30  200 150 100\n"), oAscii, bIsRGBA));
		EXPECT_FALSE(oAscii.IsGrayScale());
		EXPECT_EQ((CColor{10, 20, 30}), Pixel(oAscii, 0, 0, bIsRGBA));
		EXPECT_EQ((CColor{200, 150, 100}), Pixel(oAscii, 1, 0, bIsRGBA));

		CBgraFrame oBinary;
		ASSERT_TRUE(Decode(arBinary, oBinary, bIsRGBA));
		EXPECT_EQ((CColor{10, 20, 30}), Pixel(oBinary, 0, 0, bIsRGBA));
		EXPECT_EQ((CColor{200, 150, 100}), Pixel(oBinary, 1, 0, bIsRGBA));
	}
}

TEST(CPnmTest, OpensFile)
{
	std::vector<BYTE> arData = Bytes("P6 1 1 255\n");
	arData.insert(arData.end(), {1, 2, 3});

	const std::wstring sPath = NSFile::CFileBinary::CreateTempFileWithUniqueName(NSFile::CFileBinary::GetTempPath(), L"pnm");
	NSFile::CFileBinary oFile;
	ASSERT_TRUE(oFile.CreateFileW(sPath));
	oFile.WriteFile(arData.data(), (DWORD)arData.size());
	oFile.CloseFile();

	CBgraFrame oFrame;
	EXPECT_TRUE(oFrame.OpenFile(sPath));
	NSFile::CFileBinary::Remove(sPath);
	ASSERT_EQ(1, oFrame.get_Width());
	EXPECT_EQ((CColor{1, 2, 3}), Pixel(oFrame, 0, 0));
}

TEST(CPnmTest, RejectsInvalidImages)
{
	CBgraFrame oFrame;
	EXPECT_FALSE(Decode(Bytes("P6 2 2 255\n123"), oFrame));        // truncated raster
	EXPECT_FALSE(Decode(Bytes("P3 2 1 255\n1 2 3 4 5"), oFrame));  // truncated ASCII
	EXPECT_FALSE(Decode(Bytes("P5 1 1 0\n\x01"), oFrame));          // maxval 0
	EXPECT_FALSE(Decode(Bytes("P5 1 1 70000\n\x01"), oFrame));      // maxval > 65535
	EXPECT_FALSE(Decode(Bytes("P5 0 1 255\n"), oFrame));            // empty
	EXPECT_FALSE(Decode(Bytes("P6 100000 100000 255\n"), oFrame));  // too large
	EXPECT_FALSE(Decode(Bytes("P6 99999999999 1 255\n"), oFrame));  // overflowing size
}

// Sun Raster

TEST(CRasTest, DetectsSupportedDepths)
{
	for (unsigned int nDepth : {1u, 8u, 24u, 32u})
		EXPECT_EQ(_CXIMAGE_FORMAT_RAS, FileType(MakeRas(1, 1, nDepth, 1, {}, {0, 0, 0, 0}))) << nDepth;

	EXPECT_NE(_CXIMAGE_FORMAT_RAS, FileType(MakeRas(1, 1, 16, 1, {}, {0, 0})));
}

TEST(CRasTest, DecodesStandardBgrWithRowPadding)
{
	// 3 pixels of 3 bytes = 9 bytes, padded to 10
	std::vector<BYTE> arRaster = {
		30, 20, 10,  0, 0, 0,  255, 255, 255,  0,
		1, 2, 3,  4, 5, 6,  7, 8, 9,  0
	};
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(MakeRas(3, 2, 24, 1, {}, arRaster), oFrame));
	ASSERT_EQ(3, oFrame.get_Width());
	ASSERT_EQ(2, oFrame.get_Height());
	EXPECT_FALSE(oFrame.IsGrayScale());
	EXPECT_EQ((CColor{10, 20, 30}), Pixel(oFrame, 0, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 2, 0));
	EXPECT_EQ((CColor{3, 2, 1}), Pixel(oFrame, 0, 1));
	EXPECT_EQ((CColor{9, 8, 7}), Pixel(oFrame, 2, 1));
}

TEST(CRasTest, DecodesRgbTypeAnd32Bit)
{
	CBgraFrame oRgb;
	ASSERT_TRUE(Decode(MakeRas(1, 1, 24, 3, {}, {10, 20, 30, 0}), oRgb));
	EXPECT_EQ((CColor{10, 20, 30}), Pixel(oRgb, 0, 0));

	// 32 bit: unused byte, then B, G, R (standard) or R, G, B (RGB type)
	CBgraFrame oStandard32;
	ASSERT_TRUE(Decode(MakeRas(1, 1, 32, 1, {}, {0xFF, 30, 20, 10}), oStandard32));
	EXPECT_EQ((CColor{10, 20, 30}), Pixel(oStandard32, 0, 0));

	CBgraFrame oRgb32;
	ASSERT_TRUE(Decode(MakeRas(1, 1, 32, 3, {}, {0xFF, 10, 20, 30}), oRgb32));
	EXPECT_EQ((CColor{10, 20, 30}), Pixel(oRgb32, 0, 0));
}

TEST(CRasTest, DecodesGrayAndPalette)
{
	CBgraFrame oGray;
	ASSERT_TRUE(Decode(MakeRas(2, 1, 8, 1, {}, {0, 200}), oGray));
	EXPECT_TRUE(oGray.IsGrayScale());
	EXPECT_EQ((CColor{200, 200, 200}), Pixel(oGray, 1, 0));

	// Color map: 2 reds, 2 greens, 2 blues
	CBgraFrame oPalette;
	ASSERT_TRUE(Decode(MakeRas(2, 1, 8, 1, {255, 0, 0, 128, 0, 255}, {0, 1}), oPalette));
	EXPECT_FALSE(oPalette.IsGrayScale());
	EXPECT_EQ((CColor{255, 0, 0}), Pixel(oPalette, 0, 0));
	EXPECT_EQ((CColor{0, 128, 255}), Pixel(oPalette, 1, 0));
}

TEST(CRasTest, DecodesMonochrome)
{
	// 1 is black; 3 pixels in 1 byte, padded to 2
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(MakeRas(3, 1, 1, 1, {}, {0xA0, 0x00}), oFrame));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 0, 0));
	EXPECT_EQ(c_oWhite, Pixel(oFrame, 1, 0));
	EXPECT_EQ(c_oBlack, Pixel(oFrame, 2, 0));
}

TEST(CRasTest, DecodesRunLengthEncoding)
{
	// 8 bit, 6 pixels: 5 x 7 (0x80 4 7), a literal 0x80 (0x80 0)
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(MakeRas(6, 1, 8, 2, {}, {0x80, 4, 7, 0x80, 0}), oFrame));
	for (int nX = 0; nX < 5; ++nX)
		EXPECT_EQ((CColor{7, 7, 7}), Pixel(oFrame, nX, 0));
	EXPECT_EQ((CColor{128, 128, 128}), Pixel(oFrame, 5, 0));
}

TEST(CRasTest, DecodesLongestRleRun)
{
	// 3 bytes for 256 pixels, the highest ratio the format allows
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(MakeRas(256, 1, 8, 2, {}, {0x80, 255, 7}), oFrame));
	EXPECT_EQ((CColor{7, 7, 7}), Pixel(oFrame, 255, 0));
}

TEST(CRasTest, RejectsInvalidImages)
{
	CBgraFrame oFrame;
	EXPECT_FALSE(Decode(MakeRas(4, 4, 24, 1, {}, {1, 2, 3}), oFrame));                 // truncated
	EXPECT_FALSE(Decode(MakeRas(4, 1, 8, 2, {}, {0x80, 1}), oFrame));                  // truncated RLE
	EXPECT_FALSE(Decode(MakeRas(16384, 16384, 32, 2, {}, {}), oFrame));                // RLE without data
	EXPECT_FALSE(Decode(MakeRas(1, 1, 24, 7, {}, {1, 2, 3, 0}), oFrame));              // unknown type
	EXPECT_FALSE(Decode(MakeRas(0, 1, 24, 1, {}, {}), oFrame));                        // empty
	EXPECT_FALSE(Decode(MakeRas(100000, 100000, 24, 1, {}, {1, 2, 3, 0}), oFrame));    // too large

	std::vector<BYTE> arBadMap = MakeRas(1, 1, 8, 1, {}, {0});
	arBadMap[31] = 200; // color map longer than the file
	EXPECT_FALSE(Decode(arBadMap, oFrame));
}
