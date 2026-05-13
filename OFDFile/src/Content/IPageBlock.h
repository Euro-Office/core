#ifndef IPAGEBLOCK_H
#define IPAGEBLOCK_H

#include "../IOFDElement.h"
#include "../Types/CommonData.h"

class IRenderer;
class IFolder;

#ifdef BUILDING_WASM_MODULE
namespace NSWasm { class CData; }
#endif

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

	#ifdef BUILDING_WASM_MODULE
	virtual void GetLinks(NSWasm::CData& oRes) const = 0;
	#endif
};
}

#endif // IPAGEBLOCK_H
