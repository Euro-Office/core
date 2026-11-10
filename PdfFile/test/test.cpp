/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// CPdfFile: writing, reading, editing, splitting, merging, passwords,
// metadata and signatures. Every test builds the PDFs it needs with CPdfFile's
// own writer, so there are no fixtures. A page is recognised by its size and
// by the colour of a rectangle covering it, read back by rendering the page.

#include "gtest/gtest.h"
#include "../../DesktopEditor/common/File.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/graphics/pro/Fonts.h"
#include "../../DesktopEditor/xmlsec/src/include/CertificateCommon.h"
#include "../PdfFile.h"
#include "../PdfWriter.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <vector>

namespace
{
	struct PageSpec
	{
		double dWidthMM;
		double dHeightMM;
		LONG lColor; // renderer colour, 0xBBGGRR
	};

	// Distinct colours, far apart in every channel combination
	const LONG c_lRed   = 0x0000FF;
	const LONG c_lGreen = 0x00FF00;
	const LONG c_lBlue  = 0xFF0000;
	const LONG c_lBlack = 0x000000;

	const PageSpec c_oA4       = { 210.0, 297.0, c_lRed };
	const PageSpec c_oLandscape = { 297.0, 210.0, c_lGreen };
	const PageSpec c_oLetter   = { 215.9, 279.4, c_lBlue };

	// VerifySign() result bits (ICertificate::VerifyPKCS7)
	const int c_nSignIntact   = 1 << 0; // the signed bytes are unchanged
	const int c_nSignVerified = 1 << 1; // and the certificate verifies

	// Reads 4-byte little-endian ints from the length-prefixed buffers
	// CPdfFile returns.
	int ReadInt(const BYTE* p)
	{
		return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
	}
}

class CPdfFileTest : public testing::Test
{
protected:
	static NSFonts::IApplicationFonts* m_pFonts;
	std::wstring m_wsDir;

	static void SetUpTestSuite()
	{
		// The pages hold only paths: no system fonts needed
		m_pFonts = NSFonts::NSApplication::Create();
		std::vector<std::wstring> arFiles;
		m_pFonts->InitializeFromArrayFiles(arFiles);
	}
	static void TearDownTestSuite()
	{
		RELEASEINTERFACE(m_pFonts);
	}

	void SetUp() override
	{
		const testing::TestInfo* pInfo = testing::UnitTest::GetInstance()->current_test_info();
		m_wsDir = NSFile::GetProcessDirectory() + L"/pdffile_test_out/" + UTF8_TO_U(std::string(pInfo->name()));
		if (NSDirectory::Exists(m_wsDir))
			NSDirectory::DeleteDirectory(m_wsDir);
		NSDirectory::CreateDirectories(m_wsDir);
	}
	void TearDown() override
	{
		if (!HasFailure())
			NSDirectory::DeleteDirectory(m_wsDir);
	}

	std::wstring Path(const std::wstring& wsName) { return m_wsDir + L"/" + wsName; }

	std::unique_ptr<CPdfFile> NewPdf()
	{
		std::unique_ptr<CPdfFile> pPdf(new CPdfFile(m_pFonts));
		std::wstring wsTemp = Path(L"temp");
		NSDirectory::CreateDirectory(wsTemp);
		pPdf->SetTempDirectory(wsTemp);
		return pPdf;
	}

	void AddPage(CPdfFile& oPdf, const PageSpec& oPage)
	{
		oPdf.NewPage();
		oPdf.put_Width(oPage.dWidthMM);
		oPdf.put_Height(oPage.dHeightMM);

		oPdf.put_BrushType(c_BrushTypeSolid);
		oPdf.put_BrushColor1(oPage.lColor);
		oPdf.put_BrushAlpha1(255);
		oPdf.PathCommandStart();
		oPdf.PathCommandMoveTo(0, 0);
		oPdf.PathCommandLineTo(oPage.dWidthMM, 0);
		oPdf.PathCommandLineTo(oPage.dWidthMM, oPage.dHeightMM);
		oPdf.PathCommandLineTo(0, oPage.dHeightMM);
		oPdf.PathCommandClose();
		oPdf.DrawPath(c_nWindingFillMode);
		oPdf.PathCommandEnd();
	}

