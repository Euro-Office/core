/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */
#include "DjVuFileImplementation.h"

#include "../DesktopEditor/common/File.h"
#include "../DesktopEditor/common/Directory.h"

#include "../PdfFile/PdfFile.h"

#include "../DesktopEditor/graphics/pro/Fonts.h"
#include "../DesktopEditor/graphics/pro/Graphics.h"
#include "../DesktopEditor/graphics/pro/Image.h"

#include "../DesktopEditor/common/StringExt.h"

#define VER_DPI		96
#define HOR_DPI		96

#include <algorithm>
#include <vector>

#include "DjVuInfo.h"

#include "../DesktopEditor/graphics/pro/js/wasm/src/serialize.h"

#ifdef BUILDING_WASM_MODULE
#define DISABLE_TEMP_DIRECTORY
#endif

namespace NSDjvu
{
	static GUTF8String MakeUTF8String(const std::wstring& wsText)
	{
		std::string sText = NSFile::CUtf8Converter::GetUtf8StringFromUnicode(wsText);
		GUTF8String utf8String(sText.c_str());
		return utf8String;
	}
	static std::string MakeCString(GUTF8String& strText)
	{
		std::string sString(strText.getbuf());
		return sString;
	}
	static int GetInteger(const std::wstring& wsString)
	{
		if (wsString.size() < 1)
			return 0;

		try
		{
			return std::stoi(wsString);
		}
		catch (...)
		{
			try
			{
				return static_cast<int>(std::stol(wsString));
			}
			catch (...)
			{
				return 0;
			}
		}
	}
	static int ComputeRed(int w, int h, int rw, int rh)
	{
	  for (int red = 1; red < 16; ++red)
		  if ((w + red - 1) / red == rw && (h + red - 1) / red == rh)
			  return red;
	  return 1;
	}
	static void DrawPixmap(GP<GPixmap> pImage, Aggplus::CImage* pImageRes)
	{
		unsigned int unPixmapH = pImage->rows();
		unsigned int unPixmapW = pImage->columns();
		BYTE* pBufferDst = new BYTE[4 * static_cast<size_t>(unPixmapH) * unPixmapW];
		if (!pBufferDst)
			return;

		pImageRes->Create(pBufferDst, unPixmapW, unPixmapH, 4 * static_cast<long>(unPixmapW));

		BYTE* pBuffer = pBufferDst;
		for (int j = unPixmapH - 1; j >= 0; --j)
		{
			GPixel* pLine = pImage->operator [](j);

			for (int i = 0; i < unPixmapW; ++i, pBuffer += 4, ++pLine)
			{
				pBuffer[0] = pLine->b;
				pBuffer[1] = pLine->g;
				pBuffer[2] = pLine->r;
				pBuffer[3] = 255;
			}
		}
	}
}

namespace
{
	// Reads the size of a page from its INFO (or IW44) chunk without decoding
	// the page and without waiting for DjVuFile locks (moved here from the
	// WinDjView version of DjVuDocument)
	void ReadPageInfo(const GP<DjVuDocument>& pDoc, int nPage, int& width, int& height, int& nDpi)
	{
		width  = 1;
		height = 1;
		nDpi   = 96;

		try
		{
			GURL url = pDoc->page_to_url(nPage);
			GP<DataPool> pool = pDoc->request_data(NULL, url);
			if (!pool)
				return;
			GP<ByteStream> stream = pool->get_stream();
			GP<IFFByteStream> iff(IFFByteStream::create(stream));

			GUTF8String chkid;
			if (!iff->get_chunk(chkid) ||
				(chkid != "FORM:DJVI" && chkid != "FORM:DJVU" &&
				 chkid != "FORM:PM44" && chkid != "FORM:BM44"))
			{
				return;
			}

			bool bHasIW44 = false;
			while (iff->get_chunk(chkid) != 0)
			{
				GP<ByteStream> chunk_stream = iff->get_bytestream();

				if (chkid == "INFO")
				{
					GP<DjVuInfo> pInfo = DjVuInfo::create();
					pInfo->decode(*chunk_stream);

					width  = std::max(pInfo->width, 0);
					height = std::max(pInfo->height, 0);
					nDpi   = std::max(pInfo->dpi, 0);

					if ((pInfo->orientation & 1) != 0)
						std::swap(width, height);
				}
				else if (!bHasIW44 && (chkid == "PM44" || chkid == "BM44"))
				{
					bHasIW44 = true;

					// serial, slices, major, minor, then the size
					for (int i = 0; i < 4; ++i)
						chunk_stream->read8();
					unsigned int xhi = chunk_stream->read8();
					unsigned int xlo = chunk_stream->read8();
					unsigned int yhi = chunk_stream->read8();
					unsigned int ylo = chunk_stream->read8();

					width  = (xhi << 8) | xlo;
					height = (yhi << 8) | ylo;
					nDpi   = 100;
				}

				iff->seek_close_chunk();
			}
		}
		catch (...)
		{
		}
	}
}

