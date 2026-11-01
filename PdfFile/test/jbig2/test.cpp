/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// The PDF writer stores 1 bit images (DjVu pages and masks) as JBIG2 through
// jbig2enc and Leptonica; CBgraFrame::SaveFile writes JBIG2 through the same
// libraries. Each case decodes the result again with xpdf's JBIG2 decoder by
// rendering the PDF page 1:1, and compares it with the source pixels.
//
// The writer uses jbig2enc's symbol mode without refinement: every copy of a
// symbol is drawn from one template, placed by its centroid, so a copy may sit
// up to two pixels off. Those pages are compared with that tolerance.

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include "../../PdfFile.h"
#include "../../../DesktopEditor/common/Directory.h"
#include "../../../DesktopEditor/common/File.h"
#include "../../../DesktopEditor/graphics/Image.h"
#include "../../../DesktopEditor/graphics/pro/Fonts.h"
#include "../../../DesktopEditor/graphics/pro/Image.h"
#include "../../../DesktopEditor/raster/BgraFrame.h"

namespace
{
	// A 1 bit image, true is black
	struct CBitmap
	{
		int W;
		int H;
		std::vector<bool> Bits;

		CBitmap(int w, int h) : W(w), H(h), Bits(w * h, false) {}

		bool Get(int x, int y) const { return Bits[y * W + x]; }
		void Set(int x, int y) { if (x >= 0 && y >= 0 && x < W && y < H) Bits[y * W + x] = true; }
		void Rect(int x, int y, int w, int h)
		{
			for (int j = 0; j < h; ++j)
				for (int i = 0; i < w; ++i)
					Set(x + i, y + j);
		}
	};

	// Shapes of clearly different sizes: the PDF writer encodes in symbol mode with
	// a 0.85 match threshold, which may merge similar but different symbols. Exact
	// copies are stored once and decode exactly.
	void DrawShape(CBitmap& bmp, int nShape, int x, int y)
	{
		switch (nShape % 5)
		{
		case 0: // square
			bmp.Rect(x, y, 4, 4);
			break;
		case 1: // ring
			bmp.Rect(x, y, 13, 2); bmp.Rect(x, y + 11, 13, 2);
			bmp.Rect(x, y, 2, 13); bmp.Rect(x + 11, y, 2, 13);
			break;
		case 2: // vertical bar
			bmp.Rect(x, y, 6, 17);
			break;
		case 3: // cross
			bmp.Rect(x, y + 4, 11, 3); bmp.Rect(x + 4, y, 3, 11);
			break;
		default: // diagonal
			for (int i = 0; i < 9; ++i)
				bmp.Rect(x + 2 * i, y + i, 2, 1);
			break;
		}
	}

	// A text like page: rows of shapes, a different pattern per seed, plus one large
	// component that the encoder does not treat as a symbol
	CBitmap MakePage(int w, int h, int nSeed)
	{
		CBitmap bmp(w, h);
		unsigned int nState = 12345 + nSeed * 7919;
		for (int y = 4; y + 20 < h - 40; y += 24)
		{
			for (int x = 4; x + 20 < w; x += 24)
			{
				nState = nState * 1103515245 + 12345;
				int nPick = (nState >> 16) % 7;
				if (nPick < 5)
					DrawShape(bmp, nPick, x, y);
			}
		}
		bmp.Rect(10, h - 34, w / 2, 26);
		// with a hole
		for (int j = 0; j < 6; ++j)
			for (int i = 0; i < w / 2 - 8; ++i)
				bmp.Bits[(h - 30 + j) * w + 14 + i] = false;
		return bmp;
	}

	void ToPix(const CBitmap& bmp, NSImages::CPixJbig2& oPix)
	{
		ASSERT_TRUE(oPix.Create(bmp.W, bmp.H, 1));
		for (int y = 0; y < bmp.H; ++y)
			for (int x = 0; x < bmp.W; ++x)
				oPix.SetPixel(x, y, bmp.Get(x, y) ? 1 : 0);
	}

	// Page size in mm that renders 1:1 at w x h pixels
	double PxToMM(int px) { return px * 25.4 / 72.0; }
}

