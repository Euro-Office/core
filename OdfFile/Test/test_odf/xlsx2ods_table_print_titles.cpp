// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: a sheet with a table part (a formatted table) keeps its rows and
// columns as they are. The table's header row and first column must not become
// table:table-header-rows / table:table-header-columns (print titles), and
// column A must be written once.

#include "gtest/gtest.h"

#include "common.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace
{
	std::wstring convert_xlsx_content(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}

	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + key.size();
		const size_t end = tag.find(L'"', start);
		if (end == std::wstring::npos)
			return L"";
		return tag.substr(start, end - start);
	}

	struct Column
	{
		std::wstring style;
		int repeated;
	};

	// Every <table:table-column> in document order, also those inside a header block.
	std::vector<Column> columns(const std::wstring& xml)
	{
		std::vector<Column> result;
		const std::wstring open = L"<table:table-column";
		size_t pos = 0;
		while ((pos = xml.find(open, pos)) != std::wstring::npos)
		{
			const size_t after = pos + open.size();
			const size_t end = xml.find(L'>', after);
			if (end == std::wstring::npos)
				break;
			const wchar_t next = after < xml.size() ? xml[after] : L'\0';
			if (next == L' ' || next == L'/' || next == L'>')
			{
				const std::wstring tag = xml.substr(pos, end - pos);
				const std::wstring repeated = attribute(tag, L"table:number-columns-repeated");
				const std::string digits(repeated.begin(), repeated.end());
				const int count = digits.empty() ? 1 : std::atoi(digits.c_str());
				result.push_back({attribute(tag, L"table:style-name"), count});
			}
			pos = end;
		}
		return result;
	}

	// The style:column-width of a column style, as a number (all widths share one unit).
	double column_width(const std::wstring& xml, const std::wstring& style_name)
	{
		const size_t pos = xml.find(L" style:name=\"" + style_name + L"\"");
		if (pos == std::wstring::npos)
			return -1;
		const size_t end = xml.find(L"</style:style>", pos);
		if (end == std::wstring::npos)
			return -1;
		const std::wstring width = attribute(xml.substr(pos, end - pos), L"style:column-width");
		if (width.empty())
			return -1;
		return std::atof(std::string(width.begin(), width.end()).c_str());
	}
}

// Columns A, B and C have the widths 20, 5 and 30.
TEST(XlsxToOds, TablePartKeepsColumnsInPlace)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/table_part_column_widths.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_EQ(xml.find(L"<table:table-header-rows"), std::wstring::npos);
	EXPECT_EQ(xml.find(L"<table:table-header-columns"), std::wstring::npos);

	const std::vector<Column> cols = columns(xml);
	ASSERT_GE(cols.size(), 3u);
	EXPECT_EQ(cols[0].repeated, 1);
	EXPECT_NE(cols[0].style, cols[1].style) << "column A written twice";
	EXPECT_NE(cols[1].style, cols[2].style);

	const double a = column_width(xml, cols[0].style);
	const double b = column_width(xml, cols[1].style);
	const double c = column_width(xml, cols[2].style);
	EXPECT_GT(a, b) << "B must keep its own (narrow) width";
	EXPECT_GT(c, a) << "C must keep its own (wide) width";
}

// Columns A and B share one width (one <col min="1" max="2">), C is wider.
TEST(XlsxToOds, TablePartKeepsSharedFirstColumns)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/table_part_shared_first_columns.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_EQ(xml.find(L"<table:table-header-rows"), std::wstring::npos);
	EXPECT_EQ(xml.find(L"<table:table-header-columns"), std::wstring::npos);

	const std::vector<Column> cols = columns(xml);
	ASSERT_GE(cols.size(), 2u);
	EXPECT_EQ(cols[0].repeated, 2) << "A and B must be one column element repeated twice";
	EXPECT_NE(cols[0].style, cols[1].style);
	EXPECT_GT(column_width(xml, cols[1].style), column_width(xml, cols[0].style));
}
