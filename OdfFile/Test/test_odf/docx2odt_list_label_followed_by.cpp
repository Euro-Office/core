// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: w:suff of a list level (what follows the number: tab, space or nothing) becomes
// text:label-followed-by of the level's style:list-level-label-alignment.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
std::wstring styles_part()
{
	static const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/list_label_followed_by.docx");
	return dir.empty() ? std::wstring() : read_part(dir, L"styles.xml");
}

// The text:list-style element named "name" in styles.xml, empty when there is none.
std::wstring list_style(const std::wstring& name)
{
	const std::wstring styles = styles_part();
	const size_t start = styles.find(L"<text:list-style style:name=\"" + name + L"\"");
	if (start == std::wstring::npos)
		return std::wstring();
	const std::wstring close = L"</text:list-style>";
	const size_t end = styles.find(close, start);
	return end == std::wstring::npos ? std::wstring() : styles.substr(start, end + close.size() - start);
}

// Start tag of the label alignment of level "level" in the list style "name", empty when there is none.
std::wstring label_alignment(const std::wstring& name, int level)
{
	const std::wstring style = list_style(name);
	const size_t start = style.find(L"<text:list-level-style-number text:level=\"" + std::to_wstring(level) + L"\"");
	if (start == std::wstring::npos)
		return std::wstring();
	const size_t level_end = style.find(L"</text:list-level-style-number>", start);
	const size_t tag = style.find(L"<style:list-level-label-alignment", start);
	if (tag == std::wstring::npos || tag > level_end)
		return std::wstring();
	const size_t end = style.find(L'>', tag);
	return end == std::wstring::npos ? std::wstring() : style.substr(tag, end - tag + 1);
}
}

TEST(DocxToOdtListLabelFollowedBy, Tab)
{
	const std::wstring tag = label_alignment(L"WWNum1", 1);
	ASSERT_FALSE(tag.empty()) << "no label alignment for WWNum1 level 1";
	EXPECT_NE(tag.find(L"text:label-followed-by=\"listtab\""), std::wstring::npos) << tag;
}

TEST(DocxToOdtListLabelFollowedBy, Space)
{
	const std::wstring tag = label_alignment(L"WWNum1", 2);
	ASSERT_FALSE(tag.empty()) << "no label alignment for WWNum1 level 2";
	EXPECT_NE(tag.find(L"text:label-followed-by=\"space\""), std::wstring::npos) << tag;
}

TEST(DocxToOdtListLabelFollowedBy, Nothing)
{
	const std::wstring tag = label_alignment(L"WWNum1", 3);
	ASSERT_FALSE(tag.empty()) << "no label alignment for WWNum1 level 3";
	EXPECT_NE(tag.find(L"text:label-followed-by=\"nothing\""), std::wstring::npos) << tag;
}

// No w:suff: Word's default is a tab, and ODF requires the attribute.
TEST(DocxToOdtListLabelFollowedBy, DefaultIsTab)
{
	const std::wstring tag = label_alignment(L"WWNum1", 4);
	ASSERT_FALSE(tag.empty()) << "no label alignment for WWNum1 level 4";
	EXPECT_NE(tag.find(L"text:label-followed-by=\"listtab\""), std::wstring::npos) << tag;
}

// A w:lvlOverride whose level has its own w:suff wins over the abstract numbering.
TEST(DocxToOdtListLabelFollowedBy, OverrideWins)
{
	const std::wstring tag = label_alignment(L"WWNum2", 1);
	ASSERT_FALSE(tag.empty()) << "no label alignment for WWNum2 level 1";
	EXPECT_NE(tag.find(L"text:label-followed-by=\"nothing\""), std::wstring::npos) << tag;
}