class CPdfJbig2Test : public testing::Test
{
protected:
	static NSFonts::IApplicationFonts* m_pFonts;
	static std::wstring m_wsDir;

	static void SetUpTestSuite()
	{
		// Image only pages need no fonts
		m_pFonts = NSFonts::NSApplication::Create();
		std::vector<std::wstring> arFiles;
		m_pFonts->InitializeFromArrayFiles(arFiles);

		m_wsDir = NSFile::GetProcessDirectory() + L"/pdf_jbig2_out";
		if (NSDirectory::Exists(m_wsDir))
			NSDirectory::DeleteDirectory(m_wsDir);
		NSDirectory::CreateDirectory(m_wsDir);
	}
	static void TearDownTestSuite()
	{
		RELEASEINTERFACE(m_pFonts);
	}

	std::wstring Path(const std::wstring& wsName) { return m_wsDir + L"/" + wsName; }

	// Renders page nPage of a PDF at w x h, top-down RGBA
	std::vector<BYTE> Render(const std::wstring& wsFile, int nPage, int w, int h)
	{
		CPdfFile oPdf(m_pFonts);
		EXPECT_TRUE(oPdf.LoadFromFile(wsFile));
		BYTE* pPixels = oPdf.ConvertToPixels(nPage, w, h, true);
		EXPECT_NE(pPixels, nullptr);
		if (!pPixels)
			return std::vector<BYTE>();
		std::vector<BYTE> arRes(pPixels, pPixels + w * h * 4);
		delete[] pPixels;
		return arRes;
	}

	// The pixels of a render where bIsColor() holds
	template<typename F>
	CBitmap Classify(const std::vector<BYTE>& arRgba, int w, int h, F bIsColor)
	{
		CBitmap bmp(w, h);
		if (arRgba.size() != (size_t)w * h * 4)
			return bmp;
		for (int y = 0; y < h; ++y)
			for (int x = 0; x < w; ++x)
				if (bIsColor(&arRgba[(y * w + x) * 4]))
					bmp.Set(x, y);
		return bmp;
	}
	CBitmap Black(const std::vector<BYTE>& arRgba, int w, int h)
	{
		return Classify(arRgba, w, h, [](const BYTE* p) { return (p[0] + p[1] + p[2]) < 3 * 128; });
	}

	int CountMismatches(const CBitmap& oExpected, const CBitmap& oActual)
	{
		int nMismatches = 0;
		for (size_t i = 0; i < oExpected.Bits.size(); ++i)
			nMismatches += (oExpected.Bits[i] != oActual.Bits[i]) ? 1 : 0;
		return nMismatches;
	}

	// Black pixels of a without a black pixel of b at most two pixels away
	int CountUnmatched(const CBitmap& a, const CBitmap& b)
	{
		int nUnmatched = 0;
		for (int y = 0; y < a.H; ++y)
		{
			for (int x = 0; x < a.W; ++x)
			{
				if (!a.Get(x, y))
					continue;
				bool bFound = false;
				for (int dy = -2; dy <= 2 && !bFound; ++dy)
					for (int dx = -2; dx <= 2 && !bFound; ++dx)
						bFound = x + dx >= 0 && y + dy >= 0 && x + dx < b.W && y + dy < b.H && b.Get(x + dx, y + dy);
				nUnmatched += bFound ? 0 : 1;
			}
		}
		return nUnmatched;
	}

	// Symbol mode: the same shapes, each at most two pixels off
	void ExpectCloseTo(const CBitmap& oExpected, const CBitmap& oActual)
	{
		EXPECT_EQ(0, CountUnmatched(oExpected, oActual)) << "source pixels missing";
		EXPECT_EQ(0, CountUnmatched(oActual, oExpected)) << "extra pixels";
		// A shifted thin shape differs in about twice its area
		EXPECT_LT(CountMismatches(oExpected, oActual), CountBlackPixels(oExpected) / 5);
	}

	int CountBlackPixels(const CBitmap& bmp)
	{
		int nCount = 0;
		for (bool b : bmp.Bits)
			nCount += b ? 1 : 0;
		return nCount;
	}
};

NSFonts::IApplicationFonts* CPdfJbig2Test::m_pFonts = NULL;
std::wstring CPdfJbig2Test::m_wsDir;

