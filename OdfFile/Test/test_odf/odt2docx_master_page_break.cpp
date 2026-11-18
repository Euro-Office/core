// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX: a paragraph whose style names a master page starts a new page.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The w:p element that contains the given text, or an empty string.
std::wstring paragraph_with_text(const std::wstring& xml, const std::wstring& text)
{
    const size_t pos = xml.find(text);
    if (pos == std::wstring::npos)
        return L"";

    size_t start = xml.rfind(L"<w:p>", pos);
    const size_t start_attr = xml.rfind(L"<w:p ", pos);
    if (start == std::wstring::npos || (start_attr != std::wstring::npos && start_attr > start))
        start = start_attr;
    if (start == std::wstring::npos)
        return L"";

    return xml.substr(start, pos - start);
}

bool has_page_break(const std::wstring& paragraph)
{
    return paragraph.find(L"<w:pageBreakBefore w:val=\"true\"/>") != std::wstring::npos ||
           paragraph.find(L"<w:br w:type=\"page\"/>") != std::wstring::npos;
}

// The w:style element with the given w:name, or an empty string.
std::wstring style_with_name(const std::wstring& xml, const std::wstring& name)
{
    const size_t pos = xml.find(L"<w:name w:val=\"" + name + L"\"/>");
    if (pos == std::wstring::npos)
        return L"";

    const size_t start = xml.rfind(L"<w:style ", pos);
    const size_t end = xml.find(L"</w:style>", pos);
    if (start == std::wstring::npos || end == std::wstring::npos)
        return L"";

    return xml.substr(start, end - start);
}
} // namespace

// style:master-page-name starts a new page even when the master page stays the same,
// also when inherited from a parent style or inside an index, but not on the first paragraph
// of the body, not inside a hidden section and not when an empty value overrides the parent.
TEST(OdtToDocxMasterPageBreak, MasterPageNameStartsNewPage)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/master_page_break.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const std::wstring first = paragraph_with_text(xml, L"First paragraph");
    ASSERT_FALSE(first.empty());
    EXPECT_FALSE(has_page_break(first));

    const std::wstring plain = paragraph_with_text(xml, L"Plain paragraph");
    ASSERT_FALSE(plain.empty());
    EXPECT_FALSE(has_page_break(plain));

    const std::wstring same = paragraph_with_text(xml, L"Same page style paragraph");
    ASSERT_FALSE(same.empty());
    EXPECT_TRUE(has_page_break(same));

    const std::wstring hidden = paragraph_with_text(xml, L"Hidden paragraph");
    if (!hidden.empty())
        EXPECT_FALSE(has_page_break(hidden));

    // the master page comes from the parent style
    const std::wstring inherited = paragraph_with_text(xml, L"Inherited paragraph");
    ASSERT_FALSE(inherited.empty());
    EXPECT_TRUE(has_page_break(inherited));

    // an empty style:master-page-name overrides the parent style: no new page
    const std::wstring empty_master = paragraph_with_text(xml, L"Empty master paragraph");
    ASSERT_FALSE(empty_master.empty());
    EXPECT_FALSE(has_page_break(empty_master));

    // a paragraph inside a table of contents
    const std::wstring index = paragraph_with_text(xml, L"Index paragraph");
    ASSERT_FALSE(index.empty());
    EXPECT_TRUE(has_page_break(index));

    const std::wstring last = paragraph_with_text(xml, L"Last paragraph");
    ASSERT_FALSE(last.empty());
    EXPECT_FALSE(has_page_break(last));
}

// The page break is written once, never for a master page that does not exist, not cancelled by
// fo:break-before="auto" on a table, and not inside a table.
TEST(OdtToDocxMasterPageBreak, MasterPageBreakOnceAndOnlyOnPages)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/master_page_break.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    const std::wstring styles = read_part(dir, L"word/styles.xml");
    ASSERT_FALSE(styles.empty()) << "word/styles.xml is missing";

    // the paragraph style breaks the page itself: no second break in the paragraph
    const std::wstring break_style = style_with_name(styles, L"ChapterBreak");
    ASSERT_FALSE(break_style.empty());
    EXPECT_NE(break_style.find(L"<w:pageBreakBefore w:val=\"true\"/>"), std::wstring::npos);

    const std::wstring break_paragraph = paragraph_with_text(xml, L"Break style paragraph");
    ASSERT_FALSE(break_paragraph.empty());
    EXPECT_FALSE(has_page_break(break_paragraph));

    // the same for an automatic style that inherits both from that style
    const std::wstring break_child = paragraph_with_text(xml, L"Break child paragraph");
    ASSERT_FALSE(break_child.empty());
    EXPECT_FALSE(has_page_break(break_child));

    // a master page that does not exist is ignored
    const std::wstring missing = paragraph_with_text(xml, L"Missing master paragraph");
    ASSERT_FALSE(missing.empty());
    EXPECT_FALSE(has_page_break(missing));

    // a table that names a master page starts a new page also with fo:break-before="auto"; the break is
    // w:pageBreakBefore on the first paragraph of the first cell, here one without a style
    const std::wstring table = paragraph_with_text(xml, L"Table with master");
    ASSERT_FALSE(table.empty());
    EXPECT_NE(table.find(L"<w:pageBreakBefore w:val=\"true\"/>"), std::wstring::npos);
    EXPECT_EQ(table.find(L"<w:br w:type=\"page\"/>"), std::wstring::npos);

    // a master page name inside a table cell does not break the page
    const std::wstring cell = paragraph_with_text(xml, L"Cell section paragraph");
    ASSERT_FALSE(cell.empty());
    EXPECT_FALSE(has_page_break(cell));

    const std::wstring last = paragraph_with_text(xml, L"Last paragraph");
    ASSERT_FALSE(last.empty());
    EXPECT_FALSE(has_page_break(last));
}

