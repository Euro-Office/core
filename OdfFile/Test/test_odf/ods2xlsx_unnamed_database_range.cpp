// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of a table:database-range without table:name. The name is optional in
// ODF; LibreOffice reads such a range as the sheet's anonymous range, so its filter and sort
// must reach the XLSX like those of a named range.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// Sheets Filter and Sort (A1:C5 with a header row) each hold one unnamed database range:
	// Filter: filter buttons and field 1 = East; Sort: sort by field 2, descending. Sheets Shared and
	// Empty: see their tests below.
	const std::wstring& xlsx_dir()
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_unnamed_database_range.ods");
		return dir;
	}

	// Value of the name="..." attribute of the <table> element of a table part, "(none)" if missing.
	std::wstring table_name(const std::wstring& xml)
	{
		const size_t start = xml.find(L"<table ");
		if (start == std::wstring::npos)
			return L"(none)";
		const std::wstring key = L" name=\"";
		const size_t pos = xml.find(key, start);
		if (pos == std::wstring::npos || pos > xml.find(L">", start))
			return L"(none)";
		const size_t begin = pos + key.size();
		return xml.substr(begin, xml.find(L"\"", begin) - begin);
	}
}

TEST(OdsToXlsx, UnnamedDatabaseRangeKeepsFilter)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring table = read_part(xlsx_dir(), L"xl/tables/table1.xml");
	ASSERT_FALSE(table.empty()) << "the unnamed range with a filter became no table";
	EXPECT_TRUE(is_well_formed(table));
	EXPECT_NE(table.find(L"<autoFilter ref=\"A1:C5\">"), std::wstring::npos) << table;
	EXPECT_NE(table.find(L"<filterColumn colId=\"1\">"), std::wstring::npos) << table;
	EXPECT_NE(table.find(L"val=\"East\""), std::wstring::npos) << table;
}

TEST(OdsToXlsx, UnnamedDatabaseRangeKeepsSort)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring table = read_part(xlsx_dir(), L"xl/tables/table2.xml");
	ASSERT_FALSE(table.empty()) << "the unnamed range with a sort became no table";
	EXPECT_TRUE(is_well_formed(table));
	EXPECT_NE(table.find(L"<sortState ref=\"A1:C5\""), std::wstring::npos) << table;
	EXPECT_NE(table.find(L"<sortCondition ref=\"C1:C5\" descending=\"1\"/>"), std::wstring::npos) << table;
}

// Both ranges are anonymous ranges of their sheets, and the two table parts get different names.
TEST(OdsToXlsx, UnnamedDatabaseRangesGetDistinctAnonymousNames)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring name1 = table_name(read_part(xlsx_dir(), L"xl/tables/table1.xml"));
	const std::wstring name2 = table_name(read_part(xlsx_dir(), L"xl/tables/table2.xml"));
	EXPECT_EQ(name1.rfind(L"__Anonymous_Sheet_DB__", 0), 0u) << name1;
	EXPECT_EQ(name2.rfind(L"__Anonymous_Sheet_DB__", 0), 0u) << name2;
	EXPECT_NE(name1, name2);
}

// Sheet Shared: named A1:C5 (the table part), named D1:F5 and unnamed G1:I5, all with filters. A sheet has one
// table part and one autoFilter here, so D1:F5 keeps the autoFilter with its own filter; the unnamed range must
// not widen it to D1:I5.
TEST(OdsToXlsx, UnnamedDatabaseRangeLeavesNamedAutoFilterAlone)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring sheet = read_part(xlsx_dir(), L"xl/worksheets/sheet3.xml");
	ASSERT_FALSE(sheet.empty());
	EXPECT_TRUE(is_well_formed(sheet));
	EXPECT_NE(sheet.find(L"<autoFilter ref=\"D1:F5\">"), std::wstring::npos) << sheet;
	EXPECT_NE(sheet.find(L"val=\"North\""), std::wstring::npos) << sheet;
	EXPECT_EQ(sheet.find(L"val=\"West\""), std::wstring::npos) << sheet;

	const std::wstring table = read_part(xlsx_dir(), L"xl/tables/table3.xml");
	EXPECT_NE(table.find(L"<autoFilter ref=\"A1:C5\">"), std::wstring::npos) << table;
	EXPECT_NE(table.find(L"val=\"East\""), std::wstring::npos) << table;
}

