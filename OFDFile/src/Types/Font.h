#ifndef FONT_H
#define FONT_H

#include "../IOFDElement.h"

class IFolder;
class IRenderer;

#ifdef BUILDING_WASM_MODULE
#define FONTS_USE_ONLY_MEMORY_STREAMS
#endif

namespace OFD
{
class CFont : public IOFDElement
{
public:
	CFont(CXmlReader& oXmlReader, const std::wstring& wsRootPath, IFolder *pFolder);

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

	#ifdef FONTS_USE_ONLY_MEMORY_STREAMS
	std::wstring m_wsSelectedFont;
	#else
	bool         m_bSupportExternalFont;
	#endif

	friend class CFontChecker;
};
}

#endif // FONT_H
