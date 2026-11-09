// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of empty cells with a cell style that repeat to the
// end of a row, the way LibreOffice writes a row formatted to the end of the
// sheet. They become the row's format, not hundreds of cells that the editor
// would count as used (and print).

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	// The fixture has one sheet per case:
	// 1 "Tail": 1024 columns; row 1 = A empty, B "x", C "y" with style ce1 (a fill),
	//   then ce1 on D..AMJ; row 2 = A "z", then cells without a style to the end;
	//   rows 3-5 = one row repeated 3 times, ce1 on every cell; row 6 = A6:B6 merged
	//   ("m", B6 a covered cell), then ce1 on C..AMJ
	// 2 "Old": 256 columns; A..F text, then a font-only style ce2 on G..IV
	// 3 "Narrow": 10 columns; A "a", then ce1 on B..J
	// 4 "ColumnStyle": 1024 columns, the last one with default cell style ce2;
	//   A "a", then ce1 on B..AMJ
	// 5 "Short": 1024 columns; "a" on A..VB, then ce1 on the 424 cells left
	// 6 "Gaps": 1024 columns; A "a", 300 empty cells without a style, "b", then
	//   ce1 to the end
	// 7 "DataStyle": 1024 columns; A "a", then ce3 (a fill and the number format
	//   "0.00") on B..AMJ
	std::wstring converted_part(const std::wstring& part)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_trailing_cell_style.ods");
		return dir.empty() ? std::wstring() : read_part(dir, part);
	}

	std::wstring sheet_xml(int sheet)
	{
		return converted_part(L"xl/worksheets/sheet" + std::to_wstring(sheet) + L".xml");
	}

	// The <row> element with r="row", empty if none.
	std::wstring row_xml(const std::wstring& sheet, int row)
	{
		const size_t pos = sheet.find(L"<row r=\"" + std::to_wstring(row) + L"\"");
		if (pos == std::wstring::npos)
			return std::wstring();
		const size_t self_closing = sheet.find(L"/>", pos);
		const size_t start_end = sheet.find(L">", pos);
		if (start_end != std::wstring::npos && self_closing + 1 == start_end)
			return sheet.substr(pos, start_end - pos + 1);
		const size_t end = sheet.find(L"</row>", pos);
		return end == std::wstring::npos ? std::wstring() : sheet.substr(pos, end + 6 - pos);
	}

	std::wstring start_tag(const std::wstring& xml)
	{
		const size_t end = xml.find(L">");
		return end == std::wstring::npos ? std::wstring() : xml.substr(0, end + 1);
	}

	// Value of an attribute in a start tag, or "(none)" if it is absent.
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

	// The start tags of the <c> elements of a row, in order.
	std::vector<std::wstring> cells(const std::wstring& row)
	{
		std::vector<std::wstring> result;
		size_t pos = 0;
		while ((pos = row.find(L"<c ", pos)) != std::wstring::npos)
		{
			const size_t end = row.find(L">", pos);
			if (end == std::wstring::npos)
				break;
			result.push_back(row.substr(pos, end - pos + 1));
			pos = end;
		}
		return result;
	}
}

TEST(OdsToXlsx, TrailingCellStyleBecomesRowFormat)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring row = row_xml(sheet, 1);
	ASSERT_FALSE(row.empty());
	const std::vector<std::wstring> row_cells = cells(row);
	ASSERT_EQ(row_cells.size(), 3u) << "only A1:C1, no cells for the trailing run";
	EXPECT_EQ(attribute(row_cells[2], L"r"), L"C1");

	// the row's format is the one C1 gets from the same cell style
	const std::wstring tag = start_tag(row);
	EXPECT_EQ(attribute(tag, L"customFormat"), L"1");
	EXPECT_EQ(attribute(tag, L"s"), attribute(row_cells[2], L"s"));
	EXPECT_NE(attribute(tag, L"s"), L"0");

	// the empty cell before the run keeps the default format, not the row's
	EXPECT_EQ(attribute(row_cells[0], L"r"), L"A1");
	EXPECT_EQ(attribute(row_cells[0], L"s"), L"0");
}

// Control: a run without a style is not written and gives the row no format.
TEST(OdsToXlsx, TrailingCellsWithoutStyle)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring row = row_xml(sheet, 2);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(attribute(start_tag(row), L"s"), L"(none)");
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"(none)");
	EXPECT_EQ(cells(row).size(), 1u);
}