// Sheet Empty: an unnamed range without filter, sort or buttons keeps nothing and must not turn into a defined
// name __Anonymous_Sheet_DB__* that the user would see in the name list.
TEST(OdsToXlsx, EmptyUnnamedDatabaseRangeAddsNoDefinedName)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring workbook = read_part(xlsx_dir(), L"xl/workbook.xml");
	ASSERT_FALSE(workbook.empty());
	EXPECT_EQ(workbook.find(L"__Anonymous_Sheet_DB__"), std::wstring::npos) << workbook;
}

// Sheet TwoUnnamed: two unnamed ranges with filters, A1:C5 and D1:F5. LibreOffice keeps one anonymous range per
// sheet, the last one it reads, so only D1:F5 stays (as the table part) and A1:C5 adds no worksheet autoFilter.
TEST(OdsToXlsx, LastUnnamedDatabaseRangeOnASheetWins)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring sheet = read_part(xlsx_dir(), L"xl/worksheets/sheet5.xml");
	ASSERT_FALSE(sheet.empty());
	EXPECT_TRUE(is_well_formed(sheet));
	EXPECT_EQ(sheet.find(L"<autoFilter"), std::wstring::npos) << sheet;

	const std::wstring table = read_part(xlsx_dir(), L"xl/tables/table4.xml");
	ASSERT_FALSE(table.empty()) << "the last unnamed range became no table";
	EXPECT_NE(table.find(L"<autoFilter ref=\"D1:F5\">"), std::wstring::npos) << table;
	EXPECT_NE(table.find(L"val=\"North\""), std::wstring::npos) << table;
	EXPECT_EQ(table.find(L"val=\"East\""), std::wstring::npos) << table;
}

// Sheet Sorts: named A1:C5 with a filter (the table part), named D1:F5 with a sort, unnamed G1:I5 with a sort.
// A worksheet has at most one sortState; the named range keeps it.
TEST(OdsToXlsx, UnnamedDatabaseRangeAddsNoSecondSortState)
{
	ASSERT_FALSE(xlsx_dir().empty()) << "ODS -> XLSX conversion failed";
	const std::wstring sheet = read_part(xlsx_dir(), L"xl/worksheets/sheet6.xml");
	ASSERT_FALSE(sheet.empty());
	EXPECT_TRUE(is_well_formed(sheet));
	const size_t first = sheet.find(L"<sortState");
	ASSERT_NE(first, std::wstring::npos) << sheet;
	EXPECT_EQ(sheet.find(L"<sortState", first + 1), std::wstring::npos) << sheet;
	EXPECT_NE(sheet.find(L"<sortState ref=\"D1:F5\""), std::wstring::npos) << sheet;
}

// A database range whose address is one cell ("Cell.A1", no ':') is no range to filter or sort. The fixture's first
// range is such a named one with a filter, then an unnamed one-cell range with a filter, then a named A1:C5 range
// with field 1 = North on sheet Next. The conversion must not fail, and the one-cell filters must not end up
// on another range.
TEST(OdsToXlsx, OneCellDatabaseRangeIsSkipped)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_database_range_single_cell.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring cell_sheet = read_part(dir, L"xl/worksheets/sheet1.xml");
	ASSERT_FALSE(cell_sheet.empty());
	EXPECT_EQ(cell_sheet.find(L"<autoFilter"), std::wstring::npos) << cell_sheet;

	const std::wstring table = read_part(dir, L"xl/tables/table1.xml");
	ASSERT_FALSE(table.empty()) << "the A1:C5 range on sheet Next became no table";
	EXPECT_NE(table.find(L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><filters><filter val=\"North\"/>"
		L"</filters></filterColumn></autoFilter>"), std::wstring::npos) << table;
	EXPECT_EQ(table.find(L"val=\"East\""), std::wstring::npos) << table;
	EXPECT_EQ(table.find(L"val=\"x\""), std::wstring::npos) << table;
}
