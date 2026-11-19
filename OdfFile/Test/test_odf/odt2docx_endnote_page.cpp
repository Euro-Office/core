// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX: the endnote page. A text document prints the endnotes that no
// section collects on a page of their own after the body, with the page style
// of the endnote configuration ("Endnote" by default). A DOCX prints endnotes
// after the last paragraph, so the body has to end with a section for that
// page: the last paragraph of the body holds the section properties of the
// page style in force, an empty paragraph follows, and the last section of
// the document starts on a new page with the endnote page style.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
// The empty paragraph that stands on the endnote page.
const std::wstring kCarrier =
    L"<w:p><w:pPr><w:spacing w:before=\"0\" w:after=\"0\" w:line=\"20\" w:lineRule=\"exact\"/>";

// The page styles of the fixtures: Standard is portrait, Endnote is landscape,
// each with a header of its own.
std::wstring convert(const std::wstring& name)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/" + name);
    if (dir.empty())
        return L"";
    return read_part(dir, L"word/document.xml");
}

size_t count(const std::wstring& xml, const std::wstring& what)
{
    size_t n = 0;
    for (size_t pos = xml.find(what); pos != std::wstring::npos; pos = xml.find(what, pos + what.size()))
        ++n;
    return n;
}

// The value of a number attribute (name="123") at or after `from`.
long attribute(const std::wstring& xml, size_t from, const std::wstring& name)
{
    const size_t pos = xml.find(name + L"=\"", from);
    if (pos == std::wstring::npos)
        return -1;
    return std::stol(xml.substr(pos + name.size() + 2));
}

// Each w:sectPr of the document, in order.
std::vector<std::wstring> sections(const std::wstring& xml)
{
    std::vector<std::wstring> result;
    for (size_t pos = xml.find(L"<w:sectPr"); pos != std::wstring::npos; pos = xml.find(L"<w:sectPr", pos + 1))
    {
        const size_t end = xml.find(L"</w:sectPr>", pos);
        result.push_back(xml.substr(pos, end == std::wstring::npos ? end : end - pos));
    }
    return result;
}

// "P" or "L" for each w:sectPr of the document, in order.
std::wstring orientations(const std::wstring& xml)
{
    std::wstring result;
    for (const std::wstring& section : sections(xml))
    {
        const size_t size = section.find(L"<w:pgSz");
        if (size == std::wstring::npos)
            return result + L"?";
        result += attribute(section, size, L"w:w") > attribute(section, size, L"w:h") ? L"L" : L"P";
    }
    return result;
}

// The value of a string attribute of the first `element` of a section ("" without one).
std::wstring value(const std::wstring& section, const std::wstring& element, const std::wstring& name)
{
    const size_t pos = section.find(L"<" + element);
    if (pos == std::wstring::npos)
        return L"";
    const std::wstring key = name + L"=\"";
    const size_t from = section.find(key, pos);
    if (from == std::wstring::npos || from > section.find(L'>', pos))
        return L"";
    return section.substr(from + key.size(), section.find(L'"', from + key.size()) - from - key.size());
}

// The body ends with: section properties of the page style in force, inside
// the paragraph with the text `last`; the empty paragraph; the section of the
// endnote page, which starts on a new page.
void expect_endnote_page(const std::wstring& xml, const std::wstring& before_last, const std::wstring& last)
{
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));

    const std::vector<std::wstring> all = sections(xml);
    ASSERT_EQ(2u, all.size());
    EXPECT_EQ(L"continuous", value(all[0], L"w:type", L"w:val"));
    EXPECT_EQ(L"nextPage", value(all[1], L"w:type", L"w:val"));

    const size_t first_section = xml.find(L"<w:sectPr");
    const size_t last_text = xml.find(L">" + last + L"<");
    ASSERT_NE(std::wstring::npos, last_text);
    EXPECT_GT(first_section, xml.find(L">" + before_last + L"<"));
    EXPECT_LT(first_section, last_text);
    EXPECT_EQ(std::wstring::npos, xml.substr(first_section, last_text - first_section).find(L"</w:p>"))
        << "the section properties are not in the paragraph with " << std::string(last.begin(), last.end());

    EXPECT_EQ(1u, count(xml, kCarrier));
    EXPECT_GT(xml.find(kCarrier), last_text);
    EXPECT_LT(xml.find(kCarrier), xml.rfind(L"<w:sectPr"));
    EXPECT_EQ(std::wstring::npos, xml.find(L"<w:pageBreakBefore", last_text));
}
} // namespace