	// Writes a PDF with these pages. Set up the writer further with prepare.
	template<typename F>
	void WritePdf(const std::wstring& wsPath, const std::vector<PageSpec>& arrPages, F prepare)
	{
		std::unique_ptr<CPdfFile> pPdf = NewPdf();
		pPdf->CreatePdf();
		prepare(*pPdf);
		for (const PageSpec& oPage : arrPages)
			AddPage(*pPdf, oPage);
		ASSERT_EQ(0, pPdf->SaveToFile(wsPath));
	}
	void WritePdf(const std::wstring& wsPath, const std::vector<PageSpec>& arrPages)
	{
		WritePdf(wsPath, arrPages, [](CPdfFile&) {});
	}

	std::unique_ptr<CPdfFile> Load(const std::wstring& wsPath, const wchar_t* wsPassword = NULL)
	{
		std::unique_ptr<CPdfFile> pPdf = NewPdf();
		EXPECT_TRUE(pPdf->LoadFromFile(wsPath, L"", wsPassword, wsPassword)) << U_TO_UTF8(wsPath);
		return pPdf;
	}

	// Page size in mm
	void PageSize(CPdfFile& oPdf, int nPage, double& dW, double& dH)
	{
		double dDpiX = 0, dDpiY = 0;
		oPdf.GetPageInfo(nPage, &dW, &dH, &dDpiX, &dDpiY);
		dW = dW / dDpiX * 25.4;
		dH = dH / dDpiY * 25.4;
	}

	// Colour in the middle of the rendered page, as 0xBBGGRR
	LONG PageColor(CPdfFile& oPdf, int nPage)
	{
		const int w = 40, h = 40;
		BYTE* pPixels = oPdf.ConvertToPixels(nPage, w, h, true);
		if (!pPixels)
			return -1;
		const BYTE* p = pPixels + ((h / 2) * w + w / 2) * 4;
		LONG lColor = (LONG)p[0] | ((LONG)p[1] << 8) | ((LONG)p[2] << 16);
		delete[] pPixels;
		return lColor;
	}

	// Checks the pages by size and colour, in this order
	void ExpectPages(CPdfFile& oPdf, const std::vector<PageSpec>& arrPages)
	{
		ASSERT_EQ((int)arrPages.size(), oPdf.GetPagesCount());
		for (size_t i = 0; i < arrPages.size(); ++i)
		{
			double dW = 0, dH = 0;
			PageSize(oPdf, (int)i, dW, dH);
			EXPECT_NEAR(arrPages[i].dWidthMM, dW, 0.5) << "page " << i;
			EXPECT_NEAR(arrPages[i].dHeightMM, dH, 0.5) << "page " << i;
			EXPECT_EQ(arrPages[i].lColor, PageColor(oPdf, (int)i)) << "page " << i;
		}
	}

	std::vector<BYTE> ReadAll(const std::wstring& wsPath)
	{
		BYTE* pData = NULL;
		DWORD dwSize = 0;
		std::vector<BYTE> arrRes;
		if (NSFile::CFileBinary::ReadAllBytes(wsPath, &pData, dwSize))
			arrRes.assign(pData, pData + dwSize);
		RELEASEARRAYOBJECTS(pData);
		return arrRes;
	}
};

NSFonts::IApplicationFonts* CPdfFileTest::m_pFonts = NULL;

TEST_F(CPdfFileTest, WriteAndRead)
{
	std::wstring wsFile = Path(L"three.pdf");
	WritePdf(wsFile, { c_oA4, c_oLandscape, c_oLetter });

	std::vector<BYTE> arrData = ReadAll(wsFile);
	ASSERT_GE(arrData.size(), 5u);
	EXPECT_EQ(0, memcmp(arrData.data(), "%PDF-", 5));

	std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
	EXPECT_EQ(PdfFile::errorNone, pPdf->GetError());
	ExpectPages(*pPdf, { c_oA4, c_oLandscape, c_oLetter });
}