CDjVuFileImplementation::CDjVuFileImplementation(NSFonts::IApplicationFonts* pFonts)
{
	m_pDoc = nullptr;
	m_wsTempDirectory = L"";
	SetTempDirectory(L"");
	m_pApplicationFonts = pFonts;
}
CDjVuFileImplementation::~CDjVuFileImplementation()
{
#ifndef DISABLE_TEMP_DIRECTORY
	NSDirectory::DeleteDirectory(m_wsTempDirectory);
#endif
}
NSFonts::IApplicationFonts* CDjVuFileImplementation::GetFonts()
{
	return m_pApplicationFonts;
}

bool CDjVuFileImplementation::LoadFromFile(const std::wstring& wsSrcFileName, const std::wstring& /*wsXMLOptions*/)
{
	m_pDoc = nullptr;
	try
	{
		GUTF8String utf8;
		GURL url = GURL::Filename::UTF8(NSDjvu::MakeUTF8String(wsSrcFileName));
		m_pDoc = DjVuDocument::create(url);
		// libdjvu initializes the document (directory, bookmarks) in a thread
		m_pDoc->wait_for_complete_init();
		if (!m_pDoc->is_init_ok())
		{
			m_pDoc = NULL;
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}
bool CDjVuFileImplementation::LoadFromMemory(BYTE* data, DWORD length, const std::wstring& /*wsXmlOptions*/)
{
	m_pDoc = nullptr;
	try
	{
		GP<ByteStream> stream = ByteStream::create(data, (size_t)length);
		m_pDoc = DjVuDocument::create(stream);
		// libdjvu initializes the document (directory, bookmarks) in a thread
		m_pDoc->wait_for_complete_init();
		if (!m_pDoc->is_init_ok())
		{
			m_pDoc = NULL;
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}
void CDjVuFileImplementation::Close()
{
}
std::wstring CDjVuFileImplementation::GetTempDirectory() const
{
	return m_wsTempDirectory;
}
void CDjVuFileImplementation::SetTempDirectory(const std::wstring& wsDirectory)
{
#ifndef DISABLE_TEMP_DIRECTORY
	if (!m_wsTempDirectory.empty())
		NSDirectory::DeleteDirectory(m_wsTempDirectory);

	m_wsTempDirectory = wsDirectory;
	if (m_wsTempDirectory.empty())
		m_wsTempDirectory = NSFile::CFileBinary::GetTempPath();

	m_wsTempDirectory += L"/DJVU/";
	NSDirectory::CreateDirectory(m_wsTempDirectory);
#endif
}

int CDjVuFileImplementation::GetPagesCount() const
{
	if (!m_pDoc)
		return 0;

	return m_pDoc->get_pages_num();
}
void CDjVuFileImplementation::GetPageInfo(int nPageIndex, double* pdWidth, double* pdHeight, double* pdDpiX, double* pdDpiY) const
{
	if (!m_pDoc)
	{
		*pdWidth  = 0;
		*pdHeight = 0;

		*pdDpiX = 96;
		*pdDpiY = 96;
	}

#if 0
	GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);

	pPage->wait_for_complete_decode();
	pPage->set_rotate(0);

	*pdWidth = pPage->get_real_width();
	*pdHeight = pPage->get_real_height();

	*pdDpiX = pPage->get_dpi();
	*pdDpiY = pPage->get_dpi();
#endif
	int nW = 0;
	int nH = 0;
	int nDpi = 0;
	ReadPageInfo(m_pDoc, nPageIndex, nW, nH, nDpi);
	*pdWidth = nW;
	*pdHeight = nH;
	*pdDpiX = nDpi;
	*pdDpiY = nDpi;
}
void  CDjVuFileImplementation::DrawPageOnRenderer(IRenderer* pRenderer, int nPageIndex, bool* /*pBreak*/)
{
	if (!m_pDoc)
		return;

	try
	{
		GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);
		pPage->wait_for_complete_decode();
		pPage->set_rotate(0);

		long lRendererType = c_nUnknownRenderer;
		pRenderer->get_Type(&lRendererType);

		XmlUtils::CXmlNode text;
		if (c_nPDFWriter == lRendererType || c_nHtmlRendrererText == lRendererType)
			text = ParseText(pPage);

		CreateFrame(pRenderer, pPage, nPageIndex, text);
	}
	catch (...)
	{
		// Render nothing instead of a broken page.
		return;
	}
}
void CDjVuFileImplementation::ConvertToPdf(const std::wstring& wsDstPath)
{
	CPdfFile oPdf(m_pApplicationFonts);
	oPdf.CreatePdf();
	
	bool bBreak = false;
	for (int nPageIndex = 0, nPagesCount = GetPagesCount(); nPageIndex < nPagesCount; nPageIndex++)
	{
		oPdf.NewPage();

		double dPageDpiX, dPageDpiY;
		double dWidth, dHeight;
		GetPageInfo(nPageIndex, &dWidth, &dHeight, &dPageDpiX, &dPageDpiY);
		dWidth  *= 25.4 / dPageDpiX;
		dHeight *= 25.4 / dPageDpiY;
		oPdf.put_Width(dWidth);
		oPdf.put_Height(dHeight);

		DrawPageOnRenderer(&oPdf, nPageIndex, &bBreak);
#ifdef _DEBUG
		printf("%d of %d pages\n", nPageIndex + 1, nPagesCount);
#endif
	}

	oPdf.SaveToFile(wsDstPath);
}
std::wstring CDjVuFileImplementation::GetInfo()
{
	std::wstring sRes = L"{";

	double nW = 0;
	double nH = 0;
	double nDpi = 0;
	GetPageInfo(0, &nW, &nH, &nDpi, &nDpi);
	sRes += L"\"PageWidth\":";
	sRes += std::to_wstring((int)(nW * 100));
	sRes += L",\"PageHeight\":";
	sRes += std::to_wstring((int)(nH * 100));
	sRes += L",\"NumberOfPages\":";
	sRes += std::to_wstring(GetPagesCount());
	sRes += L"}";

	return sRes;
}

void getBookmars(const GP<DjVmNav>& nav, int& pos, int count, NSWasm::CData& out, int level)
{
	while (count > 0 && pos < nav->getBookMarkCount())
	{
		GP<DjVmNav::DjVuBookMark> gpBookMark;
		nav->getBookMark(gpBookMark, pos++);

		GUTF8String str = gpBookMark->url;
		int endpos;
		DWORD nPage = str.toULong(1, endpos) - 1;
		if (endpos == (int)str.length())
		{
			out.AddInt(nPage);
			out.AddInt(level);
			out.AddDouble(0.0);
			GUTF8String description = gpBookMark->displayname;
			out.WriteString((BYTE*)description.getbuf(), description.length());
		}

		getBookmars(nav, pos, gpBookMark->count, out, level + 1);
		count--;
	}
}
BYTE* CDjVuFileImplementation::GetStructure()
{
	GP<DjVmNav> nav = m_pDoc->get_djvm_nav();
	if (!nav)
		return nullptr;

	int pos = 0;
	int count = nav->getBookMarkCount();
	if (count <= 0)
		return nullptr;

	NSWasm::CData oRes;
	oRes.SkipLen();
	getBookmars(nav, pos, count, oRes, 1);
	oRes.WriteLen();
	BYTE* bRes = oRes.GetBuffer();
	oRes.ClearWithoutAttack();
	return bRes;
}
BYTE* CDjVuFileImplementation::GetPageGlyphs(int nPageIndex)
{
	return nullptr;
	try
	{
		GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);
		const GP<DjVuText> text(DjVuText::create());
		const GP<ByteStream> text_str(pPage->get_text());
		if (!text_str)
			return nullptr;
		text->decode(text_str);

		GUTF8String pageText = text->get_xmlText(pPage->get_height());
		XmlUtils::CXmlNode hiddenText;
		XmlUtils::CXmlNode pageColumn;
		XmlUtils::CXmlNode region;
		hiddenText.FromXmlStringA(NSDjvu::MakeCString(pageText));
		hiddenText.GetNode(L"PAGECOLUMN", pageColumn);
		pageColumn.GetNode(L"REGION", region);

        NSWasm::CData oRes;
		oRes.SkipLen();
		std::vector<XmlUtils::CXmlNode> oParagraphsNodes = region.GetNodes(L"PARAGRAPH");
        for (size_t nParagraphIndex = 0; nParagraphIndex < oParagraphsNodes.size(); nParagraphIndex++)
        {
            XmlUtils::CXmlNode & oParagraphNode = oParagraphsNodes[nParagraphIndex];

			std::vector<XmlUtils::CXmlNode> oLinesNodes = oParagraphNode.GetNodes(L"LINE");
            for (size_t nLineIndex = 0; nLineIndex < oLinesNodes.size(); nLineIndex++)
            {
                XmlUtils::CXmlNode & oLineNode = oLinesNodes[nLineIndex];

				std::vector<XmlUtils::CXmlNode> oWordsNodes = oLineNode.GetNodes(L"WORD");
                for (size_t nWordIndex = 0; nWordIndex < oWordsNodes.size(); nWordIndex++)
                {
                    XmlUtils::CXmlNode & oWordNode = oWordsNodes[nWordIndex];

                    std::wstring csWord   = oWordNode.GetText();
                    std::wstring csCoords = oWordNode.GetAttribute(L"coords");
                    double arrCoords[4] = {0, 0, 0, 0};
                    ParseCoords(csCoords, arrCoords, 1);

					std::string sText = U_TO_UTF8(csWord);
					oRes.WriteString((BYTE*)sText.c_str(), sText.length());
					oRes.AddDouble(arrCoords[0]);
					oRes.AddDouble(arrCoords[3]);
					oRes.AddDouble(arrCoords[2] - arrCoords[0]);
					oRes.AddDouble(arrCoords[1] - arrCoords[3]);
				}
			}
		}
		oRes.WriteLen();

		BYTE* res = oRes.GetBuffer();
		oRes.ClearWithoutAttack();
		return res;
	}
	catch (...)
	{
		return nullptr;
	}
}
BYTE* CDjVuFileImplementation::GetPageLinks(int nPageIndex)
{
	double dPageDpiX, dPageDpiY;
	double dWidth, dHeight;
	GetPageInfo(nPageIndex, &dWidth, &dHeight, &dPageDpiX, &dPageDpiY);

	try
	{
		GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);
		pPage->wait_for_complete_decode();
		GP<DjVuAnno> pAnno = pPage->get_decoded_anno();
		if (!pAnno)
			return nullptr;
		GPList<GMapArea> map_areas = pAnno->ant->map_areas;

		NSWasm::CData oRes;
		oRes.SkipLen();
		for (GPosition pos(map_areas); pos; ++pos)
		{
			GUTF8String str = map_areas[pos]->url;
			oRes.WriteString((BYTE*)str.getbuf(), str.length());
			// Upper left corner
			double x = map_areas[pos]->get_xmin();
			double y = dHeight - map_areas[pos]->get_ymax();
			oRes.AddDouble(0.0);
			oRes.AddDouble(x);
			oRes.AddDouble(y);
			oRes.AddDouble(map_areas[pos]->get_xmax() - x);
			oRes.AddDouble(map_areas[pos]->get_ymax() - map_areas[pos]->get_ymin());
		}
		oRes.WriteLen();

		BYTE* res = oRes.GetBuffer();
		oRes.ClearWithoutAttack();
		return res;
	}
	catch (...)
	{
		return nullptr;
	}
}

