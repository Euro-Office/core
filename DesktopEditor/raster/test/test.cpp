/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for the image formats behind CBgraFrame: the PNM (raster/Pnm) and Sun
// Raster (raster/Ras) decoders, which replaced CxImage's JasPer based decoding,
// and CxImage's JPEG codec on top of libjpeg-turbo. The images are built in
// memory, so every expected pixel is known.

#include "gtest/gtest.h"

#include "../BgraFrame.h"
#include "../ImageFileFormatChecker.h"
#include "../../common/File.h"

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include <jpeglib.h>

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

	void ExpectNear(const CColor& oExpected, const CColor& oActual, int nTolerance)
	{
		EXPECT_NEAR(oExpected.r, oActual.r, nTolerance) << "red";
		EXPECT_NEAR(oExpected.g, oActual.g, nTolerance) << "green";
		EXPECT_NEAR(oExpected.b, oActual.b, nTolerance) << "blue";
	}

	// 32x32 image of four 16x16 quadrants, so each quadrant is a whole MCU even
	// with 4:2:0 chroma subsampling and keeps its own color
	const CColor c_arQuadrants[4] = {{220, 30, 40}, {30, 200, 60}, {20, 40, 210}, {128, 128, 128}};
	const int c_nQuadImageSize = 32;

	CColor QuadrantColor(int nX, int nY)
	{
		return c_arQuadrants[(nY >= 16 ? 2 : 0) + (nX >= 16 ? 1 : 0)];
	}

	struct CJpegOptions
	{
		J_COLOR_SPACE eColorSpace = JCS_RGB;
		bool bProgressive = false;
		bool bArithmetic = false;
	};

	// Encodes a c_nQuadImageSize square at quality 100 without chroma
	// subsampling. fSample returns the components of one pixel.
	std::vector<BYTE> MakeJpeg(const CJpegOptions& oOptions, const std::function<std::vector<BYTE>(int, int)>& fSample)
	{
		jpeg_compress_struct cinfo;
		jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);

		unsigned char* pOut = NULL;
		unsigned long nOutSize = 0;
		jpeg_mem_dest(&cinfo, &pOut, &nOutSize);

		cinfo.image_width = c_nQuadImageSize;
		cinfo.image_height = c_nQuadImageSize;
		cinfo.in_color_space = oOptions.eColorSpace;
		cinfo.input_components = JCS_GRAYSCALE == oOptions.eColorSpace ? 1 : (JCS_CMYK == oOptions.eColorSpace ? 4 : 3);
		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, 100, TRUE);
		for (int i = 0; i < cinfo.num_components; ++i)
			cinfo.comp_info[i].h_samp_factor = cinfo.comp_info[i].v_samp_factor = 1;
		if (oOptions.bProgressive)
			jpeg_simple_progression(&cinfo);
		cinfo.arith_code = oOptions.bArithmetic ? TRUE : FALSE;

		jpeg_start_compress(&cinfo, TRUE);
		std::vector<BYTE> arRow;
		while (cinfo.next_scanline < cinfo.image_height)
		{
			arRow.clear();
			for (int nX = 0; nX < c_nQuadImageSize; ++nX)
			{
				std::vector<BYTE> arPixel = fSample(nX, (int)cinfo.next_scanline);
				arRow.insert(arRow.end(), arPixel.begin(), arPixel.end());
			}
			JSAMPROW pRow = arRow.data();
			jpeg_write_scanlines(&cinfo, &pRow, 1);
		}
		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);

		std::vector<BYTE> arData(pOut, pOut + nOutSize);
		free(pOut);
		return arData;
	}

	std::vector<BYTE> MakeQuadrantJpeg(const CJpegOptions& oOptions = CJpegOptions())
	{
		return MakeJpeg(oOptions, [](int nX, int nY) {
			CColor c = QuadrantColor(nX, nY);
			return std::vector<BYTE>{(BYTE)c.r, (BYTE)c.g, (BYTE)c.b};
		});
	}

	// Left half 40, right half 230
	std::vector<BYTE> MakeGrayJpeg()
	{
		CJpegOptions oOptions;
		oOptions.eColorSpace = JCS_GRAYSCALE;
		return MakeJpeg(oOptions, [](int nX, int) { return std::vector<BYTE>{(BYTE)(nX < 16 ? 40 : 230)}; });
	}

	void PutUInt16LE(std::vector<BYTE>& arData, unsigned int nValue)
	{
		arData.push_back((BYTE)nValue);
		arData.push_back((BYTE)(nValue >> 8));
	}

	void PutUInt32LE(std::vector<BYTE>& arData, unsigned int nValue)
	{
		PutUInt16LE(arData, nValue & 0xFFFF);
		PutUInt16LE(arData, nValue >> 16);
	}

	// Little-endian TIFF with one strip: a grayscale c_nQuadImageSize square
	// JPEG (compression 7)
	std::vector<BYTE> MakeJpegTiff(const std::vector<BYTE>& arJpeg)
	{
		const unsigned int arTags[][3] = {
			// tag, type (3 short, 4 long), value
			{256, 4, (unsigned int)c_nQuadImageSize}, // ImageWidth
			{257, 4, (unsigned int)c_nQuadImageSize}, // ImageLength
			{258, 3, 8},                              // BitsPerSample
			{259, 3, 7},                              // Compression: JPEG
			{262, 3, 1},                              // PhotometricInterpretation: BlackIsZero
			{273, 4, 0},                              // StripOffsets, set below
			{277, 3, 1},                              // SamplesPerPixel
			{278, 4, (unsigned int)c_nQuadImageSize}, // RowsPerStrip
			{279, 4, (unsigned int)arJpeg.size()}     // StripByteCounts
		};
		const unsigned int nTags = sizeof(arTags) / sizeof(arTags[0]);
		const unsigned int nDataOffset = 8 + 2 + 12 * nTags + 4;

		std::vector<BYTE> arData = {'I', 'I', 42, 0};
		PutUInt32LE(arData, 8);
		PutUInt16LE(arData, nTags);
		for (const auto& arTag : arTags)
		{
			PutUInt16LE(arData, arTag[0]);
			PutUInt16LE(arData, arTag[1]);
			PutUInt32LE(arData, 1);
			const unsigned int nValue = 273 == arTag[0] ? nDataOffset : arTag[2];
			if (3 == arTag[1])
			{
				PutUInt16LE(arData, nValue);
				PutUInt16LE(arData, 0);
			}
			else
				PutUInt32LE(arData, nValue);
		}
		PutUInt32LE(arData, 0); // no next IFD
		arData.insert(arData.end(), arJpeg.begin(), arJpeg.end());
		return arData;
	}

	void ExpectQuadrants(CBgraFrame& oFrame, bool bIsRGBA = false)
	{
		ASSERT_EQ(c_nQuadImageSize, oFrame.get_Width());
		ASSERT_EQ(c_nQuadImageSize, oFrame.get_Height());
		for (int nY : {4, 27})
			for (int nX : {4, 27})
				ExpectNear(QuadrantColor(nX, nY), Pixel(oFrame, nX, nY, bIsRGBA), 4);
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

// JPEG

TEST(CJpegTest, DetectsJpeg)
{
	EXPECT_EQ(_CXIMAGE_FORMAT_JPG, FileType(MakeQuadrantJpeg()));
}

TEST(CJpegTest, DecodesBaselineInBothChannelOrders)
{
	for (bool bIsRGBA : {false, true})
	{
		CBgraFrame oFrame;
		ASSERT_TRUE(Decode(MakeQuadrantJpeg(), oFrame, bIsRGBA));
		EXPECT_FALSE(oFrame.IsGrayScale());
		ExpectQuadrants(oFrame, bIsRGBA);
	}
}

TEST(CJpegTest, DecodesProgressiveAndArithmeticCoding)
{
	CJpegOptions oProgressive;
	oProgressive.bProgressive = true;
	CBgraFrame oProgressiveFrame;
	ASSERT_TRUE(Decode(MakeQuadrantJpeg(oProgressive), oProgressiveFrame));
	ExpectQuadrants(oProgressiveFrame);

	CJpegOptions oArithmetic;
	oArithmetic.bArithmetic = true;
	CBgraFrame oArithmeticFrame;
	ASSERT_TRUE(Decode(MakeQuadrantJpeg(oArithmetic), oArithmeticFrame));
	ExpectQuadrants(oArithmeticFrame);
}

TEST(CJpegTest, DecodesGrayscale)
{
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(MakeGrayJpeg(), oFrame));
	EXPECT_TRUE(oFrame.IsGrayScale());
	ExpectNear({40, 40, 40}, Pixel(oFrame, 4, 4), 2);
	ExpectNear({230, 230, 230}, Pixel(oFrame, 27, 27), 2);
}