TEST_F(CPdfJbig2Test, DrawImage1bppRoundTrip)
{
	const int W = 300, H = 200;
	CBitmap bmp = MakePage(W, H, 0);
	ASSERT_GT(CountBlackPixels(bmp), 1000);

	{
		CPdfFile oPdf(m_pFonts);
		oPdf.CreatePdf();
		oPdf.NewPage();
		oPdf.put_Width(PxToMM(W));
		oPdf.put_Height(PxToMM(H));

		NSImages::CPixJbig2 oPix;
		ToPix(bmp, oPix);
		EXPECT_EQ(S_OK, oPdf.DrawImage1bpp(&oPix, W, H, 0, 0, PxToMM(W), PxToMM(H)));
		oPix.Destroy();
		ASSERT_EQ(0, oPdf.SaveToFile(Path(L"single.pdf")));
	}

	ExpectCloseTo(bmp, Black(Render(Path(L"single.pdf"), 0, W, H), W, H));
}

// The writer shares one JBIG2Globals symbol dictionary between up to five images,
// then starts a new one: seven pages use two dictionaries
TEST_F(CPdfJbig2Test, DrawImage1bppManyPages)
{
	const int W = 240, H = 160, nPages = 7;
	std::vector<CBitmap> arPages;
	for (int i = 0; i < nPages; ++i)
		arPages.push_back(MakePage(W, H, i + 1));

	{
		CPdfFile oPdf(m_pFonts);
		oPdf.CreatePdf();
		for (int i = 0; i < nPages; ++i)
		{
			oPdf.NewPage();
			oPdf.put_Width(PxToMM(W));
			oPdf.put_Height(PxToMM(H));

			NSImages::CPixJbig2 oPix;
			ToPix(arPages[i], oPix);
			EXPECT_EQ(S_OK, oPdf.DrawImage1bpp(&oPix, W, H, 0, 0, PxToMM(W), PxToMM(H)));
			oPix.Destroy();
		}
		ASSERT_EQ(0, oPdf.SaveToFile(Path(L"pages.pdf")));
	}

	for (int i = 0; i < nPages; ++i)
	{
		SCOPED_TRACE("page " + std::to_string(i));
		ExpectCloseTo(arPages[i], Black(Render(Path(L"pages.pdf"), i, W, H), W, H));
	}
}

// DjVu color pages: the foreground is a color image with a JBIG2 mask
TEST_F(CPdfJbig2Test, DrawImageWith1bppMask)
{
	const int W = 200, H = 140;
	CBitmap bmp = MakePage(W, H, 42);

	// Solid blue, so that the base image's JPEG encoding stays exact
	std::vector<BYTE> arBlue(W * H * 4);
	for (int i = 0; i < W * H; ++i)
	{
		arBlue[i * 4 + 0] = 255;
		arBlue[i * 4 + 1] = 0;
		arBlue[i * 4 + 2] = 0;
		arBlue[i * 4 + 3] = 255;
	}
	Aggplus::CImage oImage;
	oImage.Create(arBlue.data(), W, H, 4 * W, true);

	{
		CPdfFile oPdf(m_pFonts);
		oPdf.CreatePdf();
		oPdf.NewPage();
		oPdf.put_Width(PxToMM(W));
		oPdf.put_Height(PxToMM(H));

		NSImages::CPixJbig2 oPix;
		ToPix(bmp, oPix);
		EXPECT_EQ(S_OK, oPdf.DrawImageWith1bppMask((IGrObject*)&oImage, &oPix, W, H, 0, 0, PxToMM(W), PxToMM(H)));
		oPix.Destroy();
		ASSERT_EQ(0, oPdf.SaveToFile(Path(L"mask.pdf")));
	}

	std::vector<BYTE> arRgba = Render(Path(L"mask.pdf"), 0, W, H);
	ASSERT_EQ(arRgba.size(), (size_t)W * H * 4);

	// Black mask bits show the blue image, the others the white page
	CBitmap oBlue = Classify(arRgba, W, H, [](const BYTE* p) { return p[0] < 128 && p[1] < 128 && p[2] > 128; });
	CBitmap oWhite = Classify(arRgba, W, H, [](const BYTE* p) { return p[0] > 128 && p[1] > 128 && p[2] > 128; });
	EXPECT_EQ(W * H, CountBlackPixels(oBlue) + CountBlackPixels(oWhite));
	ExpectCloseTo(bmp, oBlue);
}

