// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: the sortState of a table, of an autoFilter or of a worksheet becomes the
// table:sort of the database range. One fixture sheet per case.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	const std::wstring& sort_content()
	{
		static const std::wstring xml = []()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/xlsx_sort_state.xlsx");
			return dir.empty() ? std::wstring() : read_part(dir, L"content.xml");
		}();
		return xml;
	}

	// The table:database-range elements whose range is on the given sheet.
	std::wstring database_ranges(const std::wstring& sheet)
	{
		const std::wstring& xml = sort_content();
		std::wstring result;
		size_t from = 0;
		while (true)
		{
			const size_t attr = xml.find(L"table:target-range-address=\"" + sheet + L".", from);
			if (attr == std::wstring::npos)
				break;
			const size_t start = xml.rfind(L"<table:database-range ", attr);
			size_t end = xml.find(L"<table:database-range ", attr);
			const size_t close = xml.find(L"</table:database-ranges>", attr);
			if (end == std::wstring::npos || (close != std::wstring::npos && close < end))
				end = close;
			if (start == std::wstring::npos || end == std::wstring::npos)
				break;
			result += xml.substr(start, end - start);
			from = end;
		}
		return result;
	}

	// The database range elements of database_ranges(), one per entry.
	std::vector<std::wstring> range_elements(const std::wstring& ranges)
	{
		std::vector<std::wstring> result;
		const std::wstring tag = L"<table:database-range ";
		for (size_t pos = ranges.find(tag); pos != std::wstring::npos;)
		{
			const size_t next = ranges.find(tag, pos + tag.size());
			result.push_back(ranges.substr(pos, next == std::wstring::npos ? std::wstring::npos : next - pos));
			pos = next;
		}
		return result;
	}

	size_t count_of(const std::wstring& text, const std::wstring& what)
	{
		size_t count = 0;
		for (size_t pos = text.find(what); pos != std::wstring::npos; pos = text.find(what, pos + what.size()))
			count++;
		return count;
	}

	class XlsxToOdsSort : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_FALSE(sort_content().empty()) << "XLSX -> ODS conversion failed";
			ASSERT_TRUE(is_well_formed(sort_content()));
		}
	};
}

// A table part: the keys count from the first column of the table, case sensitivity is kept.
TEST_F(XlsxToOdsSort, TablePart)
{
	const std::wstring range = database_ranges(L"Table");
	EXPECT_NE(range.find(L"<table:sort table:case-sensitive=\"true\">"
						 L"<table:sort-by table:field-number=\"1\" table:data-type=\"automatic\"/>"
						 L"<table:sort-by table:field-number=\"2\" table:data-type=\"automatic\" table:order=\"descending\"/>"
						 L"</table:sort>"), std::wstring::npos) << range;
}

// A sheet autoFilter that starts in column B; the cell colour key has no ODF counterpart.
TEST_F(XlsxToOdsSort, AutoFilter)
{
	const std::wstring range = database_ranges(L"Filter");
	EXPECT_NE(range.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:sort><table:sort-by table:field-number=\"2\" table:data-type=\"automatic\" "
						 L"table:order=\"descending\"/></table:sort>"), std::wstring::npos) << range;
	EXPECT_EQ(count_of(range, L"<table:sort-by "), 1u) << range;
}

// A worksheet-level sortState gets a database range of its own, without filter buttons.
TEST_F(XlsxToOdsSort, SheetLevel)
{
	const std::wstring range = database_ranges(L"Sheet");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 1u) << range;
	EXPECT_NE(range.find(L"table:target-range-address=\"Sheet.A1:Sheet.C6\""), std::wstring::npos) << range;
	EXPECT_EQ(range.find(L"table:display-filter-buttons"), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:sort><table:sort-by table:field-number=\"2\" table:data-type=\"automatic\"/>"
						 L"</table:sort>"), std::wstring::npos) << range;
}

// columnSort: the range sorts columns, the key is a row.
TEST_F(XlsxToOdsSort, Columns)
{
	const std::wstring range = database_ranges(L"Columns");
	EXPECT_NE(range.find(L"table:orientation=\"column\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:sort-by table:field-number=\"1\" table:data-type=\"automatic\" "
						 L"table:order=\"descending\"/>"), std::wstring::npos) << range;
}

