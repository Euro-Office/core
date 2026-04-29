#include "Font.h"

#include "../Utils/Utils.h"
#include "../Utils/CFontChecker.h"

#include "../../../DesktopEditor/graphics/IRenderer.h"
#include "../../../OfficeUtils/src/ZipFolder.h"

namespace OFD
{
CFont::CFont(CXmlReader& oXmlReader, const std::wstring& wsRootPath, IFolder* pFolder)
	: IOFDElement(oXmlReader),
	  m_wsCharset(L"unicode"), m_bItalic(false), m_bBold(false),
	  m_bSerif(false), m_bFixedWidth(false)
	#ifdef BUILDING_WASM_MODULE
	 , m_pFontManager(nullptr)
	#endif
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

		#ifndef FONTS_USE_ONLY_MEMORY_STREAMS
		if (nullptr != pFolder && IFolder::IFolderType::iftZip == pFolder->getType())
			m_bSupportExternalFont = false;
		#endif
	}
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
	if (m_wsSelectedFont.empty())
		return;
	// put_FontName cannot be called, otherwise pRenderer will have uncontrolled font selection
	pRenderer->put_FontPath(m_wsSelectedFont); // The font has been added to the GlobalMemoryStorage fonts, so it can be put
#else
	pRenderer->put_FontName(m_wsFontName);
	if (m_bSupportExternalFont && !m_wsFilePath.empty())
		pRenderer->put_FontPath(m_wsFilePath);
#endif
}

#ifdef BUILDING_WASM_MODULE
NSFonts::IFontManager* CFont::GetFontManager() const
{
	return m_pFontManager;
}
#endif
}
