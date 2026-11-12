/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for the FreeType based font engine in libgraphics (CApplicationFonts,
// CFontManager). The EOTest* fonts come from fonts/generate.py and have
// exactly known metrics; the Type 1 font is Nimbus Sans from PdfFile.

#include "gtest/gtest.h"

#include "../../graphics/pro/Fonts.h"
#include "../ApplicationFonts.h"
#include "../../common/File.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace
{
	std::wstring ToWString(const char* sUtf8)
	{
		return NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)sUtf8, (LONG)strlen(sUtf8));
	}

	std::wstring FontPath(const char* sName)
	{
		return ToWString(FONTENGINE_TEST_FONTS_DIR) + L"/" + ToWString(sName);
	}

	std::wstring Type1Path()
	{
		return ToWString(FONTENGINE_TEST_TYPE1_FONT);
	}

	template<typename T>
	struct CRelease
	{
		void operator()(T* p) const { if (p) p->Release(); }
	};
	template<typename T>
	using CPtr = std::unique_ptr<T, CRelease<T>>;
}

class CFontEngineTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		std::vector<std::wstring> arFiles = {
			FontPath("EOTestTrueType.ttf"),
			FontPath("EOTestCFF.otf"),
			Type1Path()
		};
		s_pApplication = NSFonts::NSApplication::Create();
		// flag 1: list OpenType (CFF) fonts too, not only TrueType
		s_pApplication->InitializeFromArrayFiles(arFiles, 1);
	}

	static void TearDownTestSuite()
	{
		RELEASEINTERFACE(s_pApplication);
	}

	static NSFonts::CFontInfo* FindInList(const std::wstring& sPath, NSFonts::IApplicationFonts* pApplication = s_pApplication)
	{
		std::vector<NSFonts::CFontInfo*>* pFonts = pApplication->GetList()->GetFonts();
		for (NSFonts::CFontInfo* pInfo : *pFonts)
		{
			if (pInfo->m_wsFontPath == sPath)
				return pInfo;
		}
		return NULL;
	}

	static CPtr<NSFonts::IFontManager> LoadFont(const std::wstring& sPath)
	{
		CPtr<NSFonts::IFontManager> pManager(s_pApplication->GenerateFontManager());
		// 100 pt at 72 dpi: one em is 100 px, so 1000 font units are 100 px
		if (!pManager->LoadFontFromFile(sPath, 0, 100, 72, 72))
			return nullptr;
		pManager->AfterLoad();
		return pManager;
	}

	static NSFonts::IApplicationFonts* s_pApplication;
};

NSFonts::IApplicationFonts* CFontEngineTest::s_pApplication = NULL;

// The font list reads localized family names straight from FreeType's
// internal sfnt name table (CFontList::Add)
TEST_F(CFontEngineTest, FontListReadsSfntFamilyNames)
{
	struct
	{
		const char* sFile;
		const wchar_t* sFamily;
		const wchar_t* sGermanFamily;
		NSFonts::EFontFormat eFormat;
	} arCases[] = {
		{"EOTestTrueType.ttf", L"EO Test TrueType", L"EO Testschrift TT", NSFonts::fontTrueType},
		{"EOTestCFF.otf", L"EO Test CFF", L"EO Testschrift CFF", NSFonts::fontOpenType},
	};

	for (const auto& oCase : arCases)
	{
		SCOPED_TRACE(oCase.sFile);
		NSFonts::CFontInfo* pInfo = FindInList(FontPath(oCase.sFile));
		ASSERT_NE(nullptr, pInfo);
		EXPECT_EQ(oCase.sFamily, pInfo->m_wsFontName);
		EXPECT_EQ(L"Regular", pInfo->m_wsStyle);
		EXPECT_EQ(oCase.eFormat, pInfo->m_eFontFormat);
		EXPECT_EQ(800, pInfo->m_shAscent);
		EXPECT_EQ(-200, pInfo->m_shDescent);
		EXPECT_NE(pInfo->names.end(), std::find(pInfo->names.begin(), pInfo->names.end(), std::wstring(oCase.sGermanFamily)));
	}
}

// WOFF2 needs the brotli decoder. Listed on its own: next to the .ttf it would
// be merged as the same family and style.
TEST_F(CFontEngineTest, FontListReadsWoff2Font)
{
	std::vector<std::wstring> arFiles = {FontPath("EOTestTrueType.woff2")};
	CPtr<NSFonts::IApplicationFonts> pApplication(NSFonts::NSApplication::Create());
	pApplication->InitializeFromArrayFiles(arFiles);

	NSFonts::CFontInfo* pInfo = FindInList(arFiles[0], pApplication.get());
	ASSERT_NE(nullptr, pInfo);
	EXPECT_EQ(L"EO Test TrueType", pInfo->m_wsFontName);
	EXPECT_EQ(NSFonts::fontTrueType, pInfo->m_eFontFormat);
	EXPECT_NE(pInfo->names.end(), std::find(pInfo->names.begin(), pInfo->names.end(), std::wstring(L"EO Testschrift TT")));
}

// Only sfnt fonts go into the font list; Type 1 fonts are loaded directly
TEST_F(CFontEngineTest, FontListSkipsType1Font)
{
	EXPECT_EQ(nullptr, FindInList(Type1Path()));
}

struct CSfntFontParam
{
	const char* sFile;
	const wchar_t* sFamily;
	const wchar_t* sType;
};

void PrintTo(const CSfntFontParam& oParam, std::ostream* pStream)
{
	*pStream << oParam.sFile;
}

class CSfntFontTest : public CFontEngineTest, public ::testing::WithParamInterface<CSfntFontParam> {};

