#ifndef OOXMLTAGS_H
#define OOXMLTAGS_H

#include "HTMLTags.h"
#include "../Writers/OOXMLWriter.h"
#include "../Table.h"

#include <unordered_map>

namespace HTML
{
#define CREATE_OOXML_TAG(class_name) CREATE_TAG(class_name##OOXML, COOXMLWriter)

CREATE_OOXML_TAG(Anchor);
CREATE_OOXML_TAG(Abbr);
CREATE_OOXML_TAG(Break);
CREATE_OOXML_TAG(Font);
CREATE_OOXML_TAG(Input);
CREATE_OOXML_TAG(BaseFont);
CREATE_OOXML_TAG(List);
CREATE_OOXML_TAG(ListElement);
CREATE_OOXML_TAG(HTML);

class CDivisionOOXMLTag : public IHTMLTag<COOXMLWriter>
{
	std::stack<UINT> m_arFootnoteIDs;
public:
	CDivisionOOXMLTag(COOXMLWriter *pWriter);

	bool Open (const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

class CImageOOXMLTag : public IHTMLTag<COOXMLWriter>
{
	std::vector<std::wstring> m_arImages;
public:
	CImageOOXMLTag(COOXMLWriter *pWriter);

	bool Open (const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

class CBlockquoteOOXMLTag : public IHTMLTag<COOXMLWriter>
{
	std::map<std::wstring, UINT> m_mDivs;
public:
	CBlockquoteOOXMLTag(COOXMLWriter *pWriter);

	bool Open (const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

class CHorizontalRuleOOXMLTag : public IHTMLTag<COOXMLWriter>
{
	UINT m_unShapeId;
public:
	CHorizontalRuleOOXMLTag(COOXMLWriter *pWriter);

	bool Open (const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

void InitTagsForOOXML(std::map<int, std::shared_ptr<ITag>>& mTags, COOXMLWriter* pWriter);

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
	COOXMLTable(TExternalTableData &oExternalData);
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

	void OpenCell(XmlString& oXmlString);
	void OpenCell(XmlString& oXmlString, const NSCSS::CNode& oCellNode, ITableElementCell* pCell, size_t unColumnIndex, ERowParseMode eRowParseMode, ERowPosition eRowPosition, size_t& unHeight, const TTableStyles& oTableStyles, std::unordered_map<size_t, NSCSS::CNode>& mFillingColumn);
	void CloseCell(XmlString& oXmlString);

	void ConvertTable(XmlUtils::CXmlLiteReader& oReader, COOXMLWriter& oWriter, const COOXMLTable& oTable, const NSCSS::CNode& oTableNode);
	void ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, COOXMLWriter& oWriter, std::vector<NSCSS::CNode>& arSelectors, std::stack<int>& arDepths, const Table& oMatrix, TTableStyles& oTableStyles, ERowParseMode eRowParseMode);
};
}

#endif // OOXMLTAGS_H
