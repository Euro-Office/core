// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of a cell whose value type differs from the type of its data style, such as a float cell
// with a percentage style or a date cell with a time style. ODF allows this, and the format of the style applies.
// In the fixture, column A holds such a cell and column B a cell of the same cell style whose value type matches,
// so each A cell must get the number format of its B cell instead of General.

#include "gtest/gtest.h"

#include "common.h"

#include <regex>
#include <string>

namespace
{
std::wstring part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/number_cell_type_mismatch.ods");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The format of a cell: its s attribute -> the xf in cellXfs -> numFmtId -> formatCode ("General" for 0).
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

void expect_same_format(const std::wstring& mismatched, const std::wstring& matching)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_NE(cell_format(matching), L"General");
    EXPECT_EQ(cell_format(mismatched), cell_format(matching));
}
}

// A float cell with a percentage style (0.00%) shows 25.00%, not 0.25.
TEST(OdsToXlsxCellTypeNumberFormat, FloatCellKeepsPercentageFormat)
{
    expect_same_format(L"A1", L"B1");
}

// A date cell with a time style (hh:mm:ss) shows the time, not a date serial number.
TEST(OdsToXlsxCellTypeNumberFormat, DateCellKeepsTimeFormat)
{
    expect_same_format(L"A2", L"B2");
}

// A float cell with a date style (dd.mm.yyyy) shows the date.
TEST(OdsToXlsxCellTypeNumberFormat, FloatCellKeepsDateFormat)
{
    expect_same_format(L"A3", L"B3");
}

// A float cell with a currency style keeps the currency symbol.
TEST(OdsToXlsxCellTypeNumberFormat, FloatCellKeepsCurrencyFormat)
{
    expect_same_format(L"A4", L"B4");
}

// A time cell with a date style shows the date.
TEST(OdsToXlsxCellTypeNumberFormat, TimeCellKeepsDateFormat)
{
    expect_same_format(L"A5", L"B5");
}
