// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// PPTX -> ODP conversions checked on the converted content.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// A table refers to a built-in table style by its ID, and the package has no
// ppt/tableStyles.xml (an optional part). The table must still convert.
TEST(PptxToOdp, TableStyleIdWithoutTableStylesPart)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/table_builtin_style_no_part.pptx");
	ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

	const std::wstring xml = read_part(dir, L"content.xml");
	ASSERT_FALSE(xml.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"<table:table"), std::wstring::npos);
	for (const wchar_t* text : {L"H1", L"H3", L"A2", L"C2"})
		EXPECT_NE(xml.find(text), std::wstring::npos) << "missing cell text";
}
