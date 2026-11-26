// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX number format of a cell with its own style in a column (or row) whose default cell style has a data
// style. The cell's style replaces the default cell style of its column and row, so the cell gets the data style of
// its own style and its parents, or the default format when they have none. In the fixture, column A has a default
// cell style with the format 0.00, and every cell holds the value 0.5.

#include "gtest/gtest.h"

#include "common.h"

#include <regex>
#include <string>

namespace
{
std::wstring part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/number_cell_style_precedence.ods");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The numFmtId of a cell: its s attribute -> the xf in cellXfs ("0" when the cell or its xf has none).
std::wstring cell_numfmt_id(const std::wstring& ref)
{
    const std::wstring sheet = part(L"xl/worksheets/sheet1.xml");
    const std::wstring styles = part(L"xl/styles.xml");

    std::wsmatch m;
    if (!std::regex_search(sheet, m, std::wregex(L"<c r=\"" + ref + L"\"[^>]*\\ss=\"(\\d+)\"")))
        return L"0";
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
    if (!std::regex_search(xfTag, m, std::wregex(L"numFmtId=\"(\\d+)\"")))
        return L"0";
    return m[1].str();
}

// The format of a cell: its numFmtId -> formatCode ("General" for 0, "built-in <id>" for an id without numFmt).
std::wstring cell_format(const std::wstring& ref)
{
    const std::wstring id = cell_numfmt_id(ref);
    if (id == L"0" || id.front() == L'<')
        return id == L"0" ? L"General" : id;

    std::wsmatch m;
    const std::wstring styles = part(L"xl/styles.xml");
    if (!std::regex_search(styles, m, std::wregex(L"<numFmt numFmtId=\"" + id + L"\" formatCode=\"([^\"]*)\"")))
        return L"built-in " + id;
    return m[1].str();
}

// Whether a cell is written as a number (no t attribute or t="n"), so that its number format applies.
bool is_number_cell(const std::wstring& ref)
{
    std::wsmatch m;
    const std::wstring sheet = part(L"xl/worksheets/sheet1.xml");
    if (!std::regex_search(sheet, m, std::wregex(L"<c r=\"" + ref + L"\"[^>]*>")))
        return false;
    return m[0].str().find(L" t=\"") == std::wstring::npos || m[0].str().find(L" t=\"n\"") != std::wstring::npos;
}
} // namespace

// A1: own style without a data style -> the default format, not the 0.00 of the column.
TEST(OdsToXlsxCellStylePrecedence, CellStyleWithoutDataStyleReplacesColumnFormat)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    ASSERT_TRUE(is_number_cell(L"A1"));
    EXPECT_EQ(cell_format(L"A1"), L"General");
}

// A2: no own style -> the format of the column's default cell style (0.00).
TEST(OdsToXlsxCellStylePrecedence, CellWithoutStyleKeepsColumnFormat)
{
    ASSERT_TRUE(is_number_cell(L"A2"));
    const std::wstring format = cell_format(L"A2");
    EXPECT_TRUE(format == L"0.00" || format == L"built-in 2") << format;
}

// A3: own style with a data style -> its format (0.000), not the column's.
TEST(OdsToXlsxCellStylePrecedence, CellStyleWithDataStyleWins)
{
    ASSERT_TRUE(is_number_cell(L"A3"));
    EXPECT_EQ(cell_format(L"A3"), L"0.000");
}

// A4: own style whose parent (a common style) has the data style -> the parent's format (0.000, as A3).
TEST(OdsToXlsxCellStylePrecedence, CellStyleParentDataStyleWins)
{
    ASSERT_TRUE(is_number_cell(L"A4"));
    EXPECT_EQ(cell_format(L"A4"), L"0.000");
}

// A5: own style without a data style in a row whose default cell style has 0.000 -> the default format.
TEST(OdsToXlsxCellStylePrecedence, CellStyleWithoutDataStyleReplacesRowFormat)
{
    ASSERT_TRUE(is_number_cell(L"A5"));
    EXPECT_EQ(cell_format(L"A5"), L"General");
}
