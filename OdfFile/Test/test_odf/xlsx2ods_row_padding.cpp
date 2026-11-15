// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: every row of a sheet ends at the last column of the sheet, so that a row formatted to the end
// of the sheet stays formatted to its end.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	struct sheet_shape
	{
		long columns = 0;                   // sum of the table:table-column repeats
		std::vector<long> row_cells;        // per table:table-row, sum of its cell repeats
		std::vector<std::wstring> row_last; // per table:table-row, start tag of its last cell
	};

	// True if xml has the element name at pos ("<table:table-row" but not "<table:table-rows").
	bool element_at(const std::wstring& xml, size_t pos, const std::wstring& name)
	{
		if (xml.compare(pos, name.size(), name) != 0)
			return false;
		const wchar_t next = pos + name.size() < xml.size() ? xml[pos + name.size()] : L'\0';
		return next == L' ' || next == L'/' || next == L'>';
	}

	long repeats(const std::wstring& tag, const std::wstring& attribute)
	{
		const size_t at = tag.find(L" " + attribute + L"=\"");
		if (at == std::wstring::npos)
			return 1;
		return std::stol(tag.substr(at + attribute.size() + 3));
	}

	// The columns and rows of the first table of content.xml.
	sheet_shape read_sheet(const std::wstring& content)
	{
		sheet_shape shape;
		size_t pos = content.find(L"<table:table ");
		const size_t end = content.find(L"</table:table>", pos);
		if (pos == std::wstring::npos || end == std::wstring::npos)
			return shape;

		bool in_row = false;
		for (pos = content.find(L'<', pos + 1); pos < end; pos = content.find(L'<', pos + 1))
		{
			const size_t close = content.find(L'>', pos);
			const std::wstring tag = content.substr(pos, close - pos + 1);
			if (element_at(content, pos, L"<table:table-column"))
				shape.columns += repeats(tag, L"table:number-columns-repeated");
			else if (element_at(content, pos, L"<table:table-row"))
			{
				shape.row_cells.push_back(0);
				shape.row_last.push_back(L"");
				in_row = tag[tag.size() - 2] != L'/';
			}
			else if (element_at(content, pos, L"</table:table-row"))
				in_row = false;
			else if (in_row && (element_at(content, pos, L"<table:table-cell") ||
								element_at(content, pos, L"<table:covered-table-cell")))
			{
				shape.row_cells.back() += repeats(tag, L"table:number-columns-repeated");
				shape.row_last.back() = tag;
			}
		}
		return shape;
	}

	std::wstring style_name(const std::wstring& tag)
	{
		const std::wstring attribute = L" table:style-name=\"";
		const size_t at = tag.find(attribute);
		if (at == std::wstring::npos)
			return L"";
		const size_t start = at + attribute.size();
		return tag.substr(start, tag.find(L'"', start) - start);
	}

	void expect_rows_end_at_last_column(const wchar_t* fixture, long columns, long declared_columns)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";
		const std::wstring content = read_part(dir, L"content.xml");
		ASSERT_FALSE(content.empty()) << "content.xml is missing";
		EXPECT_TRUE(is_well_formed(content));

		const sheet_shape shape = read_sheet(content);
		EXPECT_EQ(shape.columns, declared_columns);
		ASSERT_GE(shape.row_cells.size(), 2u);
		for (size_t i = 0; i < shape.row_cells.size(); i++)
			EXPECT_EQ(shape.row_cells[i], columns) << "row element " << i + 1;

		// row 2 is filled: its trailing cells carry the row's cell style
		const std::wstring row_style = style_name(shape.row_last[1]);
		EXPECT_FALSE(row_style.empty());
		EXPECT_GT(repeats(shape.row_last[1], L"table:number-columns-repeated"), 1);
	}
}

// All 16384 columns declared, as in a sheet with a row or column filled to its end: no extra column after XFD,
// and each row (also the filled one) has 16384 cells, not its cells plus 1024.
TEST(XlsxToOds, RowsEndAtLastColumnOfFullWidthSheet)
{
	expect_rows_end_at_last_column(L"ExampleFiles/row_format_all_columns.xlsx", 16384, 16384);
}

// No column declarations: the sheet gets 1024 default columns and each row 1024 cells.
TEST(XlsxToOds, RowsEndAtLastColumnOfDefaultSheet)
{
	expect_rows_end_at_last_column(L"ExampleFiles/row_format_default_columns.xlsx", 1024, 1024);
}

// A malformed <col> past XFD (max 20000) keeps its column declaration, but no row gets cells past XFD.
TEST(XlsxToOds, RowsEndAtXfdWhenColumnsGoPastIt)
{
	expect_rows_end_at_last_column(L"ExampleFiles/row_format_columns_past_last.xlsx", 16384, 20000);
}
