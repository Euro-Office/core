#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "Utils/CFontChecker.h"
#include "Page.h"
#include "Annotation.h"
#include "OutlineElem.h"

namespace NSWasm { class CData; }

namespace OFD
{
class CPermission
{
	bool m_bEdit;
	bool m_bAnnot;
	bool m_bExport;
	bool m_bSignature;
	bool m_bWatermark;
	bool m_bPrintScreen;
public:
	CPermission();

	bool Read(CXmlReader& oLiteReader);
};

class CDocument
{
	CCommonData m_oCommonData;
	CPermission m_oPermission;
	CAnnotation m_oAnnotation;

	std::map<unsigned int, const CPage*> m_mPages;
	std::vector<const COutlineElem*> m_arOutlines;
public:
	CDocument();
	~CDocument();

	bool Empty() const;

	bool Read(const std::wstring& wsFilePath, IFolder* pFolder);

	bool DrawPage(IRenderer* pRenderer, int nPageIndex) const;

	unsigned int GetPageCount() const;
	bool GetPageSize(int nPageIndex, double& dWidth, double &dHeight) const;

	void UpdateFonts(CFontChecker* pFontChecker);

	#ifdef BUILDING_WASM_MODULE
	void GetStructure(UINT& unMaxNumberPage, NSWasm::CData& oRes) const;
	#endif
private:
	void AddOutlineElem(const COutlineElem* pOutlineElem);
};
}

#endif // DOCUMENT_H
