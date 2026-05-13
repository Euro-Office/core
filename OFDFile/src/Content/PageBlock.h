#ifndef PAGEBLOCK_H
#define PAGEBLOCK_H

#include "IPageBlock.h"

namespace OFD
{
class CPageBlock : public IPageBlock
{
	TBox m_oBoundary;

	std::vector<IPageBlock*> m_arPageBlocks;
public:
	CPageBlock(CXmlReader& oLiteReader, IFolder *pFolder);

	static void ReadIntoContainer(CXmlReader& oLiteReader, std::vector<IPageBlock*>& arPageBlocks, IFolder* pFolder);

	void Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const override;

	#ifdef BUILDING_WASM_MODULE
	virtual void GetLinks(NSWasm::CData& oRes) const override;
	#endif
};
}

#endif // PAGEBLOCK_H