// CBgraFrame::SaveFile with type 21 writes a JBIG2 generic region without file
// header (CJBig2File in PDF mode). Wrap it into a minimal PDF to decode it.
TEST_F(CPdfJbig2Test, BgraFrameSaveJbig2)
{
	const int W = 160, H = 96;
	CBitmap bmp = MakePage(W, H, 7);

	BYTE* pData = new BYTE[W * H * 4];
	for (int i = 0; i < W * H; ++i)
	{
		BYTE v = bmp.Bits[i] ? 0 : 255;
		pData[i * 4 + 0] = v;
		pData[i * 4 + 1] = v;
		pData[i * 4 + 2] = v;
		pData[i * 4 + 3] = 255;
	}
	// A blue pixel: gray 29 is below the 188 threshold, so black
	pData[0] = 255; pData[1] = 0; pData[2] = 0;
	bmp.Set(0, 0);

	CBgraFrame oFrame;
	oFrame.put_Data(pData);
	oFrame.put_Width(W);
	oFrame.put_Height(H);
	oFrame.put_Stride(4 * W);
	ASSERT_TRUE(oFrame.SaveFile(Path(L"frame.jb2"), 21));

	BYTE* pJbig2 = NULL;
	DWORD nJbig2 = 0;
	ASSERT_TRUE(NSFile::CFileBinary::ReadAllBytes(Path(L"frame.jb2"), &pJbig2, nJbig2));
	ASSERT_GT(nJbig2, 20u);
	std::string sJbig2((const char*)pJbig2, nJbig2);
	RELEASEARRAYOBJECTS(pJbig2);

	std::string sContent = "q " + std::to_string(W) + " 0 0 " + std::to_string(H) + " 0 0 cm /Im0 Do Q\n";
	std::vector<std::string> arObjects = {
		"<< /Type /Catalog /Pages 2 0 R >>",
		"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
		"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 " + std::to_string(W) + " " + std::to_string(H) + "]"
			" /Resources << /XObject << /Im0 4 0 R >> >> /Contents 5 0 R >>",
		"<< /Type /XObject /Subtype /Image /Width " + std::to_string(W) + " /Height " + std::to_string(H) +
			" /ColorSpace /DeviceGray /BitsPerComponent 1 /Filter /JBIG2Decode /Length " + std::to_string(sJbig2.size()) +
			" >>\nstream\n" + sJbig2 + "\nendstream",
		"<< /Length " + std::to_string(sContent.size()) + " >>\nstream\n" + sContent + "endstream",
	};
	std::string sPdf = "%PDF-1.4\n";
	std::vector<size_t> arOffsets;
	for (size_t i = 0; i < arObjects.size(); ++i)
	{
		arOffsets.push_back(sPdf.size());
		sPdf += std::to_string(i + 1) + " 0 obj\n" + arObjects[i] + "\nendobj\n";
	}
	size_t nXref = sPdf.size();
	sPdf += "xref\n0 " + std::to_string(arObjects.size() + 1) + "\n0000000000 65535 f \n";
	for (size_t nOffset : arOffsets)
	{
		char sEntry[24];
		snprintf(sEntry, sizeof(sEntry), "%010zu 00000 n \n", nOffset);
		sPdf += sEntry;
	}
	sPdf += "trailer\n<< /Size " + std::to_string(arObjects.size() + 1) + " /Root 1 0 R >>\nstartxref\n" + std::to_string(nXref) + "\n%%EOF\n";

	NSFile::CFileBinary oFile;
	ASSERT_TRUE(oFile.CreateFileW(Path(L"frame.pdf")));
	oFile.WriteFile((const BYTE*)sPdf.data(), (DWORD)sPdf.size());
	oFile.CloseFile();

	// A generic region is lossless
	EXPECT_EQ(0, CountMismatches(bmp, Black(Render(Path(L"frame.pdf"), 0, W, H), W, H)));
}
