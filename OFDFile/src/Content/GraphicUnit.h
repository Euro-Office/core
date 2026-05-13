#ifndef GRAPHICUNIT_H
#define GRAPHICUNIT_H

#include "../Types/PenSettings.h"
#include "../Utils/Types.h"
#include "../Action.h"

#ifdef BUILDING_WASM_MODULE
namespace NSWasm { class CData; }
#endif

namespace OFD
{
class CGraphicUnit
{
	TBox m_oBoundary;
	std::wstring m_wsName;
	bool m_bVisible;
	TMatrix m_oCTM;
	unsigned int m_unDrawParam;
	CPenSettings m_oPenSettings;

	std::vector<const CAction*> m_arActions;
public:
	CGraphicUnit(CXmlReader& oReader);

	void Apply(IRenderer* pRenderer, TMatrix& oOldTransform) const;

	void ReadChildren(CXmlReader& oReader);

	TBox GetBoundary() const;

	#ifdef BUILDING_WASM_MODULE
	void GetLinks(NSWasm::CData& oRes) const;
	#endif
private:
	void AddAction(const CAction* pAction);
};
}

#endif // GRAPHICUNIT_H
