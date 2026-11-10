/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// CDjVuFile on DjVuLibre's libdjvu: page sizes, rendering of bilevel (JB2) and
// color (IW44) pages, bookmarks, links and the text layer, and damaged input.
// The fixtures in data/ come from data/generate.py, which describes the pages.

#include "gtest/gtest.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../DjVu.h"
#include "../../DesktopEditor/common/File.h"
#include "../../DesktopEditor/graphics/pro/Fonts.h"
#include "../../PdfFile/PdfFile.h"

namespace
{
	std::wstring Fixture(const char* sName)
	{
		std::string sPath = std::string(DJVU_TEST_DATA_DIR) + "/" + sName;
		return std::wstring(sPath.begin(), sPath.end());
	}

	std::vector<BYTE> ReadFixture(const char* sName)
	{
		BYTE* pData = NULL;
		DWORD nLength = 0;
		std::vector<BYTE> arrData;
		if (NSFile::CFileBinary::ReadAllBytes(Fixture(sName), &pData, nLength))
			arrData.assign(pData, pData + nLength);
		RELEASEARRAYOBJECTS(pData);
		return arrData;
	}

	// Reads the buffers of GetStructure and GetLinks (NSWasm::CData): a total
	// length, then 32 bit integers (doubles are stored times 100) and strings
	// with a 32 bit length
	class CReader
	{
	public:
		explicit CReader(BYTE* pData) : m_pData(pData), m_nPos(4)
		{
			memcpy(&m_nLength, pData, 4);
		}
		bool End() const { return m_nPos >= m_nLength; }
		int Int()
		{
			int nValue = 0;
			memcpy(&nValue, m_pData + m_nPos, 4);
			m_nPos += 4;
			return nValue;
		}
		std::string String()
		{
			unsigned int nLen = (unsigned int)Int();
			std::string sValue((const char*)(m_pData + m_nPos), nLen);
			m_nPos += nLen;
			return sValue;
		}

	private:
		BYTE* m_pData;
		unsigned int m_nLength;
		unsigned int m_nPos;
	};

	// Records the words of the text layer that DrawPageOnRenderer writes
	// into a PDF page
	class CTextRecorder : public CPdfFile
	{
	public:
		explicit CTextRecorder(NSFonts::IApplicationFonts* pFonts) : CPdfFile(pFonts) {}

		virtual HRESULT CommandDrawText(const std::wstring& wsText, const double& dX, const double& dY, const double& dW, const double& dH) override
		{
			Words.push_back(wsText);
			return CPdfFile::CommandDrawText(wsText, dX, dY, dW, dH);
		}

		std::vector<std::wstring> Words;
	};

	class DjVuFileTest : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			s_pFonts = NSFonts::NSApplication::Create();
			s_pFonts->Initialize();
		}
		static void TearDownTestSuite()
		{
			RELEASEINTERFACE(s_pFonts);
		}

		static NSFonts::IApplicationFonts* s_pFonts;
	};
	NSFonts::IApplicationFonts* DjVuFileTest::s_pFonts = NULL;

	// The bilevel page, true is black, y from the top
	bool BilevelBlack(int x, int y)
	{
		bool bRect = x >= 20 && x < 120 && y >= 10 && y < 100;
		bool bHole = x >= 50 && x < 90 && y >= 30 && y < 70;
		return bRect && !bHole;
	}

	// Pixels of ConvertToPixels are R, G, B, A bytes; rows are top-down when
	// bIsFlip is true
	struct CPixel
	{
		int R, G, B;
	};
	CPixel GetPixel(const BYTE* pPixels, int nWidth, int x, int y)
	{
		const BYTE* p = pPixels + 4 * (y * nWidth + x);
		return { p[0], p[1], p[2] };
	}
}

