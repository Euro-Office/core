// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT conversions checked on the converted content.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// Plain-text and check-box content controls are sized from the current font
// size. A DOCX without styles.xml has no font size anywhere; the controls must
// still convert, with their value and the text around them.
TEST(DocxToOdt, TextContentControlWithoutFontSize)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/content_control_text_no_styles.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring xml = read_part(dir, L"content.xml");
	ASSERT_FALSE(xml.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"CCTEXT"), std::wstring::npos);
	EXPECT_NE(xml.find(L"END"), std::wstring::npos);
}

TEST(DocxToOdt, CheckBoxContentControlWithoutFontSize)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/content_control_checkbox_no_styles.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring xml = read_part(dir, L"content.xml");
	ASSERT_FALSE(xml.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"END"), std::wstring::npos);
}
