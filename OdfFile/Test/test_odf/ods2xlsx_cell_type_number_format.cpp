// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of a cell whose value type differs from the type of its data style, such as a float cell
// with a percentage style or a date cell with a time style. ODF allows this, and the format of the style applies.
// In the fixture, column A holds such a cell and column B a cell of the same cell style whose value type matches,
// so each A cell must get the number format of its B cell instead of General. Rows 7 and 8 hold a percentage and a
// date cell with the standard number style.

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

void expect_same_format(const std::wstring& mismatched, const std::wstring& matching)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(is_number_cell(mismatched));
    EXPECT_TRUE(is_number_cell(matching));
    EXPECT_NE(cell_format(matching), L"General");
    EXPECT_EQ(cell_format(mismatched), cell_format(matching));
}

}

// A float cell with a percentage style (0.00%) shows 25.00%, not 0.25.
TEST(OdsToXlsxCellTypeNumberFormat, FloatCellKeepsPercentageFormat)
{
    expect_same_format(L"A1", L"B1");
}

// A date-time cell with a time style (hh:mm:ss) shows the time, not a date serial number.
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

// A currency cell with a number style (#,##0.00) keeps it, as in LibreOffice.
TEST(OdsToXlsxCellTypeNumberFormat, CurrencyCellKeepsNumberFormat)
{
    expect_same_format(L"A6", L"B6");
}

// A percentage or date cell with the standard number style gets numFmtId 0 as before, not a "General" numFmt of its own.
TEST(OdsToXlsxCellTypeNumberFormat, PercentageCellWithStandardStyleStaysGeneral)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    ASSERT_TRUE(is_number_cell(L"A7")) << "A7 must stay a number cell";
    EXPECT_EQ(cell_numfmt_id(L"A7"), L"0");
}

TEST(OdsToXlsxCellTypeNumberFormat, DateCellWithStandardStyleStaysGeneral)
{
    ASSERT_FALSE(part(L"xl/styles.xml").empty()) << "ODS -> XLSX conversion failed";
    ASSERT_TRUE(is_number_cell(L"A8")) << "A8 must stay a number cell";
    EXPECT_EQ(cell_numfmt_id(L"A8"), L"0");
}
