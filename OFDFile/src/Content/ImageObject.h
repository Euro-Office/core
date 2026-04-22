#ifndef IMAGEOBJECT_H
#define IMAGEOBJECT_H

#include "IPageBlock.h"
#include "GraphicUnit.h"

class IFolder;

namespace OFD
{
class CImageObject : public IPageBlock, public CGraphicUnit
{
	unsigned int m_unMultiMediaID;
	IFolder*     m_pFolder;
public:
	CImageObject(CXmlReader& oLiteReader, IFolder* pFolder);

	void Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const override;
};
}

#endif // IMAGEOBJECT_H
