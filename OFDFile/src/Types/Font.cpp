#include "Font.h"

#include "../Utils/Utils.h"
#include "../../../OfficeUtils/src/ZipFolder.h"
#include "../../../DesktopEditor/graphics/IRenderer.h"

#ifdef BUILDING_WASM_MODULE
#include "../../../DesktopEditor/graphics/pro/Fonts.h"
#include "../../../PdfFile/SrcReader/FontsWasm.h"
#define FONTS_USE_ONLY_MEMORY_STREAMS
#endif

namespace OFD
{
CFont::CFont(CXmlReader& oXmlReader, const std::wstring& wsRootPath, IFolder* pFolder)
	: IOFDElement(oXmlReader),
	  m_wsCharset(L"unicode"), m_bItalic(false), m_bBold(false),
	  m_bSerif(false), m_bFixedWidth(false), m_bSupportExternalFont(true)
{
	if (0 != oXmlReader.GetAttributesCount() && oXmlReader.MoveToFirstAttribute())
	{
		std::string sArgumentName;

		do
		{
			sArgumentName = oXmlReader.GetNameA();

			if ("FontName" == sArgumentName)
				m_wsFontName = oXmlReader.GetText();
			else if ("FamilyName" == sArgumentName)
				m_wsFamilyName = oXmlReader.GetText();
			else if ("Charset" == sArgumentName)
				m_wsCharset = oXmlReader.GetText();
			else if ("Italic" == sArgumentName)
				m_bItalic = oXmlReader.GetBoolean(true);
			else if ("Bold" == sArgumentName)
				m_bBold = oXmlReader.GetBoolean(true);
			else if ("Serif" == sArgumentName)
				m_bSerif = oXmlReader.GetBoolean(true);
			else if ("FixedWidth" == sArgumentName)
				m_bFixedWidth = oXmlReader.GetBoolean(true);
		} while (oXmlReader.MoveToNextAttribute());
	}

	oXmlReader.MoveToElement();

	if (!oXmlReader.IsEmptyNode())
	{
		const int nDepth = oXmlReader.GetDepth();

		while (oXmlReader.ReadNextSiblingNode(nDepth))
		{
			if ("ofd:FontFile" == oXmlReader.GetNameA())
			{
				const std::wstring wsPath{oXmlReader.GetText2()};

				if (CanUseThisPath(wsPath, wsRootPath))
					m_wsFilePath = CombinePaths(wsRootPath, wsPath);

				break;
			}
		}

		if (nullptr != pFolder && IFolder::IFolderType::iftZip == pFolder->getType())
			m_bSupportExternalFont = false;
	}

#ifdef FONTS_USE_ONLY_MEMORY_STREAMS
	if (m_wsFilePath.empty())
	{
		// font selection
		NSFonts::CFontSelectFormat oFormat;
		oFormat.wsName  = new std::wstring(m_wsFontName);
		oFormat.bBold   = new INT(m_bBold   ? TRUE : FALSE);
		oFormat.bItalic = new INT(m_bItalic ? TRUE : FALSE);
		NSFonts::CFontInfo* pFontInfo = pFontManager->GetFontInfoByParams(oFormat);

		if (pFontInfo && L"" != pFontInfo->m_wsFontPath)
		{
			m_wsFilePath = pFontInfo->m_wsFontPath;

			if (NSWasm::IsJSEnv())
				m_wsFilePath = pFontInfo->m_wsFontName;

			if (!m_wsFilePath.empty())
			{
				m_wsFilePath = NSWasm::LoadFont(m_wsFilePath, pFontInfo->m_bBold, pFontInfo->m_bItalic);
				if (m_wsFilePath.empty())
				{
					// The font isn't ready yet, which means it's not being put to pRenderer at all. Later, when the font is loaded, the page will be redrawn.
					return;
				}
			}
		}
	}
	else
	{
		// embedded font
		std::wstring wsTempFileName;
		if (NSFonts::NSApplicationFontStream::GetGlobalMemoryStorage())
		{
			BYTE* pData = NULL;
			int nSize = 0;

			// TODO Get font data from m_wsFilePath in the archive

			wsTempFileName = NSFonts::NSApplicationFontStream::GetGlobalMemoryStorage()->GenerateId();
			NSFonts::NSApplicationFontStream::GetGlobalMemoryStorage()->Add(wsTempFileName, pData, (LONG)nSize, true);
			m_wsFilePath = wsTempFileName;

			RELEASEARRAYOBJECTS(pData);
		}
	}
#endif
}

void CFont::Apply(IRenderer* pRenderer) const
{
	if (nullptr == pRenderer)
		return;

	int nFontStyle = 0;

	if (m_bBold)
		nFontStyle |= 0x01;
	if (m_bItalic)
		nFontStyle |= 0x02;

	pRenderer->put_FontStyle(nFontStyle);

#ifdef FONTS_USE_ONLY_MEMORY_STREAMS
	// put_FontName cannot be called, otherwise pRenderer will have uncontrolled font selection
	pRenderer->put_FontPath(m_wsFilePath); // The font has been added to the GlobalMemoryStorage fonts, so it can be put
#else
	pRenderer->put_FontName(m_wsFontName);
	if (m_bSupportExternalFont && !m_wsFilePath.empty())
		pRenderer->put_FontPath(m_wsFilePath);
#endif

}
}
