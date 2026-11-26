/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Tests for the FreeType based font engine in libgraphics (CApplicationFonts,
// CFontManager). The EOTest* fonts come from fonts/generate.py and have
// exactly known metrics; the Type 1 font is Nimbus Sans from PdfFile.

#include "gtest/gtest.h"

#include "../../graphics/pro/Fonts.h"
#include "../ApplicationFontsWorker.h"
#include "../../common/Directory.h"
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

// COLRv1 color glyphs (CFontFile::GetColorGlyphRGBA). EOTestColrV1.ttf has
// full-em squares: U+E000 solid red, U+E001 linear red to blue left to right,
// U+E002 green with an inner square (250..750) in the text color.
class CColrV1Test : public CFontEngineTest
{
protected:
	struct CPixels
	{
		BYTE* pData = nullptr;
		int nLeft = 0, nTop = 0, nWidth = 0, nHeight = 0;
		~CPixels() { delete[] pData; }
		const BYTE* At(int x, int y) const { return pData + 4 * (y * nWidth + x); }
	};

	static bool Render(NSFonts::IFontManager* pManager, int nUnicode, unsigned int unForeground, CPixels& oPixels, double dPixelsPerEm = 100)
	{
		unsigned int unGID = pManager->GetGIDByUnicode(nUnicode);
		oPixels.pData = pManager->GetFile()->GetColorGlyphRGBA(unGID, dPixelsPerEm, unForeground, oPixels.nLeft, oPixels.nTop, oPixels.nWidth, oPixels.nHeight);
		return oPixels.pData != nullptr;
	}

	static void ExpectColor(const BYTE* p, int r, int g, int b, int a, int nTolerance = 2)
	{
		EXPECT_NEAR(r, p[0], nTolerance);
		EXPECT_NEAR(g, p[1], nTolerance);
		EXPECT_NEAR(b, p[2], nTolerance);
		EXPECT_NEAR(a, p[3], nTolerance);
	}
};

TEST_F(CColrV1Test, DetectsColorFont)
{
	CPtr<NSFonts::IFontManager> pColor = LoadFont(FontPath("EOTestColrV1.ttf"));
	CPtr<NSFonts::IFontManager> pPlain = LoadFont(FontPath("EOTestTrueType.ttf"));
	ASSERT_NE(nullptr, pColor);
	ASSERT_NE(nullptr, pPlain);
	EXPECT_TRUE(pColor->GetFile()->HasColorGlyphs());
	EXPECT_FALSE(pPlain->GetFile()->HasColorGlyphs());

	CPixels oPixels;
	EXPECT_FALSE(Render(pPlain.get(), 'A', 0x000000FF, oPixels));
}

TEST_F(CColrV1Test, RendersSolidFill)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);

	CPixels oPixels;
	ASSERT_TRUE(Render(pManager.get(), 0xE000, 0x000000FF, oPixels));
	// 1000 units at 100 px per em, from the baseline up
	EXPECT_EQ(0, oPixels.nLeft);
	EXPECT_EQ(100, oPixels.nTop);
	EXPECT_EQ(100, oPixels.nWidth);
	EXPECT_EQ(100, oPixels.nHeight);
	ExpectColor(oPixels.At(50, 50), 255, 0, 0, 255);
}

TEST_F(CColrV1Test, RendersLinearGradient)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);

	CPixels oPixels;
	ASSERT_TRUE(Render(pManager.get(), 0xE001, 0x000000FF, oPixels));
	ExpectColor(oPixels.At(0, 50), 252, 0, 3, 255, 4);
	ExpectColor(oPixels.At(99, 50), 3, 0, 252, 255, 4);
	ExpectColor(oPixels.At(50, 10), 127, 0, 127, 255, 4);
}

TEST_F(CColrV1Test, UsesTextColorForForegroundPaletteIndex)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);

	CPixels oPixels;
	ASSERT_TRUE(Render(pManager.get(), 0xE002, 0x3366CCFF, oPixels));
	ExpectColor(oPixels.At(10, 10), 0, 255, 0, 255);
	ExpectColor(oPixels.At(50, 50), 0x33, 0x66, 0xCC, 255);
}

// allfontsgen hands code points from U+1F000 to the color font, although the
// smaller text fixtures rank higher; U+2764 keeps its text presentation
TEST(CFontRangesTest, ColorFontWinsEmojiFallback)
{
	std::wstring sDir = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());
	ASSERT_FALSE(sDir.empty());

	CApplicationFontsWorker oWorker;
	oWorker.m_bIsUseSystemFonts = false;
	oWorker.m_bIsUseOpenType = true;
	oWorker.m_bIsNeedThumbnails = false;
	oWorker.m_arAdditionalFolders = { ToWString(FONTENGINE_TEST_FONTS_DIR) };
	oWorker.m_sDirectory = sDir;
	oWorker.m_sAllFontsJSPath = sDir + L"/AllFonts.js";

	NSFonts::IApplicationFonts* pFonts = oWorker.Check();
	ASSERT_NE(nullptr, pFonts);
	EXPECT_EQ(L"EO Test COLRv1", pFonts->GetFontBySymbol(0x1F600));
	EXPECT_NE(L"EO Test COLRv1", pFonts->GetFontBySymbol(0x2764));
	EXPECT_FALSE(pFonts->GetFontBySymbol(0x2764).empty());

	RELEASEINTERFACE(pFonts);
	NSDirectory::DeleteDirectory(sDir);
}

