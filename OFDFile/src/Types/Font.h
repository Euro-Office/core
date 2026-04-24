#ifndef FONT_H
#define FONT_H

#include "../IOFDElement.h"

class IFolder;
class IRenderer;

namespace NSFonts { class IFontManager; }

namespace OFD
{
class CFont : public IOFDElement
{
public:
	CFont(CXmlReader& oXmlReader, const std::wstring& wsRootPath, IFolder *pFolder, NSFonts::IFontManager* pFontManager);

	void Apply(IRenderer* pRenderer) const;
private:
	std::wstring m_wsFontName;
	std::wstring m_wsFamilyName;
	std::wstring m_wsCharset;
	bool         m_bItalic;
	bool         m_bBold;
	bool         m_bSerif;
	bool         m_bFixedWidth;

	std::wstring m_wsFilePath;

	bool         m_bSupportExternalFont;
};
}

#endif // FONT_H