TEST_F(CPdfFileTest, LoadFromMemory)
{
	std::wstring wsFile = Path(L"one.pdf");
	WritePdf(wsFile, { c_oLetter });

	std::vector<BYTE> arrData = ReadAll(wsFile);
	std::unique_ptr<CPdfFile> pPdf = NewPdf();
	ASSERT_TRUE(pPdf->LoadFromMemory(arrData.data(), (DWORD)arrData.size()));
	ExpectPages(*pPdf, { c_oLetter });
}

TEST_F(CPdfFileTest, NotAPdf)
{
	std::wstring wsFile = Path(L"text.pdf");
	NSFile::CFileBinary::SaveToFile(wsFile, L"this is not a PDF");

	std::unique_ptr<CPdfFile> pPdf = NewPdf();
	EXPECT_FALSE(pPdf->LoadFromFile(wsFile));
	EXPECT_NE(PdfFile::errorNone, pPdf->GetError());
}

TEST_F(CPdfFileTest, DocumentInfo)
{
	std::wstring wsFile = Path(L"info.pdf");
	WritePdf(wsFile, { c_oA4 }, [](CPdfFile& oPdf)
	{
		oPdf.SetDocumentInfo(L"Quarterly report", L"Euro-Office test", L"Subject line", L"alpha, beta");
	});

	std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
	std::string sInfo = U_TO_UTF8(pPdf->GetInfo());
	EXPECT_NE(std::string::npos, sInfo.find("Quarterly report")) << sInfo;
	EXPECT_NE(std::string::npos, sInfo.find("Subject line")) << sInfo;
	EXPECT_NE(std::string::npos, sInfo.find("alpha, beta")) << sInfo;
}

TEST_F(CPdfFileTest, MetaData)
{
	// Docxf forms keep their source document in the PDF this way
	std::string sMeta = "PK\x03\x04 form source with a \0 byte";
	sMeta.push_back('\0');
	sMeta += "and more";

	std::wstring wsFile = Path(L"meta.pdf");
	WritePdf(wsFile, { c_oA4 }, [&](CPdfFile& oPdf)
	{
		oPdf.AddMetaData(L"ONLYOFFICEFORM", (BYTE*)sMeta.data(), (DWORD)sMeta.size());
	});

	BYTE* pData = NULL;
	DWORD dwLength = 0;
	ASSERT_TRUE(CPdfFile::GetMetaData(wsFile, L"ONLYOFFICEFORM", &pData, dwLength));
	EXPECT_EQ(std::string((char*)pData, dwLength), sMeta);
	RELEASEARRAYOBJECTS(pData);

	EXPECT_FALSE(CPdfFile::GetMetaData(wsFile, L"OTHERNAME", &pData, dwLength));
	RELEASEARRAYOBJECTS(pData);

	std::wstring wsPlain = Path(L"plain.pdf");
	WritePdf(wsPlain, { c_oA4 });
	EXPECT_FALSE(CPdfFile::GetMetaData(wsPlain, L"ONLYOFFICEFORM", &pData, dwLength));
	RELEASEARRAYOBJECTS(pData);
}

TEST_F(CPdfFileTest, Password)
{
	std::wstring wsFile = Path(L"locked.pdf");
	WritePdf(wsFile, { c_oA4, c_oLetter }, [](CPdfFile& oPdf)
	{
		oPdf.SetPassword(L"s3cret");
	});

	std::unique_ptr<CPdfFile> pLocked = NewPdf();
	EXPECT_FALSE(pLocked->LoadFromFile(wsFile));
	EXPECT_EQ(PdfFile::errorEncrypted, pLocked->GetError());

	std::unique_ptr<CPdfFile> pWrong = NewPdf();
	EXPECT_FALSE(pWrong->LoadFromFile(wsFile, L"", L"wrong", L"wrong"));

	std::unique_ptr<CPdfFile> pPdf = Load(wsFile, L"s3cret");
	ExpectPages(*pPdf, { c_oA4, c_oLetter });
}