TEST_F(DjVuFileTest, ReadsPagesOfABundledDocument)
{
	CDjVuFile oFile(s_pFonts);
	ASSERT_TRUE(oFile.LoadFromFile(Fixture("document.djvu")));
	EXPECT_EQ(oFile.GetType(), odftDJVU);
	ASSERT_EQ(oFile.GetPagesCount(), 2);

	double dWidth = 0, dHeight = 0, dDpiX = 0, dDpiY = 0;
	oFile.GetPageInfo(0, &dWidth, &dHeight, &dDpiX, &dDpiY);
	EXPECT_EQ(dWidth, 240);
	EXPECT_EQ(dHeight, 120);
	EXPECT_EQ(dDpiX, 300);
	EXPECT_EQ(dDpiY, 300);

	oFile.GetPageInfo(1, &dWidth, &dHeight, &dDpiX, &dDpiY);
	EXPECT_EQ(dWidth, 128);
	EXPECT_EQ(dHeight, 96);
	EXPECT_EQ(dDpiX, 100);

	EXPECT_EQ(oFile.GetInfo(), L"{\"PageWidth\":24000,\"PageHeight\":12000,\"NumberOfPages\":2}");
}

TEST_F(DjVuFileTest, RendersABilevelPageExactly)
{
	std::vector<BYTE> arrData = ReadFixture("bilevel.djvu");
	ASSERT_FALSE(arrData.empty());

	CDjVuFile oFile(s_pFonts);
	ASSERT_TRUE(oFile.LoadFromMemory(arrData.data(), (DWORD)arrData.size()));
	ASSERT_EQ(oFile.GetPagesCount(), 1);

	BYTE* pPixels = oFile.ConvertToPixels(0, 240, 120, true);
	ASSERT_NE(pPixels, nullptr);

	int nWrong = 0;
	for (int y = 0; y < 120; ++y)
	{
		for (int x = 0; x < 240; ++x)
		{
			CPixel oPixel = GetPixel(pPixels, 240, x, y);
			int nExpected = BilevelBlack(x, y) ? 0 : 255;
			if (oPixel.R != nExpected || oPixel.G != nExpected || oPixel.B != nExpected)
				++nWrong;
		}
	}
	EXPECT_EQ(nWrong, 0);
	delete[] pPixels;
}

TEST_F(DjVuFileTest, RendersAColorPage)
{
	CDjVuFile oFile(s_pFonts);
	ASSERT_TRUE(oFile.LoadFromFile(Fixture("color.djvu")));

	BYTE* pPixels = oFile.ConvertToPixels(0, 128, 96, true);
	ASSERT_NE(pPixels, nullptr);

	// IW44 is lossy: compare the middle of each quadrant with a tolerance
	struct CCase { int x, y; CPixel Expected; };
	const CCase arrCases[] = {
		{ 32, 24, { 255, 0, 0 } },
		{ 96, 24, { 0, 255, 0 } },
		{ 32, 72, { 0, 0, 255 } },
		{ 96, 72, { 255, 255, 255 } },
	};
	for (const CCase& oCase : arrCases)
	{
		CPixel oPixel = GetPixel(pPixels, 128, oCase.x, oCase.y);
		EXPECT_NEAR(oPixel.R, oCase.Expected.R, 24) << "at " << oCase.x << "," << oCase.y;
		EXPECT_NEAR(oPixel.G, oCase.Expected.G, 24) << "at " << oCase.x << "," << oCase.y;
		EXPECT_NEAR(oPixel.B, oCase.Expected.B, 24) << "at " << oCase.x << "," << oCase.y;
	}
	delete[] pPixels;
}

TEST_F(DjVuFileTest, ReadsBookmarks)
{
	CDjVuFile oFile(s_pFonts);
	ASSERT_TRUE(oFile.LoadFromFile(Fixture("document.djvu")));

	BYTE* pStructure = oFile.GetStructure();
	ASSERT_NE(pStructure, nullptr);

	// Page index, level, y (always 0), title
	struct CBookmark { int Page; int Level; std::string Title; };
	std::vector<CBookmark> arrBookmarks;
	CReader oReader(pStructure);
	while (!oReader.End())
	{
		CBookmark oBookmark;
		oBookmark.Page = oReader.Int();
		oBookmark.Level = oReader.Int();
		EXPECT_EQ(oReader.Int(), 0);
		oBookmark.Title = oReader.String();
		arrBookmarks.push_back(oBookmark);
	}
	delete[] pStructure;

	ASSERT_EQ(arrBookmarks.size(), 3u);
	EXPECT_EQ(arrBookmarks[0].Title, "Black page");
	EXPECT_EQ(arrBookmarks[0].Page, 0);
	EXPECT_EQ(arrBookmarks[0].Level, 1);
	EXPECT_EQ(arrBookmarks[1].Title, "Inner");
	EXPECT_EQ(arrBookmarks[1].Page, 0);
	EXPECT_EQ(arrBookmarks[1].Level, 2);
	EXPECT_EQ(arrBookmarks[2].Title, "Color page");
	EXPECT_EQ(arrBookmarks[2].Page, 1);
	EXPECT_EQ(arrBookmarks[2].Level, 1);
}

