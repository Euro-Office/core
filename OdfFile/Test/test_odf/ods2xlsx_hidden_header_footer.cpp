// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX: a header or footer that the master page switches off with style:display="false"
// is not printed, nor are its left/first variants, and it takes no space on the page.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, all on the same page layout (1 cm margins, a header
	// and a footer 1 cm high):
	// 1 header and footer shown
	// 2 header and footer hidden, with a shown header-left and footer-first
	// 3 header shown, footer hidden
	std::wstring sheet_xml(int sheet)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_hidden_header_footer.ods");
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

	bool contains(const std::wstring& xml, const std::wstring& text)
	{
		return xml.find(text) != std::wstring::npos;
	}

	const double cm1 = 1 / 2.54;
}

TEST(OdsToXlsxHiddenHeaderFooter, ShownHeaderAndFooter)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_TRUE(contains(sheet, L"<oddHeader>")) << sheet;
	EXPECT_TRUE(contains(sheet, L"Shown header"));
	EXPECT_TRUE(contains(sheet, L"<oddFooter>"));
	EXPECT_TRUE(contains(sheet, L"Shown footer"));
	EXPECT_NEAR(margin(sheet, L"top"), cm1 + cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"bottom"), cm1 + cm1, 1e-3);
}

TEST(OdsToXlsxHiddenHeaderFooter, HiddenHeaderAndFooter)
{
	const std::wstring sheet = sheet_xml(2);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_FALSE(contains(sheet, L"<headerFooter")) << sheet;
	EXPECT_FALSE(contains(sheet, L"Hidden"));
	EXPECT_NEAR(margin(sheet, L"top"), cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"bottom"), cm1, 1e-3);
}

TEST(OdsToXlsxHiddenHeaderFooter, HiddenFooterOnly)
{
	const std::wstring sheet = sheet_xml(3);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	EXPECT_TRUE(contains(sheet, L"Shown header")) << sheet;
	EXPECT_FALSE(contains(sheet, L"<oddFooter"));
	EXPECT_FALSE(contains(sheet, L"Hidden"));
	EXPECT_NEAR(margin(sheet, L"top"), cm1 + cm1, 1e-3);
	EXPECT_NEAR(margin(sheet, L"bottom"), cm1, 1e-3);
}