TEST_F(CPdfFileTest, ChangePassword)
{
	std::wstring wsFile = Path(L"open.pdf");
	WritePdf(wsFile, { c_oLandscape });

	std::wstring wsLocked = Path(L"locked.pdf");
	{
		std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
		EXPECT_GE(pPdf->ChangePassword(wsLocked, L"new-pass"), 0);
	}
	{
		std::unique_ptr<CPdfFile> pPdf = NewPdf();
		EXPECT_FALSE(pPdf->LoadFromFile(wsLocked));
	}

	std::wstring wsUnlocked = Path(L"unlocked.pdf");
	{
		std::unique_ptr<CPdfFile> pPdf = Load(wsLocked, L"new-pass");
		ExpectPages(*pPdf, { c_oLandscape });
		EXPECT_GE(pPdf->ChangePassword(wsUnlocked), 0);
	}
	std::unique_ptr<CPdfFile> pPdf = Load(wsUnlocked);
	ExpectPages(*pPdf, { c_oLandscape });
}

TEST_F(CPdfFileTest, EditDeleteAddMoveRotate)
{
	std::wstring wsFile = Path(L"source.pdf");
	WritePdf(wsFile, { c_oA4, c_oLandscape, c_oLetter });

	std::wstring wsEdited = Path(L"edited.pdf");
	{
		std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
		ASSERT_TRUE(pPdf->EditPdf(wsEdited));
		// EditPage() takes the page's index in the source document,
		// MovePage() its current position: rotate before reordering.
		ASSERT_TRUE(pPdf->EditPage(2));
		pPdf->RotatePage(90);                              // Letter rotated
		ASSERT_TRUE(pPdf->DeletePage(1));                  // A4, Letter
		ASSERT_TRUE(pPdf->MovePage(1, 0));                 // Letter, A4
		ASSERT_TRUE(pPdf->AddPage(2));                     // Letter, A4, new
		pPdf->put_Width(c_oLandscape.dWidthMM);
		pPdf->put_Height(c_oLandscape.dHeightMM);
		pPdf->Close();
	}

	std::unique_ptr<CPdfFile> pPdf = Load(wsEdited);
	ASSERT_EQ(3, pPdf->GetPagesCount());
	EXPECT_EQ(c_lBlue, PageColor(*pPdf, 0));
	EXPECT_EQ(c_lRed, PageColor(*pPdf, 1));
	EXPECT_EQ(90, pPdf->GetRotate(0));
	EXPECT_EQ(0, pPdf->GetRotate(1));

	// The source is unchanged
	std::unique_ptr<CPdfFile> pSource = Load(wsFile);
	ExpectPages(*pSource, { c_oA4, c_oLandscape, c_oLetter });
}

TEST_F(CPdfFileTest, DrawOnPage)
{
	std::wstring wsFile = Path(L"source.pdf");
	WritePdf(wsFile, { c_oA4, c_oLetter });

	std::wstring wsEdited = Path(L"edited.pdf");
	{
		std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
		ASSERT_TRUE(pPdf->EditPdf(wsEdited));
		ASSERT_TRUE(pPdf->EditPage(0));
		// A black square over the middle of the page
		pPdf->put_BrushType(c_BrushTypeSolid);
		pPdf->put_BrushColor1(c_lBlack);
		pPdf->put_BrushAlpha1(255);
		pPdf->PathCommandStart();
		pPdf->PathCommandMoveTo(80, 120);
		pPdf->PathCommandLineTo(130, 120);
		pPdf->PathCommandLineTo(130, 180);
		pPdf->PathCommandLineTo(80, 180);
		pPdf->PathCommandClose();
		pPdf->DrawPath(c_nWindingFillMode);
		pPdf->PathCommandEnd();
		pPdf->Close();
	}

	std::unique_ptr<CPdfFile> pPdf = Load(wsEdited);
	ASSERT_EQ(2, pPdf->GetPagesCount());
	EXPECT_EQ(c_lBlack, PageColor(*pPdf, 0));
	EXPECT_EQ(c_lBlue, PageColor(*pPdf, 1));
}

