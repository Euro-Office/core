// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of the sheet scaling attributes of
// style:page-layout-properties (fit to width/height, fit to a page count).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, each with its own page layout:
	// 1 style:scale-to-X="1" style:scale-to-Y="1"
	// 2 style:scale-to-X="1"
	// 3 style:scale-to-Y="2"
	// 4 style:scale-to-pages="3"
	// 5 loext:scale-to-X="2" loext:scale-to-Y="3"
	// 6 style:scale-to="75%"
	// 7 as 1, but the table style has no style:table-properties
	// 8 as 1, with a tab colour
	// 9 style:scale-to-X="-1" style:scale-to-Y="2" (invalid X)
	// 10 style:scale-to-pages="-2" (invalid)
	std::wstring sheet_xml(int sheet)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_fit_to_page.ods");
		if (dir.empty())
			return std::wstring();
		return read_part(dir, L"xl/worksheets/sheet" + std::to_wstring(sheet) + L".xml");
	}

	// The start tag of the first element with this name, empty if none.
	std::wstring start_tag(const std::wstring& xml, const std::wstring& name)
	{
		size_t pos = xml.find(L"<" + name + L" ");
		if (pos == std::wstring::npos)
			pos = xml.find(L"<" + name + L"/");
		if (pos == std::wstring::npos)
			return std::wstring();
		const size_t end = xml.find(L">", pos);
		return end == std::wstring::npos ? std::wstring() : xml.substr(pos, end - pos + 1);
	}

	// Value of an attribute in a start tag, or "(none)" if it is absent.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"(none)";
		const size_t begin = pos + key.size();
		const size_t end = tag.find(L"\"", begin);
		return end == std::wstring::npos ? L"(none)" : tag.substr(begin, end - begin);
	}

	// sheetPr/pageSetUpPr/@fitToPage, or "(none)".
	std::wstring fit_to_page(const std::wstring& sheet)
	{
		const size_t end = sheet.find(L"</sheetPr>");
		if (end == std::wstring::npos)
			return L"(none)";
		const std::wstring tag = start_tag(sheet.substr(0, end), L"pageSetUpPr");
		return attribute(tag, L"fitToPage");
	}
}

TEST(OdsToXlsx, FitToWidthAndHeight)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"1");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"1");
}

// Only a width given: the height is unconstrained (0), not the XLSX default 1.
TEST(OdsToXlsx, FitToWidthOnly)
{
	const std::wstring sheet = sheet_xml(2);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"1");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"0");
}

TEST(OdsToXlsx, FitToHeightOnly)
{
	const std::wstring sheet = sheet_xml(3);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"0");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"2");
}

// XLSX has no "N pages in total"; like LibreOffice's own XLSX export this
// becomes one page wide and N pages high.
TEST(OdsToXlsx, FitToPageCount)
{
	const std::wstring sheet = sheet_xml(4);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"1");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"3");
}

// The loext: names written by LibreOffice before ODF 1.3.
TEST(OdsToXlsx, FitToWidthAndHeightLoext)
{
	const std::wstring sheet = sheet_xml(5);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"2");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"3");
}

// Control: a percentage scale is not fit to page.
TEST(OdsToXlsx, ScaleWithoutFitToPage)
{
	const std::wstring sheet = sheet_xml(6);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"(none)");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"scale"), L"75");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"(none)");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"(none)");
}

// A table style without style:table-properties still gets sheetPr for the fit.
TEST(OdsToXlsx, FitToPageWithoutTableProperties)
{
	const std::wstring sheet = sheet_xml(7);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	EXPECT_EQ(attribute(start_tag(sheet, L"pageSetup"), L"fitToWidth"), L"1");
}

// With a tab colour, pageSetUpPr follows tabColor, as CT_SheetPr requires.
TEST(OdsToXlsx, FitToPageWithTabColor)
{
	const std::wstring sheet = sheet_xml(8);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const size_t tab_color = sheet.find(L"<tabColor ");
	const size_t setup = sheet.find(L"<pageSetUpPr ");
	ASSERT_NE(tab_color, std::wstring::npos);
	ASSERT_NE(setup, std::wstring::npos);
	EXPECT_LT(tab_color, setup);
}

// Invalid input: a negative page count is no limit, not a huge unsigned number.
TEST(OdsToXlsx, FitToPageNegativeWidth)
{
	const std::wstring sheet = sheet_xml(9);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"1");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"0");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"2");
}

// Invalid input: a negative scale-to-pages is ignored.
TEST(OdsToXlsx, FitToPageNegativePageCount)
{
	const std::wstring sheet = sheet_xml(10);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_EQ(fit_to_page(sheet), L"(none)");
	const std::wstring page_setup = start_tag(sheet, L"pageSetup");
	EXPECT_EQ(attribute(page_setup, L"fitToWidth"), L"(none)");
	EXPECT_EQ(attribute(page_setup, L"fitToHeight"), L"(none)");
}
