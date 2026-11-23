// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of the page margins of style:page-layout-properties. LibreOffice prints
// with a 2 cm margin on each side the page layout leaves out; the XLSX must say so, or the editor
// takes Excel's narrower defaults and prints more on each page.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, each with its own page layout:
	// 1 no margins
	// 2 no margins, a header and a footer 1 cm high
	// 3 all four margins 1 cm
	// 4 only fo:margin-left="1cm"
	// 5 all four margins 1 cm, a header and a footer 1 cm high
	std::wstring sheet_xml(int sheet)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_page_margins.ods");
		if (dir.empty())
			return std::wstring();
		return read_part(dir, L"xl/worksheets/sheet" + std::to_wstring(sheet) + L".xml");
	}

	// pageMargins/@name in inches, or -1 if the element or the attribute is missing.
	double margin(const std::wstring& sheet, const std::wstring& name)
	{
		const size_t tag = sheet.find(L"<pageMargins ");
		if (tag == std::wstring::npos)
			return -1;
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = sheet.find(key, tag);
		if (pos == std::wstring::npos || pos > sheet.find(L">", tag))
			return -1;
		return std::stod(sheet.substr(pos + key.size()));
	}

	const double cm2 = 0.7875; // LibreOffice's default margin, 2 cm
	const double cm1 = 1 / 2.54;
}

TEST(OdsToXlsxPageMargins, NoMarginsGetLibreOfficeDefaults)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_NEAR(margin(sheet, L"left"), cm2, 1e-3) << sheet;
	EXPECT_NEAR(margin(sheet, L"right"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"top"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"bottom"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"header"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"footer"), cm2, 1e-3);
}

// The header and footer come between the default margins and the cells.
TEST(OdsToXlsxPageMargins, NoMarginsWithHeaderAndFooter)
{
	const std::wstring sheet = sheet_xml(2);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_NEAR(margin(sheet, L"top"), cm2 + cm1, 1e-3) << sheet;
	EXPECT_NEAR(margin(sheet, L"bottom"), cm2 + cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"header"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"footer"), cm2, 1e-3);
}

TEST(OdsToXlsxPageMargins, GivenMarginsUnchanged)
{
	const std::wstring sheet = sheet_xml(3);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_NEAR(margin(sheet, L"left"), cm1, 1e-3) << sheet;
	EXPECT_NEAR(margin(sheet, L"right"), cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"top"), cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"bottom"), cm1, 1e-3);
}

TEST(OdsToXlsxPageMargins, MissingSidesGetDefaults)
{
	const std::wstring sheet = sheet_xml(4);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_NEAR(margin(sheet, L"left"), cm1, 1e-3) << sheet;
	EXPECT_NEAR(margin(sheet, L"right"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"top"), cm2, 1e-3);
	EXPECT_NEAR(margin(sheet, L"bottom"), cm2, 1e-3);
}

TEST(OdsToXlsxPageMargins, GivenMarginsWithHeaderAndFooterUnchanged)
{
	const std::wstring sheet = sheet_xml(5);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_NEAR(margin(sheet, L"top"), cm1 + cm1, 1e-3) << sheet;
	EXPECT_NEAR(margin(sheet, L"bottom"), cm1 + cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"header"), cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"footer"), cm1, 1e-3);
}