TEST_F(CPdfFileTest, SplitPages)
{
	std::wstring wsFile = Path(L"source.pdf");
	WritePdf(wsFile, { c_oA4, c_oLandscape, c_oLetter });

	std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
	std::vector<int> arrPages = { 0, 2 };
	BYTE* pSplit = pPdf->SplitPages(arrPages.data(), (unsigned int)arrPages.size());
	ASSERT_NE(nullptr, pSplit);

	// [length including these 4 bytes][PDF]
	int nLength = ReadInt(pSplit);
	ASSERT_GT(nLength, 4);
	std::vector<BYTE> arrPdf(pSplit + 4, pSplit + nLength);
	RELEASEARRAYOBJECTS(pSplit);

	std::unique_ptr<CPdfFile> pPart = NewPdf();
	ASSERT_TRUE(pPart->LoadFromMemory(arrPdf.data(), (DWORD)arrPdf.size()));
	ExpectPages(*pPart, { c_oA4, c_oLetter });
}

TEST_F(CPdfFileTest, MergePages)
{
	std::wstring wsFirst = Path(L"first.pdf");
	std::wstring wsSecond = Path(L"second.pdf");
	WritePdf(wsFirst, { c_oA4 });
	WritePdf(wsSecond, { c_oLandscape, c_oLetter });

	std::wstring wsMerged = Path(L"merged.pdf");
	{
		std::unique_ptr<CPdfFile> pPdf = Load(wsFirst);
		ASSERT_TRUE(pPdf->EditPdf(wsMerged));
		ASSERT_TRUE(pPdf->MergePages(wsSecond));
		pPdf->Close();
	}

	std::unique_ptr<CPdfFile> pPdf = Load(wsMerged);
	ExpectPages(*pPdf, { c_oA4, c_oLandscape, c_oLetter });
}

TEST_F(CPdfFileTest, CopyPagesThroughRenderer)
{
	std::wstring wsFile = Path(L"source.pdf");
	WritePdf(wsFile, { c_oLandscape, c_oA4 });

	std::wstring wsCopy = Path(L"copy.pdf");
	{
		std::unique_ptr<CPdfFile> pSource = Load(wsFile);
		std::unique_ptr<CPdfFile> pCopy = NewPdf();
		pCopy->CreatePdf();
		for (int i = 0; i < pSource->GetPagesCount(); ++i)
		{
			double dW = 0, dH = 0;
			PageSize(*pSource, i, dW, dH);
			pCopy->NewPage();
			pCopy->put_Width(dW);
			pCopy->put_Height(dH);
			pSource->DrawPageOnRenderer(pCopy.get(), i, NULL);
		}
		ASSERT_EQ(0, pCopy->SaveToFile(wsCopy));
	}

	std::unique_ptr<CPdfFile> pPdf = Load(wsCopy);
	ExpectPages(*pPdf, { c_oLandscape, c_oA4 });
}

class CPdfSignTest : public CPdfFileTest
{
protected:
	// Signs page 0 of wsFile into wsSigned with a new certificate; returns it
	ICertificate* SignInto(const std::wstring& wsFile, const std::wstring& wsSigned)
	{
		std::map<std::wstring, std::wstring> mapProps;
		mapProps.insert(std::make_pair(L"email", L"test@euro-office.example"));
		ICertificate* pCertificate = NSCertificate::GenerateByAlg("rsa2048", mapProps);
		EXPECT_NE(nullptr, pCertificate);
		if (!pCertificate)
			return NULL;

		std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
		EXPECT_TRUE(pPdf->EditPdf(wsSigned));
		EXPECT_TRUE(pPdf->EditPage(0));
		pPdf->Sign(10, 10, 60, 20);
		// Signing needs EditClose() first, then the byte range is prepared
		pPdf->EditClose();
		EXPECT_TRUE(pPdf->PrepareSignature(wsSigned));

		std::vector<BYTE> arrFile = ReadAll(wsSigned);
		std::vector<BYTE> arrToSign = SignedBytes(arrFile);
		EXPECT_FALSE(arrToSign.empty());

		BYTE* pSignature = NULL;
		unsigned int nSignature = 0;
		EXPECT_TRUE(pCertificate->SignPKCS7(arrToSign.data(), (unsigned int)arrToSign.size(), pSignature, nSignature));
		EXPECT_TRUE(pPdf->FinalizeSignature(pSignature, nSignature));
		RELEASEARRAYOBJECTS(pSignature);
		pPdf->Close();
		return pCertificate;
	}

