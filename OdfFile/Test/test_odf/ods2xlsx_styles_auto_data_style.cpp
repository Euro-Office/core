// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of a number format whose data style is defined only in the automatic styles of styles.xml.
// LibreOffice writes a data style there when a header or footer field uses it, and its cell styles in content.xml
// then refer to the same name instead of writing the data style again.

#include "gtest/gtest.h"

#include "common.h"

#include <regex>
#include <string>

namespace
{
std::wstring part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/number_styles_auto_data_style.ods");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The format code of a cell: its s attribute -> the xf in cellXfs -> numFmtId -> formatCode ("General" for 0).
std::wstring cell_format(const std::wstring& ref)
{
    const std::wstring sheet = part(L"xl/worksheets/sheet1.xml");
    const std::wstring styles = part(L"xl/styles.xml");

    std::wsmatch m;
    if (!std::regex_search(sheet, m, std::wregex(L"<c r=\"" + ref + L"\"[^>]*\\ss=\"(\\d+)\"")))
        return L"General";
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

    const std::wstring id = m[1].str();
    if (!std::regex_search(styles, m, std::wregex(L"<numFmt numFmtId=\"" + id + L"\" formatCode=\"([^\"]*)\"")))
        return L"built-in " + id;
    return m[1].str();
}
}

// A1's own cell style uses N9, which only styles.xml's automatic styles define ("0.000").
TEST(OdsToXlsxStylesAutoDataStyle, CellStyleFindsTheDataStyle)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(cell_format(L"A1"), L"0.000");
}

// B1 has no cell style; the default cell style of its column uses N9.
TEST(OdsToXlsxStylesAutoDataStyle, ColumnDefaultFindsTheDataStyle)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(cell_format(L"B1"), L"0.000");
}

// styles.xml's automatic styles also define N8 and N7 under names that content.xml (N8, "0.00000") and office:styles
// (N7, "0.0") define as well: those definitions are used, as before.
TEST(OdsToXlsxStylesAutoDataStyle, OtherDefinitionsOfTheNameUnchanged)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(cell_format(L"C1"), L"0.00000");
    EXPECT_EQ(cell_format(L"D1"), L"0.0");
}