// A master page with another page layout also starts a new section. The page break is then written as
// w:pageBreakBefore, never as a page break run at the start of the paragraph: after a section break that already
// starts a new page, such a run gives an empty page. This holds for a named paragraph style, for an automatic style
// without paragraph properties and for one with them.
TEST(OdtToDocxMasterPageBreak, PageLayoutChangeGivesNoEmptyPage)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/master_page_layout_break.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const std::wstring page_break_run = L"<w:br w:type=\"page\"/>";
    const std::wstring page_break_before = L"<w:pageBreakBefore w:val=\"true\"/>";
    const std::wstring section = L"<w:sectPr";

    // the section handling is unchanged: the paragraph before each switch ends a section
    const std::wstring first = paragraph_with_text(xml, L"First paragraph");
    ASSERT_FALSE(first.empty());
    EXPECT_NE(first.find(section), std::wstring::npos);
    EXPECT_FALSE(has_page_break(first));

    const std::wstring named = paragraph_with_text(xml, L"Named style paragraph");
    ASSERT_FALSE(named.empty());
    EXPECT_EQ(named.find(page_break_run), std::wstring::npos);
    EXPECT_NE(named.find(page_break_before), std::wstring::npos);
    EXPECT_NE(named.find(section), std::wstring::npos);

    const std::wstring automatic = paragraph_with_text(xml, L"Automatic style paragraph");
    ASSERT_FALSE(automatic.empty());
    EXPECT_EQ(automatic.find(page_break_run), std::wstring::npos);
    EXPECT_NE(automatic.find(page_break_before), std::wstring::npos);
    EXPECT_NE(automatic.find(section), std::wstring::npos);

    const std::wstring with_properties = paragraph_with_text(xml, L"Automatic style with properties paragraph");
    ASSERT_FALSE(with_properties.empty());
    EXPECT_EQ(with_properties.find(page_break_run), std::wstring::npos);
    EXPECT_NE(with_properties.find(page_break_before), std::wstring::npos);
    EXPECT_EQ(with_properties.find(section), std::wstring::npos);

    const std::wstring last = paragraph_with_text(xml, L"Last paragraph");
    ASSERT_FALSE(last.empty());
    EXPECT_FALSE(has_page_break(last));

    // one section per master page switch and the last one in w:body: no page break run at all
    size_t sections = 0;
    for (size_t pos = xml.find(section); pos != std::wstring::npos; pos = xml.find(section, pos + 1))
        sections++;
    EXPECT_EQ(sections, 4u);
    EXPECT_EQ(xml.find(page_break_run), std::wstring::npos);
}

// A table that names a master page starts a new page through w:pageBreakBefore on the first paragraph of its first
// cell, whatever the style of that paragraph. Word and LibreOffice ignore a page break run there, so the table would
// stay on the previous page.
TEST(OdtToDocxMasterPageBreak, TableBreakIsPageBreakBeforeInFirstCell)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/master_page_break.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const std::wstring page_break_run = L"<w:br w:type=\"page\"/>";
    const std::wstring page_break_before = L"<w:pageBreakBefore w:val=\"true\"/>";

    // named paragraph style, automatic style without paragraph properties, automatic style with them
    for (const wchar_t* text :
         {L"Table named style cell", L"Table plain automatic cell", L"Table formatted automatic cell"})
    {
        const std::wstring cell = paragraph_with_text(xml, text);
        ASSERT_FALSE(cell.empty()) << text;
        EXPECT_NE(cell.find(page_break_before), std::wstring::npos) << text;
        EXPECT_EQ(cell.find(page_break_run), std::wstring::npos) << text;
    }

    // only the first paragraph of the cell breaks the page
    const std::wstring second = paragraph_with_text(xml, L"Table second paragraph");
    ASSERT_FALSE(second.empty());
    EXPECT_FALSE(has_page_break(second));

    // no page break run anywhere in the document
    EXPECT_EQ(xml.find(page_break_run), std::wstring::npos);
}