// The endnote configuration names the page style Endnote: the endnote page is
// a landscape section on a new page after the portrait body.
TEST(OdtToDocxEndnotePage, EndnotesGetASectionOnANewPage)
{
    const std::wstring xml = convert(L"endnote_page.odt");
    expect_endnote_page(xml, L"FIRST", L"LAST");
    EXPECT_EQ(L"PL", orientations(xml));
}

// The body keeps the header of its page style, the endnote page gets the
// header of the endnote page style.
TEST(OdtToDocxEndnotePage, EndnotePageHasTheHeaderOfItsPageStyle)
{
    const std::wstring xml = convert(L"endnote_page.odt");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";

    const std::vector<std::wstring> all = sections(xml);
    ASSERT_EQ(2u, all.size());
    const std::wstring body = value(all[0], L"w:headerReference", L"r:id");
    const std::wstring endnotes = value(all[1], L"w:headerReference", L"r:id");
    EXPECT_FALSE(body.empty());
    EXPECT_FALSE(endnotes.empty());
    EXPECT_NE(body, endnotes);
}

// A body that ends with a list: the last paragraph of the list (here in a
// nested list) ends the section.
TEST(OdtToDocxEndnotePage, LastParagraphOfAListEndsTheBody)
{
    const std::wstring xml = convert(L"endnote_page_list.odt");
    expect_endnote_page(xml, L"ITEM-2", L"ITEM-LAST");
    EXPECT_EQ(L"PL", orientations(xml));
}

// Without an endnote configuration the page style is Endnote; a document
// that has no such page style keeps the page style in force. Either way the
// endnotes start on a new page.
TEST(OdtToDocxEndnotePage, PageStyleDefaultsToEndnote)
{
    const std::wstring by_name = convert(L"endnote_page_no_configuration.odt");
    expect_endnote_page(by_name, L"FIRST", L"LAST");
    EXPECT_EQ(L"PL", orientations(by_name));

    const std::wstring in_force = convert(L"endnote_page_no_page_style.odt");
    expect_endnote_page(in_force, L"FIRST", L"LAST");
    EXPECT_EQ(L"PP", orientations(in_force));
}

// No endnote page: for endnotes that a section collects at its own end, for
// footnotes, and for a body that ends with a table (no paragraph of the body
// can end the section there).
TEST(OdtToDocxEndnotePage, NoEndnotePageWithoutEndnotesAfterTheLastParagraph)
{
    for (const wchar_t* name :
         {L"endnote_page_in_section.odt", L"endnote_page_footnote.odt", L"endnote_page_table.odt"})
    {
        const std::wstring xml = convert(name);
        ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
        EXPECT_TRUE(is_well_formed(xml));
        EXPECT_EQ(0u, count(xml, kCarrier));
        EXPECT_EQ(std::wstring::npos, xml.find(L"<w:type w:val=\"nextPage\"/>"));
    }
}

// A heading writes the section properties into an empty paragraph before
// itself, so it would move to the endnote page: no endnote page.
TEST(OdtToDocxEndnotePage, NoEndnotePageWhenTheBodyEndsWithAHeading)
{
    const std::wstring xml = convert(L"endnote_page_heading.odt");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(0u, count(xml, kCarrier));
    EXPECT_EQ(std::wstring::npos, xml.find(L"<w:type w:val=\"nextPage\"/>"));
}

// With ContinuousEndnotes LibreOffice prints the endnotes right after the
// text: no endnote page.
TEST(OdtToDocxEndnotePage, NoEndnotePageWithContinuousEndnotes)
{
    const std::wstring xml = convert(L"endnote_page_continuous.odt");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(0u, count(xml, kCarrier));
    EXPECT_EQ(std::wstring::npos, xml.find(L"<w:type w:val=\"nextPage\"/>"));
}
