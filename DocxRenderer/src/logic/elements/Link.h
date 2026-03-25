#pragma once
#include "BaseItem.h"

namespace NSDocxRenderer
{
	enum class eLinkType
	{
		ltNone = 0,
		ltGoTo = 1,
		ltUri = 6,
		ltNamed = 9,
	};

	class CLink : public CBaseItem
	{
	public:
		CLink() = default;
		virtual ~CLink();
		void Clear();

		void AddLink(BYTE nId, const LONG& type, const std::wstring& wsData);
		void AddBBox(const double& x1, const double& y1, const double& x2, const double& y2);
	public:
		UINT m_nId{0};
		eLinkType m_eType{eLinkType::ltNone};

		double m_dTop{0.0};
		double m_dLeft{0.0};
		double m_dBottom{0.0};
		double m_dRight{0.0};

		std::wstring m_wsData{};
	};
}
