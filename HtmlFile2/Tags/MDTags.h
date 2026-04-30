#ifndef MDTAGS_H
#define MDTAGS_H

#include "HTMLTags.h"
#include "../Writers/MDWriter.h"
#include "../Table.h"

namespace HTML
{
#define CLASS_TAG_LIGHT_MD(class_name)\
template<>\
class C ## class_name ## Tag<CMDWriter> : public INTERFACE_TAGS::I ## class_name ## Tag, public INTERFACE_TAGS::ITag<CMDWriter>

CLASS_TAG_LIGHT_MD(Anchor)
{
public:
	CAnchorTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open(const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close(const NSCSS::CNode& oTagNode) override;
};

CLASS_TAG_LIGHT_MD(Break)
{
public:
	CBreakTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Read(const NSCSS::CNode& oTagNode) override;
};

CLASS_TAG_LIGHT_MD(Preformatted)
{
public:
	CPreformattedTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open() override;
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

CLASS_TAG_LIGHT_MD(Header)
{
public:
	CHeaderTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open(const NSCSS::CNode& oTagNode) override;
	void Close() override;
};

CLASS_TAG_LIGHT_MD(Image)
{
public:
	CImageTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Read(const std::vector<NSCSS::CNode>& arSelectors) override;
	bool ReadSVG(const std::vector<NSCSS::CNode>& arSelectors, const std::wstring& wsSVG) override;
};

CLASS_TAG_LIGHT_MD(Blockquote)
{
public:
	CBlockquoteTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open(const std::vector<NSCSS::CNode>& arSelectors) override;
	void Close() override;
};

CLASS_TAG_LIGHT_MD(List)
{
public:
	CListTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open(const NSCSS::CNode& oTagNode) override;
	void Close() override;
};

CLASS_TAG_LIGHT_MD(HorizontalRule)
{
public:
	CHorizontalRuleTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Write(const std::vector<NSCSS::CNode>& arSelectors) override;
};

CLASS_TAG_LIGHT_MD(ListElement)
{
public:
	CListElementTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open() override;
	void Close() override;
};

CLASS_TAG_LIGHT_MD(Code)
{
public:
	CCodeTag(CMDWriter* pWriter) : INTERFACE_TAGS::ITag<CMDWriter>(pWriter) {}
	bool Open(const NSCSS::CNode& oTagNode) override;
	void Close() override;
};

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