TEST_P(CSfntFontTest, LoadsFontWithExactMetrics)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath(GetParam().sFile));
	ASSERT_NE(nullptr, pManager);

	EXPECT_EQ(GetParam().sType, pManager->GetFontType()); // FT_Get_X11_Font_Format
	EXPECT_EQ(GetParam().sFamily, pManager->GetName());
	EXPECT_EQ(1000, pManager->GetUnitsPerEm());
	EXPECT_EQ(800, pManager->GetAscender());
	EXPECT_EQ(-200, pManager->GetDescender());

	// head.yMin / yMax via the internal TT_Face
	double dMin = 0, dMax = 0;
	pManager->GetLimitsY(dMin, dMax);
	EXPECT_EQ(-100, dMin);
	EXPECT_EQ(750, dMax);

	// units per em and OS/2 win metrics via FT_Get_Sfnt_Table
	double dUnitsPerEm = 0, dAscent = 0, dDescent = 0;
	pManager->GetFace(dUnitsPerEm, dAscent, dDescent);
	EXPECT_EQ(1000, dUnitsPerEm);
	EXPECT_EQ(900, dAscent);
	EXPECT_EQ(-250, dDescent);
}

TEST_P(CSfntFontTest, MapsCharactersAndMeasuresAdvances)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath(GetParam().sFile));
	ASSERT_NE(nullptr, pManager);

	EXPECT_EQ(2u, pManager->GetGIDByUnicode('A'));
	EXPECT_EQ(3u, pManager->GetGIDByUnicode('B'));
	EXPECT_EQ(4u, pManager->GetGIDByUnicode(0x4E00));
	EXPECT_EQ(0u, pManager->GetGIDByUnicode('Z'));

	EXPECT_NEAR(25, pManager->MeasureChar2(' ').fAdvanceX, 0.5);
	EXPECT_NEAR(60, pManager->MeasureChar2('A').fAdvanceX, 0.5);
	EXPECT_NEAR(70, pManager->MeasureChar2('B').fAdvanceX, 0.5);
	EXPECT_NEAR(100, pManager->MeasureChar2(0x4E00).fAdvanceX, 0.5);
}

TEST_P(CSfntFontTest, RendersGlyphOutlines)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath(GetParam().sFile));
	ASSERT_NE(nullptr, pManager);

	// "A" is the square (100,0)-(500,700) in font units
	TBBoxAdvance oA = pManager->MeasureChar2('A');
	EXPECT_NEAR(10, oA.box.fMinX, 1);
	EXPECT_NEAR(50, oA.box.fMaxX, 1);
	EXPECT_NEAR(70, std::max(oA.box.fMaxY, oA.box.fMinY) - std::min(oA.box.fMaxY, oA.box.fMinY), 1);
}

INSTANTIATE_TEST_SUITE_P(Formats, CSfntFontTest, ::testing::Values(
	CSfntFontParam{"EOTestTrueType.ttf", L"EO Test TrueType", L"TrueType"},
	CSfntFontParam{"EOTestTrueType.woff2", L"EO Test TrueType", L"TrueType"}, // brotli
	CSfntFontParam{"EOTestCFF.otf", L"EO Test CFF", L"CFF"}),
	[](const ::testing::TestParamInfo<CSfntFontParam>& oInfo) {
		std::string sName = oInfo.param.sFile;
		std::replace(sName.begin(), sName.end(), '.', '_');
		return sName;
	});

TEST_F(CFontEngineTest, LoadsType1Font)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(Type1Path());
	ASSERT_NE(nullptr, pManager);

	EXPECT_EQ(L"Type 1", pManager->GetFontType());
	EXPECT_EQ(L"Nimbus Sans", pManager->GetName());
	EXPECT_EQ(1000, pManager->GetUnitsPerEm());
	EXPECT_NE(0u, pManager->GetGIDByUnicode('A'));
	// Helvetica-compatible metrics: "A" is 667 units wide
	EXPECT_NEAR(66.7, pManager->MeasureChar2('A').fAdvanceX, 1);
}

TEST_F(CFontEngineTest, FailsOnMissingOrInvalidFont)
{
	CPtr<NSFonts::IFontManager> pManager(s_pApplication->GenerateFontManager());
	EXPECT_FALSE(pManager->LoadFontFromFile(FontPath("does-not-exist.ttf"), 0, 100, 72, 72));
	EXPECT_FALSE(pManager->LoadFontFromFile(FontPath("generate.py"), 0, 100, 72, 72));
}

// A missing font with a metric twin is replaced by the twin, also under the names LibreOffice and
// fontconfig alias (PostScript, StarOffice, ChromeOS, URW): an old document in Times or Courier
// keeps serif or monospaced text with the same widths.
TEST(CFontListNamePicker, MetricAliasesFindTheirTwin)
{
	CFontListNamePicker oPicker;
	for (const wchar_t* sRequested : {L"Times New Roman", L"Times", L"Thorndale", L"Tinos", L"Nimbus Roman"})
		EXPECT_TRUE(oPicker.IsLikeFonts(L"Liberation Serif", sRequested)) << sRequested;
	for (const wchar_t* sRequested : {L"Courier New", L"Courier", L"Cumberland", L"Cousine", L"Nimbus Mono PS"})
		EXPECT_TRUE(oPicker.IsLikeFonts(L"Liberation Mono", sRequested)) << sRequested;
	for (const wchar_t* sRequested : {L"Arial", L"Helvetica", L"Albany", L"Arimo"})
		EXPECT_TRUE(oPicker.IsLikeFonts(L"Liberation Sans", sRequested)) << sRequested;

	// Twins only: no serif for a sans request, no other group, the existing groups unchanged.
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Sans", L"Times"));
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Serif", L"Courier"));
	EXPECT_TRUE(oPicker.IsLikeFonts(L"Caladea", L"Cambria"));
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Serif", L"Verdana"));
}
