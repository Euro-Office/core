#ifndef MDTAGS_H
#define MDTAGS_H

#include "HTMLTags.h"
#include "../Writers/MDWriter.h"
#include "../Table.h"

#include <map>

namespace HTML
{
#define CREATE_MD_TAG(class_name) CREATE_TAG(class_name##MD, CMDWriter)

CREATE_MD_TAG(Anchor);
CREATE_MD_TAG(Break);
CREATE_MD_TAG(Preformatted);
CREATE_MD_TAG(Header);
CREATE_MD_TAG(Image);
CREATE_MD_TAG(HorizontalRule);
CREATE_MD_TAG(Blockquote);
CREATE_MD_TAG(List);
CREATE_MD_TAG(ListElement);
CREATE_MD_TAG(Code);

void InitTagsForMD(std::map<int, std::shared_ptr<ITag>>& mTags, CMDWriter* pWriter);

struct TElementInfo
{
	size_t m_unRows;
	size_t m_unColumns;

	TElementInfo()
		: m_unRows{0}, m_unColumns{0}
	{}

	TElementInfo(const size_t& unRows, const size_t& m_unColumns)
		: m_unRows{unRows}, m_unColumns{m_unColumns}
	{}
};

class CMarkdownTable : public CTableElement
{
public:
	CMarkdownTable(TExternalTableData &oExternalData);
	virtual ~CMarkdownTable();

	bool PreParse(XmlUtils::CXmlLiteReader& oReader) override;
	void Normalize() override;
	bool Convert(XmlUtils::CXmlLiteReader& oReader, const NSCSS::CNode& oTableNode) override;
private:
	bool ParseCaption(XmlUtils::CXmlLiteReader& oReader, XmlString*& pCaption) override;
	bool ParseColgroup(XmlUtils::CXmlLiteReader& oReader, std::vector<CTableColgroup*>& arColgroups) override;

	bool ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, const Table& oMatrix, CMDWriter* pWriter, bool bIsHeader = false);

	static Table Flatten(Table&& srcTable);
	static TElementInfo ComputeInfo(const ITableElementCell* pCell);
	static TElementInfo ComputeInfo(const CTableElement* pTable);
};
}

#endif // MDTAGS_H
