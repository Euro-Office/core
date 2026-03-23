#pragma once
#include "BaseItem.h"

namespace NSDocxRenderer
{
	class CLink : public CBaseItem
	{
	public:
		CLink() = default;
		virtual ~CLink();
		void Clear();

		void AddLink(const UINT& nId, const std::wstring& wsUri);
	public:
		UINT m_nId{0};
		std::wstring m_wsUri{};
	};
}
