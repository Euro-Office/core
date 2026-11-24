#include "CPolyline.h"

#include "../../../../../Common/3dParty/html/css/src/StaticFunctions.h"

namespace SVG
{
	CPolyline::CPolyline(CSvgReader& oReader, CRenderedObject* pParent)
		: CPath(oReader, pParent)
	{}

	void CPolyline::SetAttribute(const std::string& sName, CSvgReader& oReader)
	{
		if ("points" == sName)
		{
			const std::vector<double> arValues = NSCSS::NS_STATIC_FUNCTIONS::ReadDoubleValues(oReader.GetText());

			if (arValues.size() < 4)
				return;

			AddElement(new CMoveElement(Point{arValues[0], arValues[1]}));

			for (unsigned int unIndex = 2; unIndex < arValues.size(); unIndex += 2)
				AddElement(new CLineElement(Point{arValues[unIndex + 0], arValues[unIndex + 1]}));
		}
		else
			// A <polyline> has no path data: fall back to the generic attribute
			// handling instead of CPath::SetAttribute ("d" parsing).
			CRenderedObject::SetAttribute(sName, oReader); // NOLINT(bugprone-parent-virtual-call)
	}

	CPolygon::CPolygon(CSvgReader& oReader, CRenderedObject* pParent)
	    : CPolyline(oReader, pParent)
	{}

	void CPolygon::SetAttribute(const std::string& sName, CSvgReader& oReader)
	{
		CPolyline::SetAttribute(sName, oReader);
		AddElement(new CCloseElement());
	}
}
