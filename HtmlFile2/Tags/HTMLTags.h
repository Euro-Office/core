#ifndef HTMLTAGS_H
#define HTMLTAGS_H

#include "../Common/3dParty/html/css/src/CNode.h"

namespace HTML
{
namespace INTERFACE_TAGS
{
#define OPEN_INTERFACE_TAG(tag_name)\
class I ## tag_name ## Tag\
{\
public:\
	virtual ~I ## tag_name ## Tag() = default;

#define ADD_INTERFACE_METHOD(return_type, method_name, ...) virtual return_type method_name(__VA_ARGS__) = 0
#define ADD_BOOL_INTERFACE_METHOD(method_name, ...) ADD_INTERFACE_METHOD(bool, method_name, __VA_ARGS__)
#define ADD_VOID_INTERFACE_METHOD(method_name, ...) ADD_INTERFACE_METHOD(void, method_name, __VA_ARGS__)

#define CLOSE_INTERFACE_TAG() };

OPEN_INTERFACE_TAG(Anchor)
	ADD_BOOL_INTERFACE_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_INTERFACE_METHOD(Close, const NSCSS::CNode& oTagNode);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Abbr)
	ADD_BOOL_INTERFACE_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Break)
	ADD_BOOL_INTERFACE_METHOD(Read, const NSCSS::CNode& oTagNode);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Preformatted)
	ADD_BOOL_INTERFACE_METHOD(Open);
	ADD_VOID_INTERFACE_METHOD(Close, const std::vector<NSCSS::CNode>& arSelectors);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Header)
	ADD_BOOL_INTERFACE_METHOD(Open, const NSCSS::CNode& oTagNode);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Division)
	ADD_BOOL_INTERFACE_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Image)
	ADD_BOOL_INTERFACE_METHOD(Read, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_BOOL_INTERFACE_METHOD(ReadSVG, const std::vector<NSCSS::CNode>& arSelectors, const std::wstring& wsSVG);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Font)
	ADD_BOOL_INTERFACE_METHOD(Apply, const NSCSS::CNode& oTagNode, size_t unLevel);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Input)
	ADD_BOOL_INTERFACE_METHOD(Read, const std::vector<NSCSS::CNode>& arSelectors);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(BaseFont)
	ADD_BOOL_INTERFACE_METHOD(Apply, const NSCSS::CNode& oTagNode);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Blockquote)
	ADD_BOOL_INTERFACE_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(List)
	ADD_BOOL_INTERFACE_METHOD(Open, const NSCSS::CNode& oTagNode);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(HorizontalRule)
	ADD_BOOL_INTERFACE_METHOD(Write, const std::vector<NSCSS::CNode>& arSelectors);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(ListElement)
	ADD_BOOL_INTERFACE_METHOD(Open);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(Code)
	ADD_BOOL_INTERFACE_METHOD(Open, const NSCSS::CNode& oTagNode);
	ADD_VOID_INTERFACE_METHOD(Close);
CLOSE_INTERFACE_TAG()

OPEN_INTERFACE_TAG(HTML)
	ADD_BOOL_INTERFACE_METHOD(Apply, const NSCSS::CNode& oTagNode);
CLOSE_INTERFACE_TAG()

template <class Writer>
class ITag
{
public:
	virtual ~ITag() = default;
protected:
	ITag(Writer *pWriter)
		: m_pWriter(pWriter)
	{}

	bool Valid() const
	{
		return nullptr != m_pWriter;
	}

	Writer *m_pWriter;
};
}

