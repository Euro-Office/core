// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: the number suffix of a list level whose w:lvlText shows more than one level
// ("%1.%2", "%1.%2)") is the text after the last placeholder, not the separator after the first one.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
std::wstring styles_part()
{
	static const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/list_multilevel_suffix.docx");
	return dir.empty() ? std::wstring() : read_part(dir, L"styles.xml");
}

// Start tag of the numbered list level "level" in styles.xml, empty when there is none.
std::wstring level_tag(int level)
{
	const std::wstring styles = styles_part();
	const std::wstring key = L"<text:list-level-style-number text:level=\"" + std::to_wstring(level) + L"\"";
	const size_t start = styles.find(key);
	if (start == std::wstring::npos)
		return std::wstring();
	const size_t end = styles.find(L'>', start);
	return end == std::wstring::npos ? std::wstring() : styles.substr(start, end - start + 1);
}
}

// Level 1 "%1)": one level, suffix ")".
TEST(DocxToOdtListNumberSuffix, SingleLevel)
{
	const std::wstring tag = level_tag(1);
	ASSERT_FALSE(tag.empty()) << "no list level 1 in styles.xml";
	EXPECT_NE(tag.find(L"style:num-suffix=\")\""), std::wstring::npos) << tag;
}

// Level 2 "%1.%2": nothing follows the last number, so there is no suffix.
TEST(DocxToOdtListNumberSuffix, NoSuffix)
{
	const std::wstring tag = level_tag(2);
	ASSERT_FALSE(tag.empty()) << "no list level 2 in styles.xml";
	EXPECT_NE(tag.find(L"text:display-levels=\"2\""), std::wstring::npos) << tag;
	EXPECT_EQ(tag.find(L"style:num-suffix"), std::wstring::npos) << tag;
}

// Level 3 "%1.%2.%3.": suffix ".".
TEST(DocxToOdtListNumberSuffix, DotSuffix)
{
	const std::wstring tag = level_tag(3);
	ASSERT_FALSE(tag.empty()) << "no list level 3 in styles.xml";
	EXPECT_NE(tag.find(L"text:display-levels=\"3\""), std::wstring::npos) << tag;
	EXPECT_NE(tag.find(L"style:num-suffix=\".\""), std::wstring::npos) << tag;
}

// Level 4 "%1.%2.%3.%4)": suffix ")", not the "." between the numbers.
TEST(DocxToOdtListNumberSuffix, ParenthesisSuffix)
{
	const std::wstring tag = level_tag(4);
	ASSERT_FALSE(tag.empty()) << "no list level 4 in styles.xml";
	EXPECT_NE(tag.find(L"text:display-levels=\"4\""), std::wstring::npos) << tag;
	EXPECT_NE(tag.find(L"style:num-suffix=\")\""), std::wstring::npos) << tag;
}

// Level 5 "%1.%2.%3.%4.%5. ": the space after the dot stays in the suffix.
TEST(DocxToOdtListNumberSuffix, WhitespaceSuffix)
{
	const std::wstring tag = level_tag(5);
	ASSERT_FALSE(tag.empty()) << "no list level 5 in styles.xml";
	EXPECT_NE(tag.find(L"text:display-levels=\"5\""), std::wstring::npos) << tag;
	EXPECT_NE(tag.find(L"style:num-suffix=\". \""), std::wstring::npos) << tag;
}