// 250 trailing cells of a 256-column sheet were all written before.
TEST(OdsToXlsx, TrailingCellStyleOnOldSheet)
{
	const std::wstring sheet = sheet_xml(2);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring row = row_xml(sheet, 1);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(cells(row).size(), 6u);
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"1");
	EXPECT_NE(attribute(start_tag(row), L"s"), L"(none)");
	EXPECT_NE(attribute(start_tag(row), L"s"), L"0");
}

// Control: a table narrower than a sheet keeps its styled cells as cells.
TEST(OdsToXlsx, TrailingCellStyleOnNarrowTable)
{
	const std::wstring sheet = sheet_xml(3);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring row = row_xml(sheet, 1);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"(none)");
	const std::vector<std::wstring> row_cells = cells(row);
	ASSERT_EQ(row_cells.size(), 10u);
	EXPECT_EQ(attribute(row_cells[9], L"r"), L"J1");
}

// Control: when a column of the run has its own default cell style, the cells
// of the run could differ, so the row gets no format.
TEST(OdsToXlsx, TrailingCellStyleAcrossColumnStyles)
{
	const std::wstring sheet = sheet_xml(4);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring row = row_xml(sheet, 1);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"(none)");
	EXPECT_GT(cells(row).size(), 1u);
}

// Every repetition of a formatted row is written, each with the row format.
TEST(OdsToXlsx, TrailingCellStyleOnRepeatedRows)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring format = attribute(start_tag(row_xml(sheet, 1)), L"s");
	for (int r = 3; r <= 5; ++r)
	{
		const std::wstring row = row_xml(sheet, r);
		ASSERT_FALSE(row.empty()) << "row " << r << " missing";
		EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"1") << "row " << r;
		EXPECT_EQ(attribute(start_tag(row), L"s"), format) << "row " << r;
		EXPECT_TRUE(cells(row).empty()) << "row " << r;
	}
}

// A covered cell before the run keeps the default format too.
TEST(OdsToXlsx, TrailingCellStyleAfterMergedCell)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring row = row_xml(sheet, 6);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"1");
	const std::vector<std::wstring> row_cells = cells(row);
	ASSERT_EQ(row_cells.size(), 2u);
	EXPECT_EQ(attribute(row_cells[1], L"r"), L"B6");
	EXPECT_EQ(attribute(row_cells[1], L"s"), L"0");
}

// Control: a run shorter than the cells before it is converted as before.
TEST(OdsToXlsx, TrailingCellStyleShorterThanRow)
{
	const std::wstring sheet = sheet_xml(5);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring row = row_xml(sheet, 1);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"(none)");
	EXPECT_GT(cells(row).size(), 600u);
}

// Many empty cells without a style before the run would each need a cell of
// their own: the row is converted as before instead.
TEST(OdsToXlsx, TrailingCellStyleAfterManyEmptyCells)
{
	const std::wstring sheet = sheet_xml(6);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring row = row_xml(sheet, 1);
	ASSERT_FALSE(row.empty());
	EXPECT_EQ(attribute(start_tag(row), L"customFormat"), L"(none)");
	EXPECT_LT(cells(row).size(), 100u);
}

// The row format carries the number format of the run's cell style.
TEST(OdsToXlsx, TrailingCellStyleWithNumberFormat)
{
	const std::wstring sheet = sheet_xml(7);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	const std::wstring styles = converted_part(L"xl/styles.xml");
	ASSERT_FALSE(styles.empty());

	const std::wstring tag = start_tag(row_xml(sheet, 1));
	EXPECT_EQ(attribute(tag, L"customFormat"), L"1") << tag;
	const std::wstring s = attribute(tag, L"s");
	ASSERT_NE(s, L"(none)");

	// the s-th <xf> of cellXfs, and the format code of its numFmtId
	size_t pos = styles.find(L"<cellXfs");
	ASSERT_NE(pos, std::wstring::npos);
	for (int i = 0; i <= std::stoi(s); ++i)
	{
		pos = styles.find(L"<xf ", pos + 1);
		ASSERT_NE(pos, std::wstring::npos) << "xf " << s << " missing";
	}
	const std::wstring numFmtId = attribute(start_tag(styles.substr(pos)), L"numFmtId");
	const size_t format = styles.find(L"<numFmt numFmtId=\"" + numFmtId + L"\"");
	ASSERT_NE(format, std::wstring::npos) << "numFmtId " << numFmtId;
	EXPECT_EQ(attribute(start_tag(styles.substr(format)), L"formatCode"), L"0.00");
}
