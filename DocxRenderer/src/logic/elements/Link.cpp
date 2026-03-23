#include "Link.h"

namespace NSDocxRenderer {
CLink::~CLink()
{
	Clear();
}

void CLink::Clear()
{
	m_wsUri.clear();
	m_nId = 0;
}

void CLink::AddLink(const UINT& nId, const std::wstring& wsUri)
{
	m_nId = nId;
	m_wsUri = wsUri;
}
}
