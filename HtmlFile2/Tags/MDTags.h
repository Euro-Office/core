#ifndef MDTAGS_H
#define MDTAGS_H

#include "HTMLTags.h"
#include "../Writers/MDWriter.h"
#include "../Table.h"

namespace HTML
{
template<>
class CAnchor<CMDWriter> : public CTag<CMDWriter>
{
public:
	CAnchor(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBold<CMDWriter> : public CTag<CMDWriter>
{
public:
	CBold(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBreak<CMDWriter> : public CTag<CMDWriter>
{
public:
	CBreak(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CItalic<CMDWriter> : public CTag<CMDWriter>
{
public:
	CItalic(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CStrike<CMDWriter> : public CTag<CMDWriter>
{
public:
	CStrike(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CQuotation<CMDWriter> : public CTag<CMDWriter>
{
public:
	CQuotation(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CPreformatted<CMDWriter> : public CTag<CMDWriter>
{
public:
	CPreformatted(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHeader<CMDWriter> : public CTag<CMDWriter>
{
public:
	CHeader(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CImage<CMDWriter> : public CTag<CMDWriter>
{
public:
	CImage(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHorizontalRule<CMDWriter> : public CTag<CMDWriter>
{
public:
	CHorizontalRule(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBlockquote<CMDWriter> : public CTag<CMDWriter>
{
public:
	CBlockquote(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CList<CMDWriter> : public CTag<CMDWriter>
{
public:
	CList(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CListElement<CMDWriter> : public CTag<CMDWriter>
{
public:
	CListElement(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CCode<CMDWriter> : public CTag<CMDWriter>
{
public:
	CCode(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

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
