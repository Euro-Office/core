// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: a page background in a document whose section has no page
// size and no page margins (both optional).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// True if a style:page-layout-properties element has draw:fill="bitmap".
	bool has_page_bitmap_fill(const std::wstring& styles)
	{
		const size_t props = styles.find(L"<style:page-layout-properties");
		if (props == std::wstring::npos)
			return false;
		const size_t end = styles.find(L'>', props);
		const size_t fill = styles.find(L"draw:fill=\"bitmap\"", props);
		return end != std::wstring::npos && fill != std::wstring::npos && fill < end;
	}
}

// w:background with a picture and no w:color, and a w:sectPr with only w:cols.
TEST(DocxToOdt, ImagePageBackgroundWithoutPageSize)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/page_background_image_no_page_size.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));
	EXPECT_TRUE(has_page_bitmap_fill(styles));
	EXPECT_NE(read_part(dir, L"content.xml").find(L"BODY"), std::wstring::npos);
}
