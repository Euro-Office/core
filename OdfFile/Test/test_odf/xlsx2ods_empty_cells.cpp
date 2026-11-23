// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: empty cells that are not converted one by one (gaps, merged ranges, runs of one style).

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

namespace
{
	struct Cell
	{
		bool covered = false;
		std::wstring style;
		std::wstring text;
	};

	std::wstring convert_xlsx_content(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}

	std::wstring strip_tags(const std::wstring& xml)
	{
		std::wstring text;
		bool in_tag = false;
		for (wchar_t c : xml)
		{
			if (c == L'<')
				in_tag = true;
			else if (c == L'>')
				in_tag = false;
			else if (!in_tag)
				text += c;
		}
		return text;
	}

	std::wstring attr_value(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring attr = L" " + name + L"=\"";
		const size_t pos = tag.find(attr);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + attr.size();
		return tag.substr(start, tag.find(L'"', start) - start);
	}

	size_t attr_count(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring value = attr_value(tag, name);
		return value.empty() ? 1 : std::stoul(value);
	}

	// The start of the next <table:table-row> element at or after pos (not <table:table-rows>).
	size_t find_row(const std::wstring& xml, size_t pos)
	{
		for (pos = xml.find(L"<table:table-row", pos); pos != std::wstring::npos; pos = xml.find(L"<table:table-row", pos + 1))
		{
			const wchar_t next = pos + 16 < xml.size() ? xml[pos + 16] : L'\0';
			if (next == L' ' || next == L'>' || next == L'/')
				return pos;
		}
		return std::wstring::npos;
	}

	// The start of the next table cell, covered or not, at or after pos.
	size_t find_cell(const std::wstring& xml, size_t pos)
	{
		return std::min(xml.find(L"<table:table-cell", pos), xml.find(L"<table:covered-table-cell", pos));
	}

	// The row element with the given 0-based index in the first table (table:number-rows-repeated expanded).
	std::wstring row_xml(const std::wstring& xml, size_t row)
	{
		const size_t table = xml.find(L"<table:table ");
		if (table == std::wstring::npos)
			return L"";
		size_t index = 0;
		for (size_t pos = find_row(xml, table); pos != std::wstring::npos; pos = find_row(xml, pos + 1))
		{
			const size_t tag_end = xml.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				return L"";
			const bool empty = xml[tag_end - 1] == L'/';
			const size_t end = empty ? tag_end + 1 : xml.find(L"</table:table-row>", tag_end);
			if (end == std::wstring::npos)
				return L"";
			index += attr_count(xml.substr(pos, tag_end + 1 - pos), L"table:number-rows-repeated");
			if (row < index)
				return xml.substr(pos, end - pos);
		}
		return L"";
	}

	// The table:style-name of the row with the given 0-based index in the first table.
	std::wstring row_style(const std::wstring& xml, size_t row)
	{
		const std::wstring row_element = row_xml(xml, row);
		return attr_value(row_element.substr(0, row_element.find(L'>') + 1), L"table:style-name");
	}

	// The first max_cols cells of the row with the given 0-based index in the first table, one entry per
	// column (table:number-columns-repeated expanded).
	std::vector<Cell> row_cells(const std::wstring& xml, size_t row, size_t max_cols = 4)
	{
		std::vector<Cell> cells;
		const std::wstring row_element = row_xml(xml, row);

		size_t pos = find_cell(row_element, 0);
		while (pos != std::wstring::npos && cells.size() < max_cols)
		{
			const size_t tag_end = row_element.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				break;
			const std::wstring tag = row_element.substr(pos, tag_end + 1 - pos);
			const bool covered = tag.compare(0, 25, L"<table:covered-table-cell") == 0;

			Cell cell;
			cell.covered = covered;
			cell.style = attr_value(tag, L"table:style-name");
			size_t next = tag_end + 1;
			if (tag[tag.size() - 2] != L'/')
			{
				const size_t close = row_element.find(covered ? L"</table:covered-table-cell>" : L"</table:table-cell>", tag_end);
				if (close == std::wstring::npos)
					break;
				cell.text = strip_tags(row_element.substr(tag_end + 1, close - tag_end - 1));
				next = close;
			}

			const size_t repeat = attr_count(tag, L"table:number-columns-repeated");
			for (size_t i = 0; i < repeat && cells.size() < max_cols; ++i)
				cells.push_back(cell);

			pos = find_cell(row_element, next);
		}
		return cells;
	}
}