void CDjVuFileImplementation::CreateFrame(IRenderer* pRenderer, GP<DjVuImage>& pPage, int nPage, XmlUtils::CXmlNode& text)
{
	long lRendererType = c_nUnknownRenderer;
	pRenderer->get_Type(&lRendererType);
	if (c_nPDFWriter == lRendererType)
	{
		CreatePdfFrame(pRenderer, pPage, nPage, text);
		return;
	}

	int nDpi = pPage->get_dpi();
	double dPixToMM = 25.4;

	if (c_nHtmlRendrererText == lRendererType && text.IsValid())
	{
		TextToRenderer(pRenderer, text, dPixToMM / nDpi);
		return;
	}

	double dRendDpiX = 0;
	double dRendDpiY = 0;

	pRenderer->get_DpiX(&dRendDpiX);
	pRenderer->get_DpiY(&dRendDpiY);

	if (0 >= dRendDpiX)
		dRendDpiX = 72.0;
	if (0 >= dRendDpiY)
		dRendDpiY = 72.0;

	double dRendWidth = 0;
	double dRendHeight = 0;

	pRenderer->get_Width(&dRendWidth);
	pRenderer->get_Height(&dRendHeight);

	if (0 >= dRendWidth)
		dRendWidth = 200;
	if (0 >= dRendHeight)
		dRendHeight = 300;

	LONG lImageWidth	= (LONG)(dRendDpiX * dRendWidth / dPixToMM);
	LONG lImageHeight	= (LONG)(dRendDpiY * dRendHeight / dPixToMM);

	BYTE* pBufferDst = this->ConvertToPixels(pPage, lImageWidth, lImageHeight, false);

	if (!pBufferDst)
		return;

	Aggplus::CImage oImage;
	oImage.Create(pBufferDst, lImageWidth, lImageHeight, 4 * lImageWidth);

	pRenderer->BeginCommand(c_nPageType);

	if (text.IsValid())
	{
		TextToRenderer(pRenderer, text, dPixToMM / nDpi);
	}

	pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dRendWidth, dRendHeight);
	pRenderer->EndCommand(c_nPageType);
}
void CDjVuFileImplementation::CreatePdfFrame(IRenderer* pRenderer, GP<DjVuImage>& pPage, int nPageIndex, XmlUtils::CXmlNode& oText)
{
	double dPageDpiX, dPageDpiY;
	double dWidth, dHeight;
	GetPageInfo(nPageIndex, &dWidth, &dHeight, &dPageDpiX, &dPageDpiY);
	dWidth  *= 25.4 / dPageDpiX;
	dHeight *= 25.4 / dPageDpiY;

	pRenderer->BeginCommand(c_nPageType);

	TextToRenderer(pRenderer, oText, 25.4 / pPage->get_dpi());

	LONG lImageWidth  = pPage->get_real_width();
	LONG lImageHeight = pPage->get_real_height();
	double dImageRedW = dWidth  * 72.0 / 25.4 * 2.0;
	double dImageRedH = dHeight * 72.0 / 25.4 * 2.0;
	LONG lRed = std::min(lImageWidth / dImageRedW, lImageHeight / dImageRedH);
	if (lRed > 1)
	{
		lImageWidth  /= lRed;
		lImageHeight /= lRed;
	}

	CPdfFile* pPdf = (CPdfFile*)pRenderer;

	if (pPage->is_legal_photo())
	{
		BYTE* pBufferDst = new BYTE[4 * lImageHeight * lImageWidth];
		if (!pBufferDst)
			return;

		Aggplus::CImage oImage;
		oImage.Create(pBufferDst, lImageWidth, lImageHeight, 4 * lImageWidth);

		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);

		BYTE* pBuffer = pBufferDst;
		for (int j = lImageHeight - 1; j >= 0; --j)
		{
			GPixel* pLine = pImage->operator [](j);

			for (int i = 0; i < lImageWidth; ++i, pBuffer += 4, ++pLine)
			{
				pBuffer[0] = pLine->b;
				pBuffer[1] = pLine->g;
				pBuffer[2] = pLine->r;
				pBuffer[3] = 255;
			}
		}
		pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dWidth, dHeight);
	}
	else if (pPage->is_legal_compound())
	{
		GP<IW44Image> pIW44Image = pPage->get_bg44();
		if (nullptr != pIW44Image)
		{
			GP<GPixmap> pBgImage = pIW44Image->get_pixmap();
			if (nullptr != pBgImage)
			{
				Aggplus::CImage oImage;
				NSDjvu::DrawPixmap(pBgImage, &oImage);
				pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dWidth, dHeight);
			}
		}
		else
		{
			GP<GPixmap> pBgImage = pPage->get_bgpm();
			if (nullptr != pBgImage)
			{
				Aggplus::CImage oImage;
				NSDjvu::DrawPixmap(pBgImage, &oImage);
				pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dWidth, dHeight);
			}
		}
		
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_fgpm();
		if (nullptr == pImage)
			pImage = pPage->get_fg_pixmap(oRectAll, oRectAll);

		if (nullptr != pImage)
		{
			Aggplus::CImage oImage;
			NSDjvu::DrawPixmap(pImage, &oImage);

			GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);

			NSImages::CPixJbig2 oPix;
			if (oPix.Create(lImageWidth, lImageHeight, 1))
			{
				for (int nY = 0; nY < lImageHeight; nY++)
				{
					BYTE* pLine = pBitmap->operator [](nY);
					for (int nX = 0; nX < lImageWidth; nX++, pLine++)
					{
						oPix.SetPixel(nX, lImageHeight - 1 - nY, *pLine);
					}
				}

				pPdf->DrawImageWith1bppMask((IGrObject*)&oImage, &oPix, lImageWidth, lImageHeight, 0, 0, dWidth, dHeight);
				oPix.Destroy();
			}
		}
	}
	else if (pPage->is_legal_bilevel())
	{
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);

		NSImages::CPixJbig2 oPix;
		if (oPix.Create(lImageWidth, lImageHeight, 1))
		{
			for (int nY = 0; nY < lImageHeight; nY++)
			{
				BYTE* pLine = pBitmap->operator [](nY);
				for (int nX = 0; nX < lImageWidth; nX++, pLine++)
				{
					oPix.SetPixel(nX, lImageHeight - 1 - nY, *pLine);
				}
			}

			pPdf->DrawImage1bpp(&oPix, lImageWidth, lImageHeight, 0, 0, dWidth, dHeight);
			oPix.Destroy();
		}
	}
	else
	{
		// white frame??
		//memset(pBufferDst, 0xFF, 4 * lImageWidth * lImageHeight);
		GRect oRectAll(0, 0, lImageWidth, lImageHeight);
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);

		if (nullptr != pImage)
		{
			Aggplus::CImage oImage;
			NSDjvu::DrawPixmap(pImage, &oImage);
			pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dWidth, dHeight);
		}
		else
		{
			GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);
			if (nullptr != pBitmap)
			{
				int nPaletteEntries = pBitmap->get_grays();
				if (nPaletteEntries <= 2)
				{
					NSImages::CPixJbig2 oPix;
					if (oPix.Create(lImageWidth, lImageHeight, 1))
					{
						for (int nY = 0; nY < lImageHeight; nY++)
						{
							BYTE* pLine = pBitmap->operator [](nY);
							for (int nX = 0; nX < lImageWidth; nX++, pLine++)
							{
								oPix.Create(nX, lImageHeight - 1 - nY, *pLine);
							}
						}

						pPdf->DrawImage1bpp(&oPix, lImageWidth, lImageHeight, 0, 0, dWidth, dHeight);
						oPix.Destroy();
					}
				}
				else
				{
					BYTE* pBufferDst = new BYTE[4 * lImageHeight * lImageWidth];
					if (!pBufferDst)
						return;

					Aggplus::CImage oImage;
					oImage.Create(pBufferDst, lImageWidth, lImageHeight, 4 * lImageWidth);

					unsigned int* palette = new unsigned int[nPaletteEntries];

					// Create palette for the bitmap
					int color = 0xff0000;
					int decrement = color / (nPaletteEntries - 1);
					for (int i = 0; i < nPaletteEntries; ++i)
					{
						BYTE level = (BYTE)(color >> 16);
						palette[i] = (0xFF000000 | level << 16 | level << 8 | level);
						color -= decrement;
					}

					unsigned int* pBuffer = (unsigned int*)pBufferDst;
					for (int j = lImageHeight - 1; j >= 0; --j)
					{
						BYTE* pLine = pBitmap->operator [](j);

						for (int i = 0; i < lImageWidth; ++i, ++pBuffer, ++pLine)
						{
							if (*pLine < nPaletteEntries)
							{
								*pBuffer = palette[*pLine];
							}
							else
							{
								*pBuffer = palette[0];
							}
						}
					}

					RELEASEARRAYOBJECTS(palette);
					pRenderer->DrawImage((IGrObject*)&oImage, 0, 0, dWidth, dHeight);
				}
			}
		}
	}

	pRenderer->EndCommand(c_nPageType);
}

