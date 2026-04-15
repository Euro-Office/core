#ifndef MDTAGS_H
#define MDTAGS_H

#include "HTMLTags.h"
#include "../Writers/MDWriter.h"
#include "../Table.h"

namespace HTML
{
template<>
bool CAnchorTag<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors);
template<>
void CAnchorTag<CMDWriter>::Close(const NSCSS::CNode& oTagNode);

template<>
bool CBreakTag<CMDWriter>::Read(const NSCSS::CNode& oTagNode);

template<>
bool CPreformattedTag<CMDWriter>::Open();
template<>
void CPreformattedTag<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors);

template<>
bool CHeaderTag<CMDWriter>::Open(const NSCSS::CNode& oTagNode);
template<>
void CHeaderTag<CMDWriter>::Close();

template<>
bool CImageTag<CMDWriter>::Read(const std::vector<NSCSS::CNode>& arSelectors);
template<>
bool CImageTag<CMDWriter>::ReadSVG(const std::vector<NSCSS::CNode>& arSelectors, const std::wstring& wsSVG);

template<>
bool CBlockquoteTag<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors);
template<>
void CBlockquoteTag<CMDWriter>::Close();

template<>
bool CListTag<CMDWriter>::Open(const NSCSS::CNode& oTagNode);
template<>
void CListTag<CMDWriter>::Close();

template<>
bool CHorizontalRuleTag<CMDWriter>::Write(const std::vector<NSCSS::CNode>& arSelectors);

template<>
bool CListElementTag<CMDWriter>::Open();
template<>
void CListElementTag<CMDWriter>::Close();

template<>
bool CCodeTag<CMDWriter>::Open(const NSCSS::CNode& oTagNode);
template<>
void CCodeTag<CMDWriter>::Close();

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