#define OPEN_CLASS_TAG(class_name)\
template<class Writer>\
class C ## class_name ## Tag : public INTERFACE_TAGS::I ## class_name ## Tag, public INTERFACE_TAGS::ITag<Writer>\
{\
public:\
	C ## class_name ## Tag(Writer *pWriter) : INTERFACE_TAGS::ITag<Writer>(pWriter) {}

#define CLOSE_CLASS_TAG() };

#define ADD_CLASS_METHOD(return_type, method_name, ...) virtual return_type method_name(__VA_ARGS__)
#define ADD_BOOL_CLASS_METHOD(method_name, ...) ADD_CLASS_METHOD(bool, method_name, __VA_ARGS__) override { return true; }
#define ADD_VOID_CLASS_METHOD(method_name, ...) ADD_CLASS_METHOD(void, method_name, __VA_ARGS__) override {}

OPEN_CLASS_TAG(Anchor)
	ADD_BOOL_CLASS_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_CLASS_METHOD(Close, const NSCSS::CNode& oTagNode);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Abbr)
	ADD_BOOL_CLASS_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Break)
	ADD_BOOL_CLASS_METHOD(Read, const NSCSS::CNode& oTagNode);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Preformatted)
	ADD_BOOL_CLASS_METHOD(Open);
	ADD_VOID_CLASS_METHOD(Close, const std::vector<NSCSS::CNode>& arSelectors);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Header)
	ADD_BOOL_CLASS_METHOD(Open, const NSCSS::CNode& oTagNode);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Division)
	ADD_BOOL_CLASS_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Image)
	ADD_BOOL_CLASS_METHOD(Read, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_BOOL_CLASS_METHOD(ReadSVG, const std::vector<NSCSS::CNode>& arSelectors, const std::wstring& wsSVG);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Font)
	ADD_BOOL_CLASS_METHOD(Apply, const NSCSS::CNode& oTagNode, size_t unLevel);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Input)
	ADD_BOOL_CLASS_METHOD(Read, const std::vector<NSCSS::CNode>& arSelectors);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(BaseFont)
	ADD_BOOL_CLASS_METHOD(Apply, const NSCSS::CNode& oTagNode);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Blockquote)
	ADD_BOOL_CLASS_METHOD(Open, const std::vector<NSCSS::CNode>& arSelectors);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(List)
	ADD_BOOL_CLASS_METHOD(Open, const NSCSS::CNode& oTagNode);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(HorizontalRule)
	ADD_BOOL_CLASS_METHOD(Write, const std::vector<NSCSS::CNode>& arSelectors);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(ListElement)
	ADD_BOOL_CLASS_METHOD(Open);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(Code)
	ADD_BOOL_CLASS_METHOD(Open, const NSCSS::CNode& oTagNode);
	ADD_VOID_CLASS_METHOD(Close);
CLOSE_CLASS_TAG()

OPEN_CLASS_TAG(HTML)
	ADD_BOOL_CLASS_METHOD(Apply, const NSCSS::CNode& oTagNode);
CLOSE_CLASS_TAG()

#define ADD_HTML_TAG(class_name) INTERFACE_TAGS::I ## class_name ## Tag *m_p##class_name{nullptr}

struct THTMLTags
{
	ADD_HTML_TAG(Anchor);
	ADD_HTML_TAG(Abbr);
	ADD_HTML_TAG(Break);
	ADD_HTML_TAG(Preformatted);
	ADD_HTML_TAG(Header);
	ADD_HTML_TAG(Division);
	ADD_HTML_TAG(Image);
	ADD_HTML_TAG(Font);
	ADD_HTML_TAG(Input);
	ADD_HTML_TAG(BaseFont);
	ADD_HTML_TAG(Blockquote);
	ADD_HTML_TAG(List);
	ADD_HTML_TAG(HorizontalRule);
	ADD_HTML_TAG(ListElement);
	ADD_HTML_TAG(Code);
	ADD_HTML_TAG(HTML);

	~THTMLTags()
	{
		Clear();
	}

	void Clear()
	{
		#define CLEAR_VARIABLE(class_name)\
		if (nullptr == m_p##class_name)\
			delete m_p##class_name

		CLEAR_VARIABLE(Anchor);
		CLEAR_VARIABLE(Abbr);
		CLEAR_VARIABLE(Break);
		CLEAR_VARIABLE(Preformatted);
		CLEAR_VARIABLE(Header);
		CLEAR_VARIABLE(Division);
		CLEAR_VARIABLE(Image);
		CLEAR_VARIABLE(Font);
		CLEAR_VARIABLE(Input);
		CLEAR_VARIABLE(BaseFont);
		CLEAR_VARIABLE(Blockquote);
		CLEAR_VARIABLE(List);
		CLEAR_VARIABLE(HorizontalRule);
		CLEAR_VARIABLE(ListElement);
		CLEAR_VARIABLE(Code);
		CLEAR_VARIABLE(HTML);
	}

	template<typename Writer>
	bool Init(Writer *pWriter)
	{
		#define INIT_VARIABLE(class_name) m_p##class_name = new C ## class_name ## Tag<Writer>(pWriter);\
		if (nullptr == m_p##class_name)\
		{\
			Clear();\
			return false;\
		}

		INIT_VARIABLE(Anchor);
		INIT_VARIABLE(Abbr);
		INIT_VARIABLE(Break);
		INIT_VARIABLE(Preformatted);
		INIT_VARIABLE(Header);
		INIT_VARIABLE(Division);
		INIT_VARIABLE(Image);
		INIT_VARIABLE(Font);
		INIT_VARIABLE(Input);
		INIT_VARIABLE(BaseFont);
		INIT_VARIABLE(Blockquote);
		INIT_VARIABLE(List);
		INIT_VARIABLE(HorizontalRule);
		INIT_VARIABLE(ListElement);
		INIT_VARIABLE(Code);
		INIT_VARIABLE(HTML);

		return true;
	}
};
}
#endif // HTMLTAGS_H