// An autoFilter and a worksheet sortState on the same range: one database range with both.
TEST_F(XlsxToOdsSort, SameRangeAsAutoFilter)
{
	const std::wstring range = database_ranges(L"Both");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 1u) << range;
	EXPECT_NE(range.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:sort-by table:field-number=\"0\" table:data-type=\"automatic\"/>"),
			  std::wstring::npos) << range;
}

// An autoFilter with its header row and a worksheet sortState without it (as Excel writes it):
// one database range with the filter buttons and the sort.
TEST_F(XlsxToOdsSort, SortBelowHeaderRow)
{
	const std::wstring range = database_ranges(L"Body");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 1u) << range;
	EXPECT_NE(range.find(L"table:target-range-address=\"Body.A1:Body.C6\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:sort><table:sort-by table:field-number=\"1\" table:data-type=\"automatic\" "
						 L"table:order=\"descending\"/></table:sort>"), std::wstring::npos) << range;
}

// A key outside the range is dropped, the key inside is kept.
TEST_F(XlsxToOdsSort, KeyOutsideRange)
{
	const std::wstring range = database_ranges(L"Outside");
	EXPECT_EQ(count_of(range, L"<table:sort-by "), 1u) << range;
	EXPECT_NE(range.find(L"<table:sort-by table:field-number=\"1\" table:data-type=\"automatic\"/>"),
			  std::wstring::npos) << range;
}

// A table that sorts its columns and a worksheet sortState on the same range that sorts its
// rows: two database ranges, each with its own sort.
TEST_F(XlsxToOdsSort, TableAndSheetSortOnOneRange)
{
	const std::wstring range = database_ranges(L"TwoSorts");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 2u) << range;
	EXPECT_EQ(count_of(range, L"table:orientation=\"column\""), 1u) << range;
	EXPECT_EQ(count_of(range, L"<table:sort-by "), 3u) << range;
	EXPECT_NE(range.find(L"<table:sort><table:sort-by table:field-number=\"1\" table:data-type=\"automatic\"/>"
						 L"<table:sort-by table:field-number=\"0\" table:data-type=\"automatic\"/></table:sort>"),
			  std::wstring::npos) << range;
	// the column sort stays on the table, the row sort is on the new range, which has no filter buttons
	for (const std::wstring& element : range_elements(range))
	{
		const bool columns = element.find(L"table:orientation=\"column\"") != std::wstring::npos;
		EXPECT_EQ(element.find(L"table:name=\"Grid\"") != std::wstring::npos, columns) << element;
		if (!columns)
			EXPECT_EQ(element.find(L"table:display-filter-buttons"), std::wstring::npos) << element;
	}
}

// A table and a worksheet sortState with the same range and the same sort: one database range.
TEST_F(XlsxToOdsSort, TableAndSheetSameSort)
{
	const std::wstring range = database_ranges(L"SameSorts");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 1u) << range;
	EXPECT_EQ(count_of(range, L"<table:sort-by "), 1u) << range;
	EXPECT_NE(range.find(L"<table:sort-by table:field-number=\"2\" table:data-type=\"automatic\"/>"),
			  std::wstring::npos) << range;
}

// A sheet autoFilter and a worksheet sortState on other cells: LibreOffice keeps one anonymous
// database range per sheet, so the sort gets no second one; the autoFilter range stays.
TEST_F(XlsxToOdsSort, SheetSortNextToAutoFilter)
{
	const std::wstring range = database_ranges(L"FilterAndSort");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 1u) << range;
	EXPECT_EQ(count_of(range, L"table:name=\"__Anonymous_Sheet_DB__"), 1u) << range;
	EXPECT_NE(range.find(L"table:target-range-address=\"FilterAndSort.A1:FilterAndSort.C6\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << range;
	EXPECT_EQ(range.find(L"<table:sort"), std::wstring::npos) << range;
}

// A column sort one row below a sheet autoFilter is not the autoFilter's sort: its keys count
// rows from the sortState's first row. It is left out rather than shifted by one row.
TEST_F(XlsxToOdsSort, ColumnSortBelowAutoFilter)
{
	const std::wstring range = database_ranges(L"ColumnsBody");
	EXPECT_EQ(count_of(range, L"<table:database-range "), 1u) << range;
	EXPECT_NE(range.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << range;
	EXPECT_EQ(range.find(L"table:orientation"), std::wstring::npos) << range;
	EXPECT_EQ(range.find(L"<table:sort"), std::wstring::npos) << range;
}