// Fonts come from documents, so hostile paint graphs must be refused (the
// caller falls back to the outline) instead of looping, burning CPU or memory.
// U+E003 references itself, U+E004 has 2^40 paths through shared children,
// U+E005 nests 20 composites that each hold two full-size layers.
TEST_F(CColrV1Test, RefusesCyclicPaintGraph)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);
	CPixels oPixels;
	EXPECT_FALSE(Render(pManager.get(), 0xE003, 0x000000FF, oPixels));
}

TEST_F(CColrV1Test, RefusesExponentialFanOut)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);
	CPixels oPixels;
	EXPECT_FALSE(Render(pManager.get(), 0xE004, 0x000000FF, oPixels));
}

TEST_F(CColrV1Test, RefusesNestedCompositesAboveMemoryLimit)
{
	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);
	CPixels oSmall, oLarge, oSolid;
	// a 100 px glyph fits, 1000 px needs 20 x 32 MB of layers
	EXPECT_TRUE(Render(pManager.get(), 0xE005, 0x000000FF, oSmall));
	EXPECT_FALSE(Render(pManager.get(), 0xE005, 0x000000FF, oLarge, 1000));
	EXPECT_TRUE(Render(pManager.get(), 0xE000, 0x000000FF, oSolid, 1000));
}

// Warped text (WordArt) needs outlines, and color glyphs have none of their
// own: the path is the silhouette of their layers, here the full-em square
TEST_F(CColrV1Test, GlyphPathIsSilhouetteOfLayers)
{
	struct CBounds : public NSFonts::ISimpleGraphicsPath
	{
		int nPoints = 0;
		double dMinX = 1e9, dMinY = 1e9, dMaxX = -1e9, dMaxY = -1e9;
		void Add(double x, double y)
		{
			++nPoints;
			dMinX = std::min(dMinX, x); dMaxX = std::max(dMaxX, x);
			dMinY = std::min(dMinY, y); dMaxY = std::max(dMaxY, y);
		}
		bool _MoveTo(double x, double y) override { Add(x, y); return true; }
		bool _LineTo(double x, double y) override { Add(x, y); return true; }
		bool _CurveTo(double x1, double y1, double x2, double y2, double x3, double y3) override { Add(x3, y3); return true; }
		bool _Close() override { return true; }
	};

	CPtr<NSFonts::IFontManager> pManager = LoadFont(FontPath("EOTestColrV1.ttf"));
	ASSERT_NE(nullptr, pManager);
	pManager->LoadString2(L"\uE000", 0, 0);
	CBounds oBounds;
	pManager->GetStringPath(&oBounds);
	ASSERT_GT(oBounds.nPoints, 0);
	EXPECT_NEAR(oBounds.dMaxX - oBounds.dMinX, oBounds.dMaxY - oBounds.dMinY, 1.0);
	EXPECT_GT(oBounds.dMaxX - oBounds.dMinX, 0.0);
}

// A missing font with a metric twin is replaced by the twin, also under the names LibreOffice and
// fontconfig alias (PostScript, StarOffice, ChromeOS, URW): an old document in Times or Courier
// keeps serif or monospaced text with the same widths.
TEST(CFontListNamePicker, MetricAliasesFindTheirTwin)
{
	CFontListNamePicker oPicker;
	for (const wchar_t* sRequested : {L"Times New Roman", L"Times", L"Thorndale", L"Thorndale AMT", L"Tinos", L"Nimbus Roman"})
		EXPECT_TRUE(oPicker.IsLikeFonts(L"Liberation Serif", sRequested)) << sRequested;
	for (const wchar_t* sRequested : {L"Courier New", L"Courier", L"Cumberland", L"Cumberland AMT", L"Cousine", L"Nimbus Mono PS", L"Nimbus Mono"})
		EXPECT_TRUE(oPicker.IsLikeFonts(L"Liberation Mono", sRequested)) << sRequested;
	for (const wchar_t* sRequested : {L"Arial", L"Helvetica", L"Albany", L"Albany AMT", L"Arimo"})
		EXPECT_TRUE(oPicker.IsLikeFonts(L"Liberation Sans", sRequested)) << sRequested;

	// The other clones are offered as twins too, where Liberation is not installed.
	for (const wchar_t* sCandidate : {L"Tinos", L"Nimbus Roman", L"Nimbus Roman No9 L"})
		EXPECT_TRUE(oPicker.IsLikeFonts(sCandidate, L"Times New Roman")) << sCandidate;
	for (const wchar_t* sCandidate : {L"Cousine", L"Nimbus Mono PS", L"Nimbus Mono", L"Nimbus Mono L"})
		EXPECT_TRUE(oPicker.IsLikeFonts(sCandidate, L"Courier New")) << sCandidate;
	for (const wchar_t* sCandidate : {L"Arimo", L"Nimbus Sans"})
		EXPECT_TRUE(oPicker.IsLikeFonts(sCandidate, L"Arial")) << sCandidate;

	// Twins only: no serif for a sans request, no other group, the existing groups unchanged.
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Sans", L"Times"));
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Serif", L"Courier"));
	EXPECT_TRUE(oPicker.IsLikeFonts(L"Caladea", L"Cambria"));
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Serif", L"Verdana"));

	// A font whose name only contains a twin's name is not in the groups: it keeps the
	// substitute it got before.
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Liberation Mono", L"Courier 10 Pitch"));
	EXPECT_FALSE(oPicker.IsLikeFonts(L"Courier New", L"Courier 10 Pitch"));
	EXPECT_EQ(10000, oPicker.CheckEqualsFonts(L"Liberation Mono", L"Courier 10 Pitch"));
}
