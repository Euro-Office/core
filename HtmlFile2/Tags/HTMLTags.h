#ifndef HTMLTAGS_H
#define HTMLTAGS_H

#include "../Common/3dParty/html/css/src/CNode.h"

namespace HTML
{
class ITag
{
public:
	ITag() = default;
	virtual ~ITag() = default;

	virtual bool Open (const std::vector<NSCSS::CNode>& arSelectors) = 0;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) = 0;
};

template<class Writer>
class IHTMLTag : public ITag
{
protected:
	Writer* m_pWriter;

	bool Valid() const
	{
		return nullptr != m_pWriter;
	}
public:
	IHTMLTag(Writer* pWriter)
		: m_pWriter(pWriter) {};
	virtual ~IHTMLTag() = default;

	virtual bool Open (const std::vector<NSCSS::CNode>& arSelectors) = 0;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) = 0;
};

#define CREATE_TAG(tag_name, writer)\
class C ## tag_name ## Tag : public IHTMLTag<writer>\
{\
public:\
	C ## tag_name ## Tag(writer *pWriter)\
		: IHTMLTag(pWriter) {}\
	\
	bool Open (const std::vector<NSCSS::CNode>& arSelectors) override;\
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override;\
}

class CEmptyTag : public ITag
{
public:
	CEmptyTag() = default;

	bool Open (const std::vector<NSCSS::CNode>& arSelectors) override { return true; }
	void Close(const std::vector<NSCSS::CNode>& arSelectors) override {};
};
}
#endif // HTMLTAGS_H
