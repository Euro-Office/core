#ifndef IPAGEBLOCK_H
#define IPAGEBLOCK_H

#include "../IOFDElement.h"
#include "../Types/CommonData.h"

class IRenderer;
class IFolder;

namespace OFD
{
enum class EPageType
{
	Page,
	TemplatePage,
	Anotation
};

class IPageBlock : public IOFDElement
{
public:
	IPageBlock(CXmlReader& oLiteReader)
		: IOFDElement(oLiteReader){};
	virtual ~IPageBlock(){};
	virtual void Draw(IRenderer* pRenderer, const CCommonData& oCommonData, EPageType ePageType) const = 0;
};
}

#endif // IPAGEBLOCK_H
