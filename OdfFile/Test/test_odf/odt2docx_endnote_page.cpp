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

// The body ends with a table, a heading or a section, after the text `last`:
// an empty paragraph of minimal height holds the section properties of the
// page style in force, then the empty paragraph of the endnote page, whose
// section starts on a new page with the endnote page style (landscape).
void expect_endnote_page_after_spacer(const std::wstring& xml, const std::wstring& last)
{
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));

    const std::vector<std::wstring> all = sections(xml);
    ASSERT_GE(all.size(), 2u);
    EXPECT_EQ(L"continuous", value(all[all.size() - 2], L"w:type", L"w:val"));
    EXPECT_EQ(L"nextPage", value(all.back(), L"w:type", L"w:val"));
    const std::wstring orientation = orientations(xml);
    EXPECT_EQ(L"PL", orientation.substr(orientation.size() - 2));

    const size_t last_text = xml.find(L">" + last + L"<");
    ASSERT_NE(std::wstring::npos, last_text);
    EXPECT_EQ(2u, count(xml, kCarrier));
    const size_t spacer = xml.find(kCarrier, last_text);
    ASSERT_NE(std::wstring::npos, spacer) << "no empty paragraph after the body";
    const size_t spacer_end = xml.find(L"</w:p>", spacer);
    EXPECT_LT(xml.find(L"<w:sectPr", spacer), spacer_end) << "the empty paragraph has no section properties";

    const size_t carrier = xml.find(kCarrier, spacer_end);
    ASSERT_NE(std::wstring::npos, carrier);
    EXPECT_GT(xml.find(L"<w:sectPr", carrier), xml.find(L"</w:p>", carrier)) << "the endnote page paragraph has a section";
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

// No endnote page: for endnotes that a section collects at its own end, and for
// footnotes.
TEST(OdtToDocxEndnotePage, NoEndnotePageWithoutEndnotesAfterTheLastParagraph)
{
    for (const wchar_t* name : {L"endnote_page_in_section.odt", L"endnote_page_footnote.odt"})
    {
        const std::wstring xml = convert(name);
        ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
        EXPECT_TRUE(is_well_formed(xml));
        EXPECT_EQ(0u, count(xml, kCarrier));
        EXPECT_EQ(std::wstring::npos, xml.find(L"<w:type w:val=\"nextPage\"/>"));
    }
}

// A section collects the endnotes of the sections inside it: the endnote in the
// inner section is printed at the end of the outer one, so there is no endnote
// page. Without a collecting section the endnote goes to the endnote page.
TEST(OdtToDocxEndnotePage, OuterSectionCollectsTheEndnotesOfInnerSections)
{
    const std::wstring collected = convert(L"endnote_page_in_nested_section.odt");
    ASSERT_FALSE(collected.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(0u, count(collected, kCarrier));

    const std::wstring not_collected = convert(L"endnote_page_in_plain_section.odt");
    ASSERT_FALSE(not_collected.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(1u, count(not_collected, kCarrier));
}

// Two endnotes in sibling sections inside a collecting section: no endnote
// page; the walk for the second one stops at the outer section, which is
// checked already. An endnote in nested sections that collect nothing still
// gets the endnote page.
TEST(OdtToDocxEndnotePage, EndnotesInSiblingSections)
{
    const std::wstring collected = convert(L"endnote_page_in_sibling_sections.odt");
    ASSERT_FALSE(collected.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(0u, count(collected, kCarrier));

    const std::wstring mixed = convert(L"endnote_page_in_sibling_and_plain_sections.odt");
    ASSERT_FALSE(mixed.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(1u, count(mixed, kCarrier));
}

// A note with two note bodies: the reader keeps the second one and frees the
// first, with the section and the endnote inside it. The section is still
// known by its index, so the check works on no freed element. Every endnote
// is collected by the outer section: no endnote page.
TEST(OdtToDocxEndnotePage, NoteWithTwoNoteBodies)
{
    const std::wstring xml = convert(L"endnote_page_duplicate_note_body.odt");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(0u, count(xml, kCarrier));
}

// A body that ends with a table, a heading or a section has no paragraph of
// its own that can end its section: an empty paragraph after it does.
TEST(OdtToDocxEndnotePage, EndnotePageAfterATableAHeadingOrASection)
{
    expect_endnote_page_after_spacer(convert(L"endnote_page_table.odt"), L"CELL");
    expect_endnote_page_after_spacer(convert(L"endnote_page_heading.odt"), L"LAST");
    expect_endnote_page_after_spacer(convert(L"endnote_page_ends_in_section.odt"), L"LAST");
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
