#ifndef TEMPLATEPAGE_H
#define TEMPLATEPAGE_H

#include "../IOFDElement.h"

class IFolder;

namespace OFD
{
enum class EZOrder
{
	Body,
	Background
};

EZOrder GetZOrderFromString(const std::string& sValue);

class CPage;
class CTemplatePage : public IOFDElement
{
	EZOrder m_eZOrder;

	const CPage* m_pPage;
public:
	CTemplatePage(CXmlReader& oXmlReader, const std::wstring& wsRootPath, IFolder* pFolder);
	~CTemplatePage();

	EZOrder GetZOrder() const;
	const CPage* GetPage() const;
};
}

#endif // TEMPLATEPAGE_H
