#ifndef CONTENT_H
#define CONTENT_H

#include "Layer.h"
#include "../Types/CommonData.h"

namespace OFD
{
class CContent
{
	std::vector<const CLayer*> m_arLayers;
public:
	CContent();
	~CContent();

	bool Read(CXmlReader& oLiteReader, IFolder* pFolder);
	void Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const;

	#ifdef BUILDING_WASM_MODULE
	void GetLinks(NSWasm::CData& oRes) const;
	#endif
};
}

#endif // CONTENT_H
