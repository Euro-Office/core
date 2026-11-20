// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: empty cells that the XLSX does not write one by one (gaps, merged ranges).

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
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
}
