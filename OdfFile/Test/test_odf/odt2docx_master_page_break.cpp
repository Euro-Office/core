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

    // a table that names a master page starts a new page also with fo:break-before="auto"
    const std::wstring table = paragraph_with_text(xml, L"Table with master");
    ASSERT_FALSE(table.empty());
    EXPECT_TRUE(has_page_break(table));

    // a master page name inside a table cell does not break the page
    const std::wstring cell = paragraph_with_text(xml, L"Cell section paragraph");
    ASSERT_FALSE(cell.empty());
    EXPECT_FALSE(has_page_break(cell));

    const std::wstring last = paragraph_with_text(xml, L"Last paragraph");
    ASSERT_FALSE(last.empty());
    EXPECT_FALSE(has_page_break(last));
}