TEST_F(DjVuFileTest, ReadsLinks)
{
	CDjVuFile oFile(s_pFonts);
	ASSERT_TRUE(oFile.LoadFromFile(Fixture("document.djvu")));

	BYTE* pLinks = oFile.GetLinks(0);
	ASSERT_NE(pLinks, nullptr);

	CReader oReader(pLinks);
	ASSERT_FALSE(oReader.End());
	EXPECT_EQ(oReader.String(), "https://example.com/");
	EXPECT_EQ(oReader.Int(), 0);
	// Pixels from the top left, times 100
	EXPECT_EQ(oReader.Int(), 2000);  // x
	EXPECT_EQ(oReader.Int(), 1000);  // y
	EXPECT_EQ(oReader.Int(), 10000); // width
	EXPECT_EQ(oReader.Int(), 9000);  // height
	EXPECT_TRUE(oReader.End());
	delete[] pLinks;

	// The color page has no annotations
	BYTE* pNoLinks = oFile.GetLinks(1);
	if (pNoLinks)
	{
		EXPECT_TRUE(CReader(pNoLinks).End());
		delete[] pNoLinks;
	}
}

TEST_F(DjVuFileTest, WritesTheTextLayerToPdf)
{
	CDjVuFile oFile(s_pFonts);
	ASSERT_TRUE(oFile.LoadFromFile(Fixture("document.djvu")));

	CTextRecorder oPdf(s_pFonts);
	oPdf.CreatePdf();
	oPdf.NewPage();
	oPdf.put_Width(240 * 25.4 / 300);
	oPdf.put_Height(120 * 25.4 / 300);
	bool bBreak = false;
	oFile.DrawPageOnRenderer(&oPdf, 0, &bBreak);

	ASSERT_EQ(oPdf.Words.size(), 2u);
	EXPECT_EQ(oPdf.Words[0], L"Hello");
	EXPECT_EQ(oPdf.Words[1], L"DjVu");
}

TEST_F(DjVuFileTest, SurvivesDamagedInput)
{
	std::vector<BYTE> arrDocument = ReadFixture("document.djvu");
	ASSERT_FALSE(arrDocument.empty());

	// Cut off at many points, and with bytes overwritten. Loading may fail or
	// pages may come out blank, but nothing may crash.
	std::vector<std::vector<BYTE>> arrInputs;
	for (size_t nLength = 0; nLength < arrDocument.size(); nLength += 37)
		arrInputs.emplace_back(arrDocument.begin(), arrDocument.begin() + nLength);
	srand(1);
	for (int nCase = 0; nCase < 64; ++nCase)
	{
		std::vector<BYTE> arrDamaged = arrDocument;
		for (int i = 0; i < 8; ++i)
			arrDamaged[16 + rand() % (arrDamaged.size() - 16)] = (BYTE)rand();
		arrInputs.push_back(arrDamaged);
	}

	for (std::vector<BYTE>& arrInput : arrInputs)
	{
		CDjVuFile oFile(s_pFonts);
		if (!oFile.LoadFromMemory(arrInput.data(), (DWORD)arrInput.size()))
			continue;
		for (int nPage = 0, nPages = oFile.GetPagesCount(); nPage < nPages && nPage < 4; ++nPage)
		{
			double dWidth = 0, dHeight = 0, dDpiX = 0, dDpiY = 0;
			oFile.GetPageInfo(nPage, &dWidth, &dHeight, &dDpiX, &dDpiY);
			if (dWidth > 0 && dHeight > 0 && dWidth <= 1000 && dHeight <= 1000)
				delete[] oFile.ConvertToPixels(nPage, (int)dWidth, (int)dHeight);
			delete[] oFile.GetLinks(nPage);
		}
		delete[] oFile.GetStructure();
	}
}
