// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT conversions checked on the converted content.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// Value of one attribute of the n-th (0-based) draw:control in content.xml,
	// or an empty string if there is no such control or attribute.
	std::wstring control_attribute(const std::wstring& xml, size_t n, const std::wstring& name)
	{
		size_t pos = 0;
		for (size_t i = 0; ; ++i)
		{
			pos = xml.find(L"<draw:control ", pos);
			if (pos == std::wstring::npos)
				return L"";
			if (i == n)
				break;
			++pos;
		}
		const size_t end = xml.find(L'>', pos);
		const std::wstring tag = xml.substr(pos, end == std::wstring::npos ? std::wstring::npos : end - pos);

		const std::wstring key = L" " + name + L"=\"";
		const size_t start = tag.find(key);
		if (start == std::wstring::npos)
			return L"";
		const size_t value = start + key.size();
		const size_t quote = tag.find(L'"', value);
		return quote == std::wstring::npos ? L"" : tag.substr(value, quote - value);
	}

	std::wstring convert_content(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}

	// The control in a document without any font size must get the size of the
	// same control in a document whose text is explicitly 12 pt (the default).
	void expect_default_control_size(const std::wstring& fixture, const std::wstring& reference,
									 const std::wstring& value)
	{
		const std::wstring xml = convert_content(fixture);
		ASSERT_FALSE(xml.empty()) << "DOCX -> ODT conversion failed";
		EXPECT_TRUE(is_well_formed(xml));
		EXPECT_NE(xml.find(L"form:current-value=\"" + value + L"\""), std::wstring::npos);
		EXPECT_NE(xml.find(L"END"), std::wstring::npos);

		const std::wstring ref = convert_content(reference);
		ASSERT_FALSE(ref.empty()) << "DOCX -> ODT conversion of the reference failed";

		const std::wstring width = control_attribute(xml, 0, L"svg:width");
		const std::wstring height = control_attribute(xml, 0, L"svg:height");
		ASSERT_FALSE(width.empty()) << "no svg:width on the control";
		ASSERT_FALSE(height.empty()) << "no svg:height on the control";
		EXPECT_EQ(width, control_attribute(ref, 0, L"svg:width"));
		EXPECT_EQ(height, control_attribute(ref, 0, L"svg:height"));
	}
}

// Plain-text and check-box content controls are sized from the current font
// size. A DOCX without styles.xml has no font size anywhere; the controls must
// still convert, with their value, the text around them and the default size.
TEST(DocxToOdt, TextContentControlWithoutFontSize)
{
	expect_default_control_size(L"ExampleFiles/content_control_text_no_styles.docx",
								L"ExampleFiles/content_control_text_12pt.docx", L"CCTEXT");
}

TEST(DocxToOdt, CheckBoxContentControlWithoutFontSize)
{
	expect_default_control_size(L"ExampleFiles/content_control_checkbox_no_styles.docx",
								L"ExampleFiles/content_control_checkbox_12pt.docx", L"CCBOX");
}

// A trailing line feed is removed from the control value; the rest of the value
// is kept, and the control is sized like one without the line feed.
TEST(DocxToOdt, ContentControlValueEndingInLineFeed)
{
	const std::wstring xml = convert_content(L"ExampleFiles/content_control_value_newline.docx");
	ASSERT_FALSE(xml.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	// control1 holds "ABCDEFGHIJKLMN" plus a line feed, control2 the same text without it.
	const std::wstring value = L"form:current-value=\"ABCDEFGHIJKLMN\"";
	const size_t first = xml.find(value);
	ASSERT_NE(first, std::wstring::npos);
	EXPECT_NE(xml.find(value, first + 1), std::wstring::npos);

	const std::wstring width = control_attribute(xml, 0, L"svg:width");
	ASSERT_FALSE(width.empty()) << "no svg:width on the control";
	EXPECT_EQ(width, control_attribute(xml, 1, L"svg:width"));
}

TEST(DocxToOdt, ShortContentControlValueEndingInLineFeed)
{
	const std::wstring xml = convert_content(L"ExampleFiles/content_control_short_value_newline.docx");
	ASSERT_FALSE(xml.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"form:current-value=\"SHORT\""), std::wstring::npos);
	EXPECT_NE(xml.find(L"END"), std::wstring::npos);
}
