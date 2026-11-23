// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of the columns of a section (style:columns): the
// w:cols element of the section's w:sectPr in word/document.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
// The fixture has one section per case, its
// paragraph reads "IN-<name>". The page layout in use leaves 8in = 11520 twips
// of text; the layout of the first master page (A4) is a different one.
std::wstring document_xml()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/section_columns.odt");
    return dir.empty() ? std::wstring() : read_part(dir, L"word/document.xml");
}

// The w:cols element of the section that holds the paragraph "IN-<name>": the
// first one after that paragraph, before the next section's paragraph.
std::wstring section_cols(const std::wstring& document, const std::wstring& name)
{
    const size_t at = document.find(L">IN-" + name + L"<");
    if (at == std::wstring::npos)
        return L"";
    const size_t begin = document.find(L"<w:cols ", at);
    if (begin == std::wstring::npos || begin > document.find(L">IN-", at + 1))
        return L"";
    const size_t tag_end = document.find(L">", begin);
    if (tag_end == std::wstring::npos)
        return L"";
    if (document[tag_end - 1] == L'/')
        return document.substr(begin, tag_end + 1 - begin);
    const size_t end = document.find(L"</w:cols>", begin);
    return end == std::wstring::npos ? L"" : document.substr(begin, end + 9 - begin);
}

std::wstring attribute(const std::wstring& tag, const std::wstring& name)
{
    const std::wstring key = L" " + name + L"=\"";
    const size_t pos = tag.find(key);
    if (pos == std::wstring::npos)
        return L"(none)";
    const size_t begin = pos + key.size();
    const size_t end = tag.find(L"\"", begin);
    return end == std::wstring::npos ? L"(none)" : tag.substr(begin, end - begin);
}

// The w:col start tags of a w:cols element.
std::vector<std::wstring> columns(const std::wstring& cols)
{
    std::vector<std::wstring> result;
    for (size_t pos = cols.find(L"<w:col "); pos != std::wstring::npos; pos = cols.find(L"<w:col ", pos + 1))
        result.push_back(cols.substr(pos, cols.find(L">", pos) + 1 - pos));
    return result;
}
}

// style:rel-width is relative to the sum of all rel-widths, the start and end
// indents are part of the column; the last column has no space after it.
TEST(OdtToDocxSectionColumns, UnequalColumns)
{
    const std::wstring document = document_xml();
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(document));

    const std::wstring cols = section_cols(document, L"Unequal");
    ASSERT_FALSE(cols.empty());
    EXPECT_EQ(attribute(cols, L"w:equalWidth"), L"0") << cols;
    EXPECT_EQ(attribute(cols, L"w:num"), L"2") << cols;

    // 3000* and 5000* of 11520 twips, less the 0.25in (360 twips) indents
    const std::vector<std::wstring> col = columns(cols);
    ASSERT_EQ(col.size(), 2u) << cols;
    EXPECT_EQ(attribute(col[0], L"w:w"), L"3960") << cols;
    EXPECT_EQ(attribute(col[0], L"w:space"), L"720") << cols;
    EXPECT_EQ(attribute(col[1], L"w:w"), L"6840") << cols;
    EXPECT_EQ(attribute(col[1], L"w:space"), L"(none)") << cols;
}

// rel-widths of 1* are not 1 %: two of them split the width in half.
TEST(OdtToDocxSectionColumns, SmallRelativeWidths)
{
    const std::wstring document = document_xml();
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring cols = section_cols(document, L"Indented");
    ASSERT_FALSE(cols.empty());
    const std::vector<std::wstring> col = columns(cols);
    ASSERT_EQ(col.size(), 2u) << cols;
    EXPECT_EQ(attribute(col[0], L"w:w"), L"5616") << cols;
    EXPECT_EQ(attribute(col[0], L"w:space"), L"288") << cols;
    EXPECT_EQ(attribute(col[1], L"w:w"), L"5616") << cols;
}

// A gap next to columns of different widths doesn't make them equal: ODF 1.4
// §19.233 applies fo:column-gap only to columns without style:column elements
// (LibreOffice makes them equal), also for small rel-widths.
TEST(OdtToDocxSectionColumns, GapWithUnequalColumns)
{
    const std::wstring document = document_xml();
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring cols = section_cols(document, L"GapUnequal");
    EXPECT_EQ(attribute(cols, L"w:equalWidth"), L"0") << cols;
    const std::vector<std::wstring> col = columns(cols);
    ASSERT_EQ(col.size(), 2u) << cols;
    EXPECT_EQ(attribute(col[0], L"w:w"), L"2520") << cols;
    EXPECT_EQ(attribute(col[0], L"w:space"), L"720") << cols;
    EXPECT_EQ(attribute(col[1], L"w:w"), L"8280") << cols;

    const std::wstring small = section_cols(document, L"GapSmallUnequal");
    EXPECT_EQ(attribute(small, L"w:equalWidth"), L"0") << small;
    const std::vector<std::wstring> small_col = columns(small);
    ASSERT_EQ(small_col.size(), 2u) << small;
    EXPECT_EQ(attribute(small_col[0], L"w:w"), L"1944") << small;
    EXPECT_EQ(attribute(small_col[0], L"w:space"), L"720") << small;
    EXPECT_EQ(attribute(small_col[1], L"w:w"), L"8856") << small;
}

// fo:column-gap with style:column elements of equal width, as LibreOffice writes
// equal columns: written as equal columns with that gap.
TEST(OdtToDocxSectionColumns, EqualColumnsWithGap)
{
    const std::wstring document = document_xml();
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring equal = section_cols(document, L"Equal");
    ASSERT_FALSE(equal.empty());
    EXPECT_EQ(attribute(equal, L"w:equalWidth"), L"1") << equal;
    EXPECT_EQ(attribute(equal, L"w:num"), L"3") << equal;
    EXPECT_EQ(attribute(equal, L"w:space"), L"288") << equal;
    EXPECT_TRUE(columns(equal).empty()) << equal;

    const std::wstring gap_only = section_cols(document, L"GapOnly");
    EXPECT_EQ(attribute(gap_only, L"w:equalWidth"), L"1") << gap_only;
    EXPECT_EQ(attribute(gap_only, L"w:num"), L"2") << gap_only;
    EXPECT_EQ(attribute(gap_only, L"w:space"), L"432") << gap_only;
}

// Control: a single column is written as before.
TEST(OdtToDocxSectionColumns, SingleColumn)
{
    const std::wstring document = document_xml();
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring cols = section_cols(document, L"Single");
    EXPECT_EQ(cols, L"<w:cols w:equalWidth=\"1\" w:num=\"1\" w:sep=\"0\" w:space=\"708\"/>");
}
