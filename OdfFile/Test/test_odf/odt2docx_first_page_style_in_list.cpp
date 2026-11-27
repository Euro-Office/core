// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of the page style of the first page. The paragraph
// the body starts with names it (style:master-page-name), also when that
// paragraph is inside a list.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
// Value of an attribute of the element that starts at 'from'.
std::wstring attribute(const std::wstring& xml, size_t from, const std::wstring& name)
{
    const size_t end = xml.find(L'>', from);
    const size_t at  = xml.find(L" " + name + L"=\"", from);
    if (end == std::wstring::npos || at == std::wstring::npos || at > end)
        return L"";
    const size_t begin = at + name.size() + 3;
    return xml.substr(begin, xml.find(L'"', begin) - begin);
}

// The text of all w:t elements.
std::wstring text_of(const std::wstring& xml)
{
    std::wstring text;
    for (size_t at = xml.find(L"<w:t"); at != std::wstring::npos; at = xml.find(L"<w:t", at + 1))
    {
        if (xml[at + 4] != L'>' && xml[at + 4] != L' ')
            continue; // w:tab, w:tbl, ...
        const size_t begin = xml.find(L'>', at) + 1;
        text += xml.substr(begin, xml.find(L"</w:t>", begin) - begin);
    }
    return text;
}

// The text of the default header every section of the document refers to, in
// document order. A section without a default header reference gives "".
std::vector<std::wstring> headers(const std::wstring& odt)
{
    std::vector<std::wstring> result;

    const std::wstring dir = convert_odf_to_ooxml_dir(odt);
    if (dir.empty())
    {
        ADD_FAILURE() << "ODT -> DOCX conversion failed";
        return result;
    }
    const std::wstring document = read_part(dir, L"word/document.xml");
    const std::wstring rels     = read_part(dir, L"word/_rels/document.xml.rels");
    EXPECT_TRUE(is_well_formed(document));

    for (size_t at = document.find(L"<w:sectPr"); at != std::wstring::npos;
         at = document.find(L"<w:sectPr", at + 1))
    {
        const std::wstring sectPr = document.substr(at, document.find(L"</w:sectPr>", at) - at);

        std::wstring header;
        for (size_t ref = sectPr.find(L"<w:headerReference "); ref != std::wstring::npos;
             ref = sectPr.find(L"<w:headerReference ", ref + 1))
        {
            if (attribute(sectPr, ref, L"w:type") != L"default")
                continue;
            const size_t rel = rels.find(L"Id=\"" + attribute(sectPr, ref, L"r:id") + L"\"");
            if (rel == std::wstring::npos)
            {
                ADD_FAILURE() << "reference without relationship in " << sectPr;
                continue;
            }
            header = text_of(read_part(dir, L"word/" + attribute(rels, rels.rfind(L'<', rel), L"Target")));
        }
        result.push_back(header);
    }
    return result;
}

bool refers_to(const std::vector<std::wstring>& all, const std::wstring& header)
{
    for (size_t i = 0; i < all.size(); i++)
        if (all[i] == header)
            return true;
    return false;
}
}

// A numbered heading inside a list starts the body and names the page style FIRST.
TEST(OdtToDocxFirstPageStyleInList, ListAtTheStartOfTheBody)
{
    const std::vector<std::wstring> all = headers(L"ExampleFiles/first_page_style_in_list.odt");
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0], L"FIRST-HEADER");
    EXPECT_EQ(all[1], L"SECOND-HEADER");
}

// The same below a page-anchored frame, inside a section, a nested list and a list header.
TEST(OdtToDocxFirstPageStyleInList, NestedListAtTheStartOfASection)
{
    const std::vector<std::wstring> all = headers(L"ExampleFiles/first_page_style_in_nested_list.odt");
    // How many sections the text section adds is not the point here.
    ASSERT_GE(all.size(), 2u);
    EXPECT_EQ(all[0], L"FIRST-HEADER");
    EXPECT_TRUE(refers_to(all, L"SECOND-HEADER"));
    EXPECT_FALSE(refers_to(all, L"STANDARD-HEADER"));
}

// The first page style names a next style: a soft page break hands over to it. Without the first page style
// nothing is handed over. Which pages each of the two styles gets is not checked here.
TEST(OdtToDocxFirstPageStyleInList, SoftPageBreakHandsOver)
{
    const std::vector<std::wstring> all = headers(L"ExampleFiles/first_page_style_in_list_soft_break.odt");
    EXPECT_TRUE(refers_to(all, L"SECOND-HEADER"));
    EXPECT_FALSE(refers_to(all, L"STANDARD-HEADER"));
}

// A paragraph before the list starts the body: its page style is the first one, not the list's.
TEST(OdtToDocxFirstPageStyleInList, ParagraphBeforeTheList)
{
    const std::vector<std::wstring> all = headers(L"ExampleFiles/first_page_style_paragraph_before_list.odt");
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0], L"SECOND-HEADER");
    EXPECT_EQ(all[1], L"FIRST-HEADER");
}

// The name of a page style that does not exist is ignored.
TEST(OdtToDocxFirstPageStyleInList, PageStyleThatDoesNotExist)
{
    const std::vector<std::wstring> all = headers(L"ExampleFiles/first_page_style_in_list_missing.odt");
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0], L"STANDARD-HEADER");
    EXPECT_EQ(all[1], L"SECOND-HEADER");
}
