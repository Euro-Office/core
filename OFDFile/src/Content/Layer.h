#ifndef LAYER_H
#define LAYER_H

#include "IPageBlock.h"

namespace OFD
{
class CLayer : public IPageBlock
{
	enum class EType
	{
		Body,
		Foreground,
		Background
	} m_eType;

	unsigned int m_unID;
	std::vector<IPageBlock*> m_arPageBlocks;
public:
	CLayer(CXmlReader& oLiteReader, IFolder* pFolder);
	~CLayer();

	void Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const override;

	#ifdef BUILDING_WASM_MODULE
	void GetLinks(NSWasm::CData& oRes) const override;
	#endif
};
}

#endif // LAYER_H