unsigned char* CDjVuFileImplementation::ConvertToPixels(int nPageIndex, int nRasterW, int nRasterH, bool bIsFlip)
{
	try
	{
		GP<DjVuImage> pPage = m_pDoc->get_page(nPageIndex);
		//pPage->wait_for_complete_decode();
		pPage->set_rotate(0);
		return ConvertToPixels(pPage, nRasterW, nRasterH, bIsFlip, true);
	}
	catch (...)
	{
		return nullptr;
	}
}

unsigned char* CDjVuFileImplementation::ConvertToPixels(GP<DjVuImage>& pPage, int nImageW, int nImageH, bool bFlip, bool bIsSwapRGB)
{
	BYTE* pBufferDst = nullptr;

	auto processPixmap = [&](GP<GPixmap> pImage, bool bFlip = false, bool bIsSwapRGB = false)
	{
		pBufferDst = new BYTE[4 * static_cast<size_t>(nImageW) * nImageH];

		unsigned int* pBuffer = (unsigned int*)pBufferDst;
		for (int j = 0; j < nImageH; ++j)
		{
			int nRow = bFlip ? j : (nImageH - 1 - j);
			GPixel* pLine = pImage->operator[](nRow);
			for (int i = 0; i < nImageW; ++i)
			{
				if (bIsSwapRGB)
					*pBuffer++ = 0xFF000000 | pLine->b << 16 | pLine->g << 8 | pLine->r;
				else
					*pBuffer++ = 0xFF000000 | pLine->r << 16 | pLine->g << 8 | pLine->b;
				++pLine;
			}
		}
	};

	auto processBitmap = [&](GP<GBitmap> pBitmap, bool bFlip = false)
	{
		pBufferDst = new BYTE[4 * static_cast<size_t>(nImageW) * nImageH];

		int nPaletteEntries = pBitmap->get_grays();
		unsigned int* palette = new unsigned int[nPaletteEntries];
		int color = 0xFF0000;
		int decrement = color / (nPaletteEntries - 1);
		for (int i = 0; i < nPaletteEntries; ++i)
		{
			BYTE level = (BYTE)(color >> 16);
			palette[i] = (0xFF000000 | level << 16 | level << 8 | level);
			color -= decrement;
		}

		unsigned int* pBuffer = (unsigned int*)pBufferDst;
		for (int j = 0; j < nImageH; ++j)
		{
			int nRow = bFlip ? j : (nImageH - 1 - j);
			BYTE* pLine = pBitmap->operator[](nRow);
			for (int i = 0; i < nImageW; ++i)
			{
				*pBuffer++ = palette[*pLine < nPaletteEntries ? *pLine : 0];
				++pLine;
			}
		}

		RELEASEARRAYOBJECTS(palette);
	};

	GRect oRectAll(0, 0, nImageW, nImageH);

	if (pPage->is_legal_photo() || pPage->is_legal_compound())
	{
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);
		processPixmap(pImage, bFlip, bIsSwapRGB);
	}
	else if (pPage->is_legal_bilevel())
	{
		GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);
		processBitmap(pBitmap, bFlip);
	}
	else
	{
		GP<GPixmap> pImage = pPage->get_pixmap(oRectAll, oRectAll);
		if (pImage)
		{
			processPixmap(pImage, bFlip, bIsSwapRGB);
		}
		else
		{
			GP<GBitmap> pBitmap = pPage->get_bitmap(oRectAll, oRectAll, 4);
			if (pBitmap)
			{
				processBitmap(pBitmap, bFlip);
			}
		}
	}

	return pBufferDst;
}

