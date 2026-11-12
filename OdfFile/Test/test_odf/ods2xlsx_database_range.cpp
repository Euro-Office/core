// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of a table:database-range that covers the whole sheet,
// as LibreOffice writes __Anonymous_Sheet_DB__ ranges: the table or autofilter
// ends at the last used row and column instead of row 1048576.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// Value of the first ref="..." after the start of the element, or "(none)".
	std::wstring ref_of(const std::wstring& xml, const std::wstring& element)
	{
		const size_t start = xml.find(L"<" + element + L" ");
		if (start == std::wstring::npos)
			return L"(none)";
		const std::wstring key = L" ref=\"";
		const size_t pos = xml.find(key, start);
		const size_t end = xml.find(L">", start);
		if (pos == std::wstring::npos || pos > end)
			return L"(none)";
		const size_t begin = pos + key.size();
		return xml.substr(begin, xml.find(L"\"", begin) - begin);
	}
}

// tdf99627.ods: 2999 used rows (header in row 1) and a whole-sheet range with a sort.
TEST(OdsToXlsx, WholeSheetTableEndsAtLastUsedRow)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/tdf99627.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring table = read_part(dir, L"xl/tables/table1.xml");
	ASSERT_FALSE(table.empty()) << "the database range is no longer a table";
	EXPECT_TRUE(is_well_formed(table));

	EXPECT_EQ(ref_of(table, L"table"), L"A1:F2999");
	EXPECT_EQ(ref_of(table, L"sortState"), L"A1:F2999");
	EXPECT_EQ(ref_of(table, L"sortCondition"), L"E1:E2999");
}

// A whole-sheet range with filter buttons and no filter or sort keeps its autofilter.
TEST(OdsToXlsx, WholeSheetFilterButtonsEndAtLastUsedCell)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/whole_sheet_filter_buttons.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring sheet = read_part(dir, L"xl/worksheets/sheet1.xml");
	ASSERT_FALSE(sheet.empty());
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(ref_of(sheet, L"autoFilter"), L"A1:C4");
}