TEST(CJpegTest, DecodesJpegCompressedTiff)
{
	// libtiff decodes the strip with libjpeg, so both must agree on its API
	std::vector<BYTE> arJpeg = MakeGrayJpeg();
	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(MakeJpegTiff(arJpeg), oFrame));
	ASSERT_EQ(c_nQuadImageSize, oFrame.get_Width());
	ASSERT_EQ(c_nQuadImageSize, oFrame.get_Height());
	ExpectNear({40, 40, 40}, Pixel(oFrame, 4, 4), 2);
	ExpectNear({230, 230, 230}, Pixel(oFrame, 27, 27), 2);
}

TEST(CJpegTest, DecodesAdobeCmyk)
{
	// libjpeg writes CMYK with an Adobe marker. Like Photoshop's files, the
	// samples are inverted, so CxImage takes R = C * K / 255 and so on.
	CJpegOptions oOptions;
	oOptions.eColorSpace = JCS_CMYK;
	std::vector<BYTE> arData = MakeJpeg(oOptions, [](int nX, int) {
		return nX < 16 ? std::vector<BYTE>{200, 100, 50, 255} : std::vector<BYTE>{255, 255, 255, 128};
	});

	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(arData, oFrame));
	ExpectNear({200, 100, 50}, Pixel(oFrame, 4, 4), 4);
	ExpectNear({128, 128, 128}, Pixel(oFrame, 27, 27), 4);
}

