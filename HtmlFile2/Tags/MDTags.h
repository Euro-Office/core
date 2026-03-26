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
bool CBoldTag<CMDWriter>::Open();
template<>
void CBoldTag<CMDWriter>::Close();

template<>
bool CBreakTag<CMDWriter>::Read(const NSCSS::CNode& oTagNode);

template<>
bool CItalicTag<CMDWriter>::Open();
template<>
void CItalicTag<CMDWriter>::Close();

template<>
bool CStrikeTag<CMDWriter>::Open();
template<>
void CStrikeTag<CMDWriter>::Close();

template<>
bool CPreformattedTag<CMDWriter>::Open();
template<>
void CPreformattedTag<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors);

template<>
bool CHeaderTag<CMDWriter>::Read(const NSCSS::CNode& oTagNode);

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
	size_t unRows;
	size_t unColumns;
};

class CMarkdownTable : public CTableElement
{
public:
	CMarkdownTable(TExternalTableData* pExternalData);
	virtual ~CMarkdownTable();

	bool PreParse(XmlUtils::CXmlLiteReader& oReader) override;
	void Normalize() override;
	bool Convert(XmlUtils::CXmlLiteReader& oReader, const NSCSS::CNode& oTableNode) override;
private:
	bool ParseCaption(XmlUtils::CXmlLiteReader& oReader, XmlString*& pCaption) override;
	bool ParseColgroup(XmlUtils::CXmlLiteReader& oReader, std::vector<CTableColgroup*>& arColgroups) override;

	bool ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, const Table& oMatrix, CMDWriter* pWriter);

	static Table Flatten(Table&& srcTable);
	static TElementInfo ComputeInfo(const ITableElementCell* pCell);
};
}

#endif // MDTAGS_H