// A1:C1 is merged and the XLSX has no B1 and C1 elements: B1 and C1 are covered, D1 is a normal cell
// that keeps its value.
TEST(XlsxToOds, CellAfterMergedRangeWithoutCoveredCellsIsNotCovered)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_merged_without_covered_cells.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	const std::vector<Cell> cells = row_cells(xml, 0);
	ASSERT_EQ(cells.size(), 4u);
	EXPECT_FALSE(cells[0].covered);
	EXPECT_EQ(cells[0].text, L"title");
	EXPECT_TRUE(cells[1].covered) << "B1";
	EXPECT_TRUE(cells[2].covered) << "C1";
	EXPECT_FALSE(cells[3].covered) << "D1 is a covered cell";
	EXPECT_EQ(cells[3].text, L"7");

	// B3:D3 is merged after a missing A3, its covered cells are written: A3 stays a normal cell, C3 and D3
	// are covered, E3 is not.
	const std::vector<Cell> row3 = row_cells(xml, 2, 5);
	ASSERT_EQ(row3.size(), 5u);
	EXPECT_FALSE(row3[0].covered) << "A3";
	EXPECT_FALSE(row3[1].covered) << "B3";
	EXPECT_EQ(row3[1].text, L"second");
	EXPECT_TRUE(row3[2].covered) << "C3";
	EXPECT_TRUE(row3[3].covered) << "D3";
	EXPECT_FALSE(row3[4].covered) << "E3";
	EXPECT_EQ(row3[4].text, L"8");

	// D5:F5 is merged after three missing cells, no covered cells are written: A5:C5 are normal cells,
	// E5 and F5 are covered, G5 is not.
	const std::vector<Cell> row5 = row_cells(xml, 4, 7);
	ASSERT_EQ(row5.size(), 7u);
	EXPECT_FALSE(row5[0].covered) << "A5";
	EXPECT_FALSE(row5[1].covered) << "B5";
	EXPECT_FALSE(row5[2].covered) << "C5";
	EXPECT_FALSE(row5[3].covered) << "D5";
	EXPECT_EQ(row5[3].text, L"third");
	EXPECT_TRUE(row5[4].covered) << "E5";
	EXPECT_TRUE(row5[5].covered) << "F5";
	EXPECT_FALSE(row5[6].covered) << "G5";
	EXPECT_EQ(row5[6].text, L"9");

	// B7:C8 is merged over two rows: B8 and C8 are covered, A8 and D8 keep their values.
	const std::vector<Cell> row8 = row_cells(xml, 7);
	ASSERT_EQ(row8.size(), 4u);
	EXPECT_FALSE(row8[0].covered) << "A8";
	EXPECT_EQ(row8[0].text, L"11");
	EXPECT_TRUE(row8[1].covered) << "B8";
	EXPECT_TRUE(row8[2].covered) << "C8";
	EXPECT_FALSE(row8[3].covered) << "D8";
	EXPECT_EQ(row8[3].text, L"10");

	// A14:D16 is merged over three rows: A to D are covered in rows 15 and 16, E keeps its value.
	for (size_t row = 14; row < 16; ++row)
	{
		const std::vector<Cell> cells = row_cells(xml, row, 5);
		ASSERT_EQ(cells.size(), 5u);
		for (size_t col = 0; col < 4; ++col)
			EXPECT_TRUE(cells[col].covered) << static_cast<char>('A' + col) << row + 1;
		EXPECT_FALSE(cells[4].covered) << "E" << row + 1;
		EXPECT_EQ(cells[4].text, std::to_wstring(row)) << "E" << row + 1;
	}

	// A18:C20 is merged and rows 19 and 20 are missing (one gap of two rows): the range ends there, so A21 and B21 are
	// normal cells with their values.
	const std::vector<Cell> row21 = row_cells(xml, 20, 3);
	ASSERT_EQ(row21.size(), 3u);
	EXPECT_FALSE(row21[0].covered) << "A21";
	EXPECT_EQ(row21[0].text, L"21") << "A21";
	EXPECT_FALSE(row21[1].covered) << "B21";
	EXPECT_EQ(row21[1].text, L"22") << "B21";

	// A23:B25 is merged and rows 24 to 26 are missing (one gap that goes past the range): A and B are covered in rows
	// 24 and 25 only; A27 keeps its value.
	for (size_t row = 23; row < 25; ++row)
	{
		const std::vector<Cell> cells = row_cells(xml, row, 2);
		ASSERT_EQ(cells.size(), 2u);
		EXPECT_TRUE(cells[0].covered) << "A" << row + 1;
		EXPECT_TRUE(cells[1].covered) << "B" << row + 1;
	}
	const std::vector<Cell> row26 = row_cells(xml, 25, 2);
	ASSERT_EQ(row26.size(), 2u);
	EXPECT_FALSE(row26[0].covered) << "A26";
	EXPECT_FALSE(row26[1].covered) << "B26";
	const std::vector<Cell> row27 = row_cells(xml, 26, 1);
	ASSERT_EQ(row27.size(), 1u);
	EXPECT_FALSE(row27[0].covered) << "A27";
	EXPECT_EQ(row27[0].text, L"27") << "A27";

	// A30:B2030 is merged and rows 31 to 3029 are missing: the gap is split where the range ends, row 2030 is still
	// covered, row 2031 is not, and A3030 stays in row 3030.
	const std::vector<Cell> row2030 = row_cells(xml, 2029, 1);
	ASSERT_EQ(row2030.size(), 1u);
	EXPECT_TRUE(row2030[0].covered) << "A2030";
	const std::vector<Cell> row3030 = row_cells(xml, 3029, 1);
	ASSERT_EQ(row3030.size(), 1u) << "no row 3030";
	EXPECT_FALSE(row3030[0].covered) << "A3030";
	EXPECT_EQ(row3030[0].text, L"3030") << "A3030";

	// A data validation in B3050:B3059 inside the gap of rows 3041 to 3099 splits the gap in three; A3100 stays in row 3100.
	const std::vector<Cell> row3100 = row_cells(xml, 3099, 1);
	ASSERT_EQ(row3100.size(), 1u) << "no row 3100";
	EXPECT_EQ(row3100[0].text, L"3100") << "A3100";
}