TEST(CJpegTest, EncodesJpeg)
{
	BYTE* pPixels = new BYTE[4 * c_nQuadImageSize * c_nQuadImageSize];
	for (int nY = 0; nY < c_nQuadImageSize; ++nY)
	{
		for (int nX = 0; nX < c_nQuadImageSize; ++nX)
		{
			BYTE* p = pPixels + 4 * (nY * c_nQuadImageSize + nX);
			CColor c = QuadrantColor(nX, nY);
			p[0] = (BYTE)c.b;
			p[1] = (BYTE)c.g;
			p[2] = (BYTE)c.r;
			p[3] = 255;
		}
	}

	CBgraFrame oSource;
	oSource.put_Width(c_nQuadImageSize);
	oSource.put_Height(c_nQuadImageSize);
	oSource.put_Stride(4 * c_nQuadImageSize);
	oSource.put_Data(pPixels);
	oSource.SetJpegQuality(100);

	BYTE* pEncoded = NULL;
	int nEncodedSize = 0;
	ASSERT_TRUE(oSource.Encode(pEncoded, nEncodedSize, _CXIMAGE_FORMAT_JPG));
	std::vector<BYTE> arEncoded(pEncoded, pEncoded + nEncodedSize);
	CBgraFrame::FreeEncodedMemory(pEncoded);

	// Read back with libjpeg itself, not through CxImage
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, arEncoded.data(), (unsigned long)arEncoded.size());
	ASSERT_EQ(JPEG_HEADER_OK, jpeg_read_header(&cinfo, TRUE));
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress(&cinfo);
	ASSERT_EQ((JDIMENSION)c_nQuadImageSize, cinfo.output_width);
	ASSERT_EQ((JDIMENSION)c_nQuadImageSize, cinfo.output_height);
	ASSERT_EQ(3, cinfo.output_components);

	std::vector<BYTE> arRow(3 * c_nQuadImageSize);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		const int nY = (int)cinfo.output_scanline;
		JSAMPROW pRow = arRow.data();
		jpeg_read_scanlines(&cinfo, &pRow, 1);
		if (4 != nY && 27 != nY)
			continue;
		for (int nX : {4, 27})
			ExpectNear(QuadrantColor(nX, nY), CColor{arRow[3 * nX], arRow[3 * nX + 1], arRow[3 * nX + 2]}, 6);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

TEST(CJpegTest, DecodesTruncatedImagePartially)
{
	// Like other viewers, show what is there: libjpeg pads the missing data.
	// Cut in the middle of the scan, after all the tables.
	std::vector<BYTE> arData = MakeQuadrantJpeg();
	size_t nScan = 0;
	for (size_t i = 0; i + 1 < arData.size(); ++i)
		if (0xFF == arData[i] && 0xDA == arData[i + 1])
			nScan = i;
	ASSERT_NE(0u, nScan);
	arData.resize(nScan + (arData.size() - nScan) / 2);

	CBgraFrame oFrame;
	ASSERT_TRUE(Decode(arData, oFrame));
	EXPECT_EQ(c_nQuadImageSize, oFrame.get_Width());
	ExpectNear(QuadrantColor(4, 4), Pixel(oFrame, 4, 4), 4);
}

TEST(CJpegTest, RejectsInvalidImages)
{
	CBgraFrame oFrame;
	std::vector<BYTE> arGarbage = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x02, 0x12, 0x34, 0x56, 0x78};
	EXPECT_FALSE(Decode(arGarbage, oFrame, false));

	std::vector<BYTE> arData = MakeQuadrantJpeg();
	arData.resize(64); // header only
	EXPECT_FALSE(Decode(arData, oFrame));
}
