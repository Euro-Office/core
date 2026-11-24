// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX: a paragraph that names another page style right after a table.
// The section properties of the page style that ends belong in the last
// paragraph before the change. A table cannot hold them, so the table gets an
// empty paragraph after itself for them. Without it the section was not
// written at all and its pages took the next page style.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
// The end of a table and the paragraph written after it for the section
// properties, up to them.
const std::wstring kCarrier = L"</w:tbl><w:p><w:pPr><w:sectPr";

// The fixture, element by element (page styles: Standard is portrait, Wide is landscape):
//   FIRST (names Standard), table T1,
//   WIDE-1 (names Wide), table T2 with a nested table,
//   WIDE-2 (names Wide again), BEFORE-PORTRAIT,
//   PORTRAIT-2 (names Standard), table T3, a soft page break,
//   WIDE-3 (names Wide), LAST
std::wstring convert_fixture()
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_style_after_table.odt");
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

// "P" or "L" for each w:sectPr of the document, in order.
std::wstring orientations(const std::wstring& xml)
{
    std::wstring result;
    for (size_t pos = xml.find(L"<w:sectPr"); pos != std::wstring::npos; pos = xml.find(L"<w:sectPr", pos + 1))
    {
        const size_t size = xml.find(L"<w:pgSz", pos);
        if (size == std::wstring::npos)
            return result + L"?";
        result += attribute(xml, size, L"w:w") > attribute(xml, size, L"w:h") ? L"L" : L"P";
    }
    return result;
}

// The r:id of the header that each w:sectPr of the document refers to, in
// order ("" for a w:sectPr without a header reference).
std::vector<std::wstring> header_ids(const std::wstring& xml)
{
    std::vector<std::wstring> result;
    for (size_t pos = xml.find(L"<w:sectPr"); pos != std::wstring::npos; pos = xml.find(L"<w:sectPr", pos + 1))
    {
        const size_t end = xml.find(L"</w:sectPr>", pos);
        const std::wstring sect = xml.substr(pos, end == std::wstring::npos ? end : end - pos);
        const std::wstring key = L"<w:headerReference r:id=\"";
        const size_t ref = sect.find(key);
        if (ref == std::wstring::npos)
        {
            result.push_back(L"");
            continue;
        }
        const size_t from = ref + key.size();
        result.push_back(sect.substr(from, sect.find(L'"', from) - from));
    }
    return result;
}

// The text between two pieces of body text.
std::wstring between(const std::wstring& xml, const std::wstring& from, const std::wstring& to)
{
    const size_t a = xml.find(L">" + from + L"<");
    const size_t b = xml.find(L">" + to + L"<");
    if (a == std::wstring::npos || b == std::wstring::npos || b < a)
        return L"";
    return xml.substr(a, b - a);
}
} // namespace

// Each part of the document keeps its own page style: portrait up to T1,
// landscape up to BEFORE-PORTRAIT, portrait up to T3, landscape to the end.
TEST(OdtToDocxPageStyleAfterTable, SectionBeforeTheChangeIsWritten)
{
    const std::wstring xml = convert_fixture();
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));

    EXPECT_EQ(L"PLPL", orientations(xml));
    EXPECT_NE(between(xml, L"FIRST", L"WIDE-1").find(L"<w:sectPr"), std::wstring::npos);
    EXPECT_NE(between(xml, L"PORTRAIT-2", L"WIDE-3").find(L"<w:sectPr"), std::wstring::npos);
}

// The page style that ends also keeps its header: the two portrait parts
// refer to one header, the two landscape parts to the other.
TEST(OdtToDocxPageStyleAfterTable, SectionBeforeTheChangeKeepsItsHeader)
{
    const std::wstring xml = convert_fixture();
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";

    const std::vector<std::wstring> ids = header_ids(xml);
    ASSERT_EQ(4u, ids.size());
    EXPECT_FALSE(ids[0].empty());
    EXPECT_FALSE(ids[1].empty());
    EXPECT_NE(ids[0], ids[1]);
    EXPECT_EQ(ids[0], ids[2]);
    EXPECT_EQ(ids[1], ids[3]);
}

// has no text and follows the table, also when a soft page break comes
// between the table and the paragraph that names the page style (T3).
TEST(OdtToDocxPageStyleAfterTable, EmptyParagraphAfterTheTableHoldsTheSection)
{
    const std::wstring xml = convert_fixture();
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";

    EXPECT_EQ(2u, count(xml, kCarrier));
    EXPECT_NE(between(xml, L"T1", L"WIDE-1").find(kCarrier), std::wstring::npos);
    EXPECT_NE(between(xml, L"T3", L"WIDE-3").find(kCarrier), std::wstring::npos);

    for (size_t pos = xml.find(kCarrier); pos != std::wstring::npos; pos = xml.find(kCarrier, pos + 1))
    {
        const size_t end = xml.find(L"</w:p>", pos);
        ASSERT_NE(end, std::wstring::npos);
        const std::wstring paragraph = xml.substr(pos, end - pos);
        EXPECT_EQ(paragraph.find(L"<w:r>"), std::wstring::npos) << "the paragraph has a run";
        const std::wstring tail = L"</w:sectPr></w:pPr>";
        EXPECT_EQ(paragraph.size() - tail.size(), paragraph.rfind(tail))
            << "the paragraph does not end with the section properties";
    }
}

// No such paragraph where the page style stays (WIDE-2 after T2, whose cell
// has a nested table) or where a paragraph comes before the change
// (BEFORE-PORTRAIT holds the section properties itself).
TEST(OdtToDocxPageStyleAfterTable, NoExtraParagraphWithoutATableOrAChange)
{
    const std::wstring xml = convert_fixture();
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";

    EXPECT_EQ(between(xml, L"WIDE-1", L"WIDE-2").find(L"<w:sectPr"), std::wstring::npos);

    const std::wstring handOver = between(xml, L"WIDE-2", L"PORTRAIT-2");
    EXPECT_EQ(1u, count(handOver, L"<w:sectPr"));
    EXPECT_EQ(0u, count(handOver, kCarrier));
    // BEFORE-PORTRAIT's own paragraph holds them: no paragraph ends between the two.
    const size_t sect = handOver.find(L"<w:sectPr");
    const size_t text = handOver.find(L">BEFORE-PORTRAIT<");
    ASSERT_NE(sect, std::wstring::npos);
    ASSERT_NE(text, std::wstring::npos);
    EXPECT_LT(sect, text);
    EXPECT_EQ(handOver.substr(sect, text - sect).find(L"</w:p>"), std::wstring::npos);

    for (const wchar_t* word : { L"FIRST", L"T1", L"WIDE-1", L"T2", L"NESTED", L"WIDE-2", L"BEFORE-PORTRAIT",
                                 L"PORTRAIT-2", L"T3", L"WIDE-3", L"LAST" })
        EXPECT_NE(xml.find(std::wstring(L">") + word + L"<"), std::wstring::npos) << "text is missing";
}