	// The bytes /ByteRange [a b c d] covers
	std::vector<BYTE> SignedBytes(const std::vector<BYTE>& arrFile)
	{
		const std::string sKey = "/ByteRange";
		size_t nPos = std::string((const char*)arrFile.data(), arrFile.size()).rfind(sKey);
		if (std::string::npos == nPos)
			return std::vector<BYTE>();
		nPos += sKey.size();
		long arrRange[4] = { 0 };
		if (4 != sscanf((const char*)arrFile.data() + nPos, " [ %ld %ld %ld %ld", &arrRange[0], &arrRange[1], &arrRange[2], &arrRange[3]))
			return std::vector<BYTE>();
		if (arrRange[0] + arrRange[1] > (long)arrFile.size() || arrRange[2] + arrRange[3] > (long)arrFile.size())
			return std::vector<BYTE>();
		std::vector<BYTE> arrRes(arrFile.begin() + arrRange[0], arrFile.begin() + arrRange[0] + arrRange[1]);
		arrRes.insert(arrRes.end(), arrFile.begin() + arrRange[2], arrFile.begin() + arrRange[2] + arrRange[3]);
		return arrRes;
	}

	// (widget, result) pairs from VerifySign
	std::vector<std::pair<int, int>> Verify(const std::wstring& wsFile, ICertificate* pCertificate)
	{
		std::vector<std::pair<int, int>> arrRes;
		std::unique_ptr<CPdfFile> pPdf = Load(wsFile);
		BYTE* pResult = pPdf->VerifySign(wsFile, pCertificate);
		if (!pResult)
			return arrRes;
		int nLength = ReadInt(pResult);
		for (int i = 4; i + 8 <= nLength; i += 8)
			arrRes.push_back(std::make_pair(ReadInt(pResult + i), ReadInt(pResult + i + 4)));
		RELEASEARRAYOBJECTS(pResult);
		return arrRes;
	}
};

TEST_F(CPdfSignTest, SignAndVerify)
{
	std::wstring wsFile = Path(L"source.pdf");
	WritePdf(wsFile, { c_oA4, c_oLetter });

	std::wstring wsSigned = Path(L"signed.pdf");
	std::unique_ptr<ICertificate> pCertificate(SignInto(wsFile, wsSigned));
	ASSERT_TRUE(pCertificate);

	std::vector<std::pair<int, int>> arrResults = Verify(wsSigned, pCertificate.get());
	ASSERT_EQ(1u, arrResults.size());
	EXPECT_EQ(c_nSignIntact | c_nSignVerified, arrResults[0].second);

	// Signing keeps the pages
	std::unique_ptr<CPdfFile> pPdf = Load(wsSigned);
	ExpectPages(*pPdf, { c_oA4, c_oLetter });
}

// Covers the redaction early-return in CPdfWriter::DrawText (#224). Text that
// lands inside a redaction area returns before CRendererTextCommand takes
// ownership of the buffer, so DrawText has to free it. This case drives that
// path and checks it succeeds; it does not measure the heap.
TEST_F(CPdfFileTest, DrawTextInsideRedaction)
{
	ASSERT_NE(m_pFonts, nullptr);

	std::wstring wsTemp = Path(L"temp");
	NSDirectory::CreateDirectory(wsTemp);
	CPdfWriter writer(m_pFonts, false, nullptr, true, wsTemp);
	ASSERT_EQ(S_OK, writer.NewPage());

	// Bundled Base14 font: no dependency on system fonts.
	writer.put_FontName(L"Embedded: Helvetica");
	writer.put_FontSize(12);

	// Quad (page coords in points, y from top) big enough that the text
	// origin (10,10) always falls inside it, so SkipRedact() returns true.
	ASSERT_EQ(S_OK, writer.AddRedact({
		-10000.0, -10000.0,
		 10000.0, -10000.0,
		 10000.0,  10000.0,
		-10000.0,  10000.0
	}));

	const std::wstring wsText(256, L'A');
	for (int i = 0; i < 100; ++i)
		ASSERT_EQ(S_OK, writer.CommandDrawText(wsText, 10, 10, 100, 20));
}