XmlUtils::CXmlNode CDjVuFileImplementation::ParseText(GP<DjVuImage> pPage)
{
	XmlUtils::CXmlNode paragraph;
	const GP<DjVuText> text(DjVuText::create());
	const GP<ByteStream> text_str(pPage->get_text());
	if (text_str)
	{
		text->decode(text_str);
		GUTF8String pageText = text->get_xmlText(pPage->get_height());
		XmlUtils::CXmlNode hiddenText;
		XmlUtils::CXmlNode pageColumn;
		XmlUtils::CXmlNode region;
		std::string sPageText = NSDjvu::MakeCString(pageText);
		if (!hiddenText.FromXmlStringA(sPageText))
		{
			std::wstring sPageTextW = UTF8_TO_U(sPageText);
			hiddenText.FromXmlString(sPageTextW);
		}
		hiddenText.GetNode(L"PAGECOLUMN", pageColumn);
		pageColumn.GetNode(L"REGION", region);
		region.GetNode(L"PARAGRAPH", paragraph);
	}
	return paragraph;
}
void CDjVuFileImplementation::TextToRenderer(IRenderer* pRenderer, XmlUtils::CXmlNode oTextNode, double dKoef, bool /*isView*/)
{
	// Set the brush to null (to stretch it across the entire rect)
	pRenderer->put_FontName(L"DjvuEmptyFont");
	//std::wstring csText = oTextNode.GetXml();
	std::vector<XmlUtils::CXmlNode> oLinesNodes = oTextNode.GetNodes(L"LINE");
	for (size_t nLineIndex = 0; nLineIndex < oLinesNodes.size(); ++nLineIndex)
	{
		XmlUtils::CXmlNode& oLineNode = oLinesNodes[nLineIndex];

		std::vector<XmlUtils::CXmlNode> oWordsNodes = oLineNode.GetNodes(L"WORD");
		for (size_t nWordIndex = 0; nWordIndex < oWordsNodes.size(); ++nWordIndex)
		{
			XmlUtils::CXmlNode& oWordNode = oWordsNodes[nWordIndex];

			std::wstring csWord   = oWordNode.GetText();
			std::wstring csCoords = oWordNode.GetAttribute(L"coords");

			double arrCoords[4] = {0, 0, 0, 0};
			ParseCoords(csCoords, arrCoords, dKoef);
			DrawPageText(pRenderer, arrCoords, csWord);
		}
	}
}
void CDjVuFileImplementation::DrawPageText(IRenderer* pRenderer, double* pdCoords, const std::wstring& wsText)
{
	pRenderer->put_FontSize(pdCoords[1] - pdCoords[3]);
	pRenderer->CommandDrawText(wsText,
							   (float)(pdCoords[0]),
			(float)(pdCoords[3]),
			(float)(pdCoords[2] - pdCoords[0]),
			(float)(pdCoords[1] - pdCoords[3]));
}
void CDjVuFileImplementation::ParseCoords(const std::wstring& wsCoordsStr, double* pdCoords, double dKoef)
{
	std::vector<std::wstring> vCoords = NSStringExt::Split(wsCoordsStr, L',');
	if (vCoords.size() >= 4)
	{
		for (int nIndex = 0; nIndex < 4; nIndex++)
		{
			pdCoords[nIndex] = NSDjvu::GetInteger(vCoords.at(nIndex)) * dKoef;
		}
	}
}
