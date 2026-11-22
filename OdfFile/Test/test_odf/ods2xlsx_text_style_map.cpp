// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of number formats with a text part: a number:text-style whose style:map elements give
// the number parts, as LibreOffice writes "YYYY.MM.DD;@".

#include "gtest/gtest.h"

#include "common.h"

#include <regex>
#include <string>

namespace
{
std::wstring part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/number_text_style_map.ods");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The format code of a cell: its s attribute -> the xf in cellXfs -> numFmtId -> formatCode ("General" for 0).
std::wstring cell_format(const std::wstring& ref)
{
    const std::wstring sheet = part(L"xl/worksheets/sheet1.xml");
    const std::wstring styles = part(L"xl/styles.xml");

    std::wsmatch m;
    if (!std::regex_search(sheet, m, std::wregex(L"<c r=\"" + ref + L"\"[^>]*\\ss=\"(\\d+)\"")))
        return L"<no cell style>";
    size_t xf = std::stoul(m[1].str());

    const size_t xfs = styles.find(L"<cellXfs");
    if (xfs == std::wstring::npos)
        return L"<no cellXfs>";
    size_t pos = styles.find(L"<xf ", xfs);
    for (size_t i = 0; i < xf && pos != std::wstring::npos; ++i)
        pos = styles.find(L"<xf ", pos + 1);
    if (pos == std::wstring::npos)
        return L"<no xf>";
    const std::wstring xfTag = styles.substr(pos, styles.find(L'>', pos) - pos);
    if (!std::regex_search(xfTag, m, std::wregex(L"numFmtId=\"(\\d+)\"")) || m[1].str() == L"0")
        return L"General";

    if (!std::regex_search(styles, m, std::wregex(L"<numFmt numFmtId=\"" + m[1].str() + L"\" formatCode=\"([^\"]*)\"")))
        return L"<no numFmt>";
    return m[1].str();
}
}

// The only map has the condition "value()<=" the largest double: it is the format of every number, here a date.
TEST(OdsToXlsxTextStyleMap, DateWithTextPartKeepsTheDate)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(cell_format(L"B1"), L"yyyy.mm.dd") << "control: the same date style without a text part";
    EXPECT_EQ(cell_format(L"A1"), cell_format(L"B1"));
}

// The last map of a text style is for all other numbers whatever its condition (LibreOffice), so -5 shows as -5.00.
TEST(OdsToXlsxTextStyleMap, LastMapIsForAllOtherNumbers)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(cell_format(L"A2"), L"0.00");
}

// Positive, negative and zero maps: unchanged, the text part is still left out.
TEST(OdsToXlsxTextStyleMap, ThreeNumberPartsUnchanged)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(cell_format(L"B2"), L"0.0;&quot;-&quot;0.0;&quot;zero&quot;");
}
