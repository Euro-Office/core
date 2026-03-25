#include "Link.h"

namespace NSDocxRenderer
{
	CLink::~CLink()
	{
		Clear();
	}

	void CLink::Clear()
	{
		m_nId = 0;
		m_eType = eLinkType::ltNone;
		m_wsData.clear();
	}

	void CLink::AddLink(BYTE nId, const LONG& lType, const std::wstring& wsData)
	{
		m_nId = nId;
		m_eType = static_cast<eLinkType>(lType);
		m_wsData = wsData;
	}

	void CLink::AddBBox(const double& x1, const double& y1, const double& x2, const double& y2)
	{
		m_dTop = y1;
		m_dLeft = x1;
		m_dBottom = y2;
		m_dRight = x2;
	}
}
