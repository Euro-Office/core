#ifndef OOXMLTAGS_H
#define OOXMLTAGS_H

#include "HTMLTags.h"
#include "../Writers/OOXMLWriter.h"
#include "../Table.h"

namespace HTML
{
template<>
bool CAnchorTag<COOXMLWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors);
template<>
void CAnchorTag<COOXMLWriter>::Close(const NSCSS::CNode& oTagNode);

template<>
bool CAbbrTag<COOXMLWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors);
template<>
void CAbbrTag<COOXMLWriter>::Close();

template<>
bool CBreakTag<COOXMLWriter>::Read(const NSCSS::CNode& oTagNode);

template<>
bool CFontTag<COOXMLWriter>::Apply(const NSCSS::CNode& oTagNode, size_t unLevel);

template<>
bool CInputTag<COOXMLWriter>::Read(const std::vector<NSCSS::CNode>& arSelectors);

template<>
bool CBaseFontTag<COOXMLWriter>::Apply(const NSCSS::CNode& oTagNode);

template<>
bool CListTag<COOXMLWriter>::Open(const NSCSS::CNode& oTagNode);
template<>
void CListTag<COOXMLWriter>::Close();

template<>
bool CListElementTag<COOXMLWriter>::Open();
template<>
void CListElementTag<COOXMLWriter>::Close();

template<>
bool CHTMLTag<COOXMLWriter>::Apply(const NSCSS::CNode& oTagNode);

#define CLASS_TAG_LIGHT(class_name)\
template<>\
class C ## class_name ## Tag<COOXMLWriter> : public INTERFACE_TAGS::I ## class_name ## Tag, public INTERFACE_TAGS::ITag<COOXMLWriter>

CLASS_TAG_LIGHT(Division)
{
	std::stack<UINT> m_arFootnoteIDs;
public:
	CDivisionTag(COOXMLWriter* pWriter);
	bool Open(const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close() override;
};

CLASS_TAG_LIGHT(Image)
{
	std::vector<std::wstring> m_arrImages;
public:
	CImageTag(COOXMLWriter* pWriter);
	bool Read(const std::vector<NSCSS::CNode>& arSelectors) override;
	bool ReadSVG(const std::vector<NSCSS::CNode>& arSelectors, const std::wstring& wsSVG) override;
};

CLASS_TAG_LIGHT(Blockquote)
{
	std::map<std::wstring, UINT>  m_mDivs;
public:
	CBlockquoteTag(COOXMLWriter* pWriter);
	bool Open(const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close() override;
};

CLASS_TAG_LIGHT(HorizontalRule)
{
	UINT m_unShapeId;
public:
	CHorizontalRuleTag(COOXMLWriter* pWriter);
	bool Write(const std::vector<NSCSS::CNode>& arSelectors) override;
};

enum class ETableRules
{
	All,
	Groups,
	Cols,
	None,
	Rows
};

class COOXMLTable : public CTableElement
{
public:
	COOXMLTable(TExternalTableData* pExternalData);
	virtual ~COOXMLTable();

	bool PreParse(XmlUtils::CXmlLiteReader& oReader) override;
	void Normalize() override;
	bool Convert(XmlUtils::CXmlLiteReader& oReader, const NSCSS::CNode& oTableNode) override;
private:
	bool ParseCaption(XmlUtils::CXmlLiteReader& oReader, XmlString*& pCaption) override;
	bool ParseColgroup(XmlUtils::CXmlLiteReader& oReader, std::vector<CTableColgroup*>& arColgroups) override;

	bool HaveColgroups() const;
	size_t GetColumnsCount() const;

	struct TTableStyles
	{
		size_t m_unCellSpacing;
		ETableRules m_enRules;

		const COOXMLTable *m_pTable;

		NSCSS::NSProperties::CBorder m_oBorder;
		NSCSS::NSProperties::CIndent m_oPadding;

		TTableStyles()
		    : m_unCellSpacing{0}, m_enRules{ETableRules::All}, m_pTable{nullptr}
		{}
	};

	enum class ERowParseMode
	{
		Header,
		Body,
		Foother
	};

	enum class ERowPosition
	{
		First,
		Middle,
		Last
	};

	void OpenTable(XmlString& oXmlString, const NSCSS::CNode& oTableNode, TTableStyles& oTableStyles, const COOXMLTable& oTable);
	void CloseTable(XmlString& oXmlString);

	void OpenRow(XmlString& oXmlString, bool bIsHeader, size_t unMaxHeight, size_t unCellSpacing);
	void CloseRow(XmlString& oXmlString);

	void OpenCell(XmlString& oXmlString, size_t unColumnIndex);
	void OpenCell(XmlString& oXmlString, const NSCSS::CNode& oCellNode, CTableElementCell* pCell, size_t unColumnIndex, ERowParseMode eRowParseMode, ERowPosition eRowPosition, size_t& unHeight, const TTableStyles& oTableStyles);
	void CloseCell(XmlString& oXmlString);

	void ConvertTable(XmlUtils::CXmlLiteReader& oReader, COOXMLWriter& oWriter, const COOXMLTable& oTable, const NSCSS::CNode& oTableNode);
	void ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, COOXMLWriter& oWriter, std::vector<NSCSS::CNode>& arSelectors, std::stack<int>& arDepths, const Table& oMatrix, TTableStyles& oTableStyles, ERowParseMode eRowParseMode);
};
}

#endif // OOXMLTAGS_H