// Empty cells with the same style (fill and borders) next to each other: the XLSX reader folds them into
// the first one, and every folded cell keeps the style.
TEST(XlsxToOds, EmptyCellsOfOneStyleKeepTheStyle)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_repeated_empty_styled.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	// Row 1: A1:C1 empty with the style, D1 = 1.
	const std::vector<Cell> row1 = row_cells(xml, 0);
	ASSERT_EQ(row1.size(), 4u);
	const std::wstring style = row1[0].style;
	ASSERT_FALSE(style.empty()) << "A1 has no style";
	const size_t name_pos = xml.find(L"style:name=\"" + style + L"\"");
	ASSERT_NE(name_pos, std::wstring::npos) << "the style of A1 has no definition";
	const size_t style_pos = xml.rfind(L"<style:style", name_pos);
	ASSERT_NE(style_pos, std::wstring::npos);
	std::wstring definition = xml.substr(style_pos, xml.find(L"</style:style>", style_pos) - style_pos);
	std::transform(definition.begin(), definition.end(), definition.begin(),
				   [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
	EXPECT_NE(definition.find(L"fo:background-color=\"#ffff00\""), std::wstring::npos) << "the style of A1 has no yellow fill";
	EXPECT_EQ(row1[1].style, style) << "B1";
	EXPECT_EQ(row1[2].style, style) << "C1";
	EXPECT_EQ(row1[3].text, L"1");
	EXPECT_NE(row1[3].style, style) << "D1";

	// Row 2: A2 = 2, B2:C2 empty with the style at the end of the row.
	const std::vector<Cell> row2 = row_cells(xml, 1);
	ASSERT_EQ(row2.size(), 4u);
	EXPECT_EQ(row2[0].text, L"2");
	EXPECT_EQ(row2[1].style, style) << "B2";
	EXPECT_EQ(row2[2].style, style) << "C2";
	EXPECT_NE(row2[3].style, style) << "D2";

	// Row 3: A3:C3 merged, all with the style; D3 = 3 stays a normal cell.
	const std::vector<Cell> row3 = row_cells(xml, 2);
	ASSERT_EQ(row3.size(), 4u);
	EXPECT_FALSE(row3[0].covered);
	EXPECT_EQ(row3[0].style, style) << "A3";
	EXPECT_TRUE(row3[1].covered) << "B3";
	EXPECT_EQ(row3[1].style, style) << "B3";
	EXPECT_TRUE(row3[2].covered) << "C3";
	EXPECT_EQ(row3[2].style, style) << "C3";
	EXPECT_FALSE(row3[3].covered) << "D3";
	EXPECT_EQ(row3[3].text, L"3");

	// Row 4: empty cells without a style between two numbers stay without a style.
	const std::vector<Cell> row4 = row_cells(xml, 3);
	ASSERT_EQ(row4.size(), 4u);
	EXPECT_EQ(row4[0].text, L"4");
	EXPECT_EQ(row4[1].style, L"") << "B4";
	EXPECT_EQ(row4[2].style, L"") << "C4";
	EXPECT_EQ(row4[3].text, L"5");

	// Rows 5 to 7: the same run of styled empty cells A:C in each row. The XLSX reader folds rows 6 and 7
	// into row 5; every row keeps the style.
	for (size_t row = 4; row < 7; ++row)
	{
		const std::vector<Cell> cells = row_cells(xml, row);
		if (cells.size() < 4)
		{
			ADD_FAILURE() << "row " << row + 1 << " has " << cells.size() << " cells";
			continue;
		}
		EXPECT_EQ(cells[0].style, style) << "A" << row + 1;
		EXPECT_EQ(cells[1].style, style) << "B" << row + 1;
		EXPECT_EQ(cells[2].style, style) << "C" << row + 1;
		EXPECT_NE(cells[3].style, style) << "D" << row + 1;
	}
	const std::vector<Cell> row8 = row_cells(xml, 7);
	ASSERT_FALSE(row8.empty());
	EXPECT_EQ(row8[0].text, L"6");

	// Rows 9 to 11: empty rows of one custom height, folded into row 9 by the reader, keep the height;
	// row 12 has the default height.
	const std::wstring height_style = row_style(xml, 8);
	EXPECT_FALSE(height_style.empty()) << "row 9 has no style";
	EXPECT_EQ(row_style(xml, 9), height_style) << "row 10";
	EXPECT_EQ(row_style(xml, 10), height_style) << "row 11";
	EXPECT_NE(row_style(xml, 11), height_style) << "row 12";
	const std::vector<Cell> row12 = row_cells(xml, 11);
	ASSERT_FALSE(row12.empty());
	EXPECT_EQ(row12[0].text, L"7");

	// Rows 13 to 1112: 1100 empty rows of one custom height, folded into row 13; rows 1113 to 1199 are
	// missing. A1200 stays in row 1200.
	// The row element of row 13 is repeated at most 1023 times (XlsxConverter::add_folded_rows): rows 13 to 1035 keep the
	// height, rows 1036 to 1112 are written as default rows (a known limit, as on main).
	const std::wstring height_style_13 = row_style(xml, 12);
	EXPECT_FALSE(height_style_13.empty()) << "row 13 has no style";
	EXPECT_EQ(row_style(xml, 1034), height_style_13) << "row 1035";
	EXPECT_NE(row_style(xml, 1035), height_style_13) << "row 1036";
	const std::vector<Cell> row1200 = row_cells(xml, 1199);
	ASSERT_FALSE(row1200.empty()) << "no row 1200";
	EXPECT_EQ(row1200[0].text, L"8") << "A1200";

	// Row 1201 is hidden; rows 1202 and 1203 hold the same run of styled empty cells and are visible. The reader
	// folds them into row 1201 although the visibility differs: they must not become hidden.
	const std::wstring row1201 = row_xml(xml, 1200);
	EXPECT_NE(row1201.substr(0, row1201.find(L'>')).find(L"table:visibility=\"collapse\""), std::wstring::npos)
		<< "row 1201 is not hidden";
	for (size_t row = 1201; row < 1203; ++row)
	{
		const std::wstring row_element = row_xml(xml, row);
		ASSERT_FALSE(row_element.empty()) << "no row " << row + 1;
		EXPECT_EQ(row_element.substr(0, row_element.find(L'>')).find(L"table:visibility=\"collapse\""), std::wstring::npos)
			<< "row " << row + 1 << " is hidden";
	}
	const std::vector<Cell> row1204 = row_cells(xml, 1203);
	ASSERT_FALSE(row1204.empty()) << "no row 1204";
	EXPECT_EQ(row1204[0].text, L"9") << "A1204";

	// Row 1205 has a custom height, rows 1206 to 1208 are empty without attributes (the reader folds 1207 and 1208 into
	// 1206): none of them takes the height of row 1205.
	const std::wstring height_style_1205 = row_style(xml, 1204);
	EXPECT_FALSE(height_style_1205.empty()) << "row 1205 has no style";
	EXPECT_NE(row_style(xml, 1205), height_style_1205) << "row 1206";
	EXPECT_NE(row_style(xml, 1206), height_style_1205) << "row 1207";
	EXPECT_NE(row_style(xml, 1207), height_style_1205) << "row 1208";
	const std::vector<Cell> row1209 = row_cells(xml, 1208);
	ASSERT_FALSE(row1209.empty()) << "no row 1209";
	EXPECT_EQ(row1209[0].text, L"10") << "A1209";

	// Row 1210 has a style and a custom height, rows 1211 to 1213 the same style without a height (the reader folds 1212
	// and 1213 into 1211): none of them takes the height of row 1210.
	const std::wstring height_style_1210 = row_style(xml, 1209);
	EXPECT_FALSE(height_style_1210.empty()) << "row 1210 has no style";
	for (size_t row = 1210; row < 1213; ++row)
		EXPECT_NE(row_style(xml, row), height_style_1210) << "row " << row + 1;
	const std::vector<Cell> row1214 = row_cells(xml, 1213);
	ASSERT_FALSE(row1214.empty()) << "no row 1214";
	EXPECT_EQ(row1214[0].text, L"11") << "A1214";
}

// Rows 1 to 3 hold inline strings and rows 8 and 9 a formula without a cached value followed by empty cells, all of one
// style. The XLSX reader must not fold them (Euro-Office/core#417), and the folded-row repeat must not copy a row with
// content: every row keeps its own text and formula.
TEST(XlsxToOds, RowsWithInlineTextOrFormulasAreNotRepeated)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_repeated_rows_with_content.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	const std::vector<std::vector<std::wstring>> expected = {
		{L"Item", L"Region", L"Amount"}, {L"apple", L"East", L"five"}, {L"pear", L"West", L"six"}};
	for (size_t row = 0; row < expected.size(); ++row)
	{
		const std::vector<Cell> cells = row_cells(xml, row, 3);
		ASSERT_EQ(cells.size(), 3u) << "row " << row + 1;
		for (size_t col = 0; col < 3; ++col)
			EXPECT_EQ(cells[col].text, expected[row][col]) << static_cast<char>('A' + col) << row + 1;
	}

	const std::wstring row8 = row_xml(xml, 7);
	const std::wstring row9 = row_xml(xml, 8);
	ASSERT_FALSE(row8.empty()) << "no row 8";
	ASSERT_FALSE(row9.empty()) << "no row 9";
	EXPECT_NE(row8.find(L"[.B1]*2"), std::wstring::npos) << "A8: " << row8;
	EXPECT_NE(row9.find(L"[.B2]*2"), std::wstring::npos) << "A9: " << row9;
	EXPECT_EQ(row9.find(L"[.B1]*2"), std::wstring::npos) << "A9 repeats A8";
	EXPECT_EQ(attr_value(row8.substr(0, row8.find(L'>') + 1), L"table:number-rows-repeated"), L"") << "row 8 is repeated";

	const std::vector<Cell> row10 = row_cells(xml, 9);
	ASSERT_FALSE(row10.empty()) << "no row 10";
	EXPECT_EQ(row10[0].text, L"10") << "A10";
}
