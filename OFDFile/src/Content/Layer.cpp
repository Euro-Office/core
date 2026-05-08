#include "Layer.h"

#include "PageBlock.h"

namespace OFD
{
CLayer::CLayer(CXmlReader& oLiteReader, IFolder* pFolder)
	: IPageBlock(oLiteReader), m_eType(EType::Body)
{
	if (L"ofd:Layer" != oLiteReader.GetName())
		return;

	CPageBlock::ReadIntoContainer(oLiteReader, m_arPageBlocks, pFolder);
}

CLayer::~CLayer()
{
	for (IPageBlock* pPageBlock : m_arPageBlocks)
		delete pPageBlock;
}

void CLayer::Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const
{
	if (nullptr == pRenderer)
		return;

	for (const IPageBlock* pPageBlock : m_arPageBlocks)
		pPageBlock->Draw(pRenderer, oCommonData, ePageType);
}

#ifdef BUILDING_WASM_MODULE
void CLayer::GetLinks(NSWasm::CData& oRes) const
{
	for (const IPageBlock* pPageBlock : m_arPageBlocks)
		pPageBlock->GetLinks(oRes);
}
#endif

}
