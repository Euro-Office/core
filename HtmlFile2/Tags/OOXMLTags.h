#ifndef OOXMLTAGS_H
#define OOXMLTAGS_H

#include "HTMLTags.h"
#include "../Writers/OOXMLWriter.h"
#include "../Table.h"

namespace HTML
{
template<>
class CAnchor<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CAnchor(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CAbbr<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CAbbr(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBreak<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CBreak(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CDivision<COOXMLWriter> : public CTag<COOXMLWriter>
{
	std::stack<UINT> m_arFootnoteIDs;
public:
	CDivision(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CImage<COOXMLWriter> : public CTag<COOXMLWriter>
{
	std::vector<std::wstring> m_arrImages;
public:
	CImage(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CFont<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CFont(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CInput<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CInput(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBaseFont<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CBaseFont(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBlockquote<COOXMLWriter> : public CTag<COOXMLWriter>
{
	std::map<std::wstring, UINT>  m_mDivs;
public:
	CBlockquote(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHorizontalRule<COOXMLWriter> : public CTag<COOXMLWriter>
{
	UINT m_unShapeId;
public:
	CHorizontalRule(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CList<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CList(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CListElement<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CListElement(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CCaption<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CCaption(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHTML<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CHTML(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
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
	void OpenCell(XmlString& oXmlString, const NSCSS::CNode& oCellNode, CTableElementCell* pCell, size_t unColumnIndex, ERowParseMode eRowParseMode, ERowPosition eRowPosition, size_t& unHeight, const TTableStyles& oTableStyles);
	void CloseCell(XmlString& oXmlString);

	void ConvertTable(XmlUtils::CXmlLiteReader& oReader, COOXMLWriter& oWriter, const COOXMLTable& oTable, const NSCSS::CNode& oTableNode);
	void ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, COOXMLWriter& oWriter, std::vector<NSCSS::CNode>& arSelectors, std::stack<int>& arDepths, const Table& oMatrix, TTableStyles& oTableStyles, ERowParseMode eRowParseMode);
};
}

#endif // OOXMLTAGS_H
