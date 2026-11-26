// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS conversions checked on the converted parts.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// True if a style:background-image with an xlink:href sits inside a
	// style:page-layout-properties element.
	bool has_page_background_image(const std::wstring& styles)
	{
		const size_t props = styles.find(L"<style:page-layout-properties");
		if (props == std::wstring::npos)
			return false;
		const size_t end = styles.find(L"</style:page-layout-properties>", props);
		const size_t image = styles.find(L"<style:background-image", props);
		if (end == std::wstring::npos || image == std::wstring::npos || image > end)
			return false;
		const size_t href = styles.find(L"xlink:href=\"", image);
		return href != std::wstring::npos && href < end;
	}
}

// A sheet background picture on a sheet without pageMargins, pageSetup or
// printOptions.
TEST(XlsxToOds, SheetBackgroundWithoutPageSettings)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_background_no_page_setup.xlsx");
	ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));
	EXPECT_TRUE(has_page_background_image(styles));
}

// Control: the same picture on a sheet with pageMargins.
TEST(XlsxToOds, SheetBackgroundWithPageMargins)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_background_with_margins.xlsx");
	ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));
	EXPECT_TRUE(has_page_background_image(styles));
}
