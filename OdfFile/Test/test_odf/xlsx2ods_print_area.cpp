// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS conversions checked on the converted content.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	std::wstring convert_xlsx_content(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}
}

// A print area that names no sheet by index (localSheetId is optional).
TEST(XlsxToOds, PrintAreaWithoutLocalSheetId)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/print_area_no_local_sheet.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"table:name=\"Sheet1\""), std::wstring::npos);
	EXPECT_NE(xml.find(L"_xlnm.Print_Area"), std::wstring::npos);
}

// A print area whose localSheetId is not a sheet of the workbook (7, and 1,
// the first index past the only sheet): the defined name is kept as a
// document-level named range, and no sheet gets the print range.
class PrintAreaWithUnknownLocalSheetId : public ::testing::TestWithParam<const wchar_t*>
{
};

TEST_P(PrintAreaWithUnknownLocalSheetId, KeepsTheDefinedName)
{
	const std::wstring xml = convert_xlsx_content(GetParam());
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"table:name=\"Sheet1\""), std::wstring::npos);
	EXPECT_NE(xml.find(L"table:name=\"_xlnm.Print_Area\""), std::wstring::npos);
	EXPECT_EQ(xml.find(L"table:print-ranges="), std::wstring::npos);
}

INSTANTIATE_TEST_SUITE_P(XlsxToOds, PrintAreaWithUnknownLocalSheetId,
						 ::testing::Values(L"ExampleFiles/print_area_bad_local_sheet.xlsx",
										   L"ExampleFiles/print_area_local_sheet_past_end.xlsx"));

// Control: a print area on an existing sheet still becomes that sheet's
// table:print-ranges.
TEST(XlsxToOds, PrintAreaWithLocalSheetId)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/print_area_local_sheet.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"table:print-ranges="), std::wstring::npos);
	EXPECT_NE(xml.find(L"table:name=\"_xlnm.Print_Area\""), std::wstring::npos);
}
