// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of the sheet tab colour: ODF 1.3 table:tab-color and the
// older tableooo:tab-color on style:table-properties become sheetPr/tabColor.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case:
	// 1 table:tab-color="#2b6cb0"
	// 2 tableooo:tab-color="#ffff00"
	// 3 table:tab-color="#00ff00" and tableooo:tab-color="#ff0000"
	// 4 no tab colour
	std::wstring sheet_xml(int sheet)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_tab_color.ods");
		if (dir.empty())
			return std::wstring();
		return read_part(dir, L"xl/worksheets/sheet" + std::to_wstring(sheet) + L".xml");
	}

	// sheetPr/tabColor/@rgb in upper case, "(none)" if there is no tabColor.
	std::wstring tab_color(const std::wstring& sheet)
	{
		const size_t end = sheet.find(L"</sheetPr>");
		const size_t pos = sheet.find(L"<tabColor ");
		if (end == std::wstring::npos || pos == std::wstring::npos || pos > end)
			return L"(none)";
		const std::wstring key = L" rgb=\"";
		const size_t tag_end = sheet.find(L">", pos);
		const size_t begin = sheet.find(key, pos);
		if (begin == std::wstring::npos || begin > tag_end)
			return L"(none)";
		const size_t value = begin + key.size();
		return sheet.substr(value, sheet.find(L"\"", value) - value);
	}
}

TEST(OdsToXlsx, TabColorOdf13)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));
	EXPECT_EQ(tab_color(sheet), L"2B6CB0");
}

TEST(OdsToXlsx, TabColorBeforeOdf13)
{
	const std::wstring sheet = sheet_xml(2);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_EQ(tab_color(sheet), L"FFFF00");
}

// Both attributes: the ODF 1.3 one wins.
TEST(OdsToXlsx, TabColorOdf13WinsOverOld)
{
	const std::wstring sheet = sheet_xml(3);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_EQ(tab_color(sheet), L"00FF00");
}

TEST(OdsToXlsx, NoTabColor)
{
	const std::wstring sheet = sheet_xml(4);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));
	EXPECT_EQ(tab_color(sheet), L"(none)");
}
