// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: w:position (raised or lowered text) becomes style:text-position in percent of the run's font size.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The automatic style element called `name`, or "" if there is none.
	std::wstring style_element(const std::wstring& content, const std::wstring& name)
	{
		if (name.empty())
			return L"";
		const size_t at = content.find(L"style:name=\"" + name + L"\"");
		if (at == std::wstring::npos)
			return L"";
		const size_t begin = content.rfind(L"<style:style ", at);
		const size_t start_tag_end = content.find(L'>', at);
		if (begin == std::wstring::npos || start_tag_end == std::wstring::npos)
			return L"";
		if (content[start_tag_end - 1] == L'/')
			return content.substr(begin, start_tag_end + 1 - begin);
		const size_t end = content.find(L"</style:style>", start_tag_end);
		return end == std::wstring::npos ? L"" : content.substr(begin, end - begin);
	}

	// style:text-position of the span that holds exactly `text`, "-" if it has none, "" if there is no such span.
	std::wstring text_position(const std::wstring& content, const std::wstring& text)
	{
		const size_t at = content.find(L">" + text + L"</text:span>");
		if (at == std::wstring::npos)
			return L"";
		const std::wstring attribute = L"text:style-name=\"";
		const size_t span = content.rfind(L"<text:span ", at);
		if (span == std::wstring::npos)
			return L"";
		const size_t value = content.find(attribute, span);
		if (value == std::wstring::npos || value > at)
			return L"-";
		const size_t value_begin = value + attribute.size();
		const std::wstring style = style_element(content, content.substr(value_begin, content.find(L'"', value_begin) - value_begin));

		const std::wstring position = L"style:text-position=\"";
		const size_t found = style.find(position);
		if (found == std::wstring::npos)
			return L"-";
		const size_t position_begin = found + position.size();
		return style.substr(position_begin, style.find(L'"', position_begin) - position_begin);
	}

	std::wstring convert_fixture(const std::wstring& file = L"ExampleFiles/run_position.docx")
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(file);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}
}

// Document default 10 pt, paragraph style Big 20 pt, character style Small 8 pt. The percentage is relative to the
// size the run has: its own w:sz, its character style's or its paragraph's.
TEST(DocxToOdt, RunPositionIsPercentOfTheRunFontSize)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(content));

	EXPECT_EQ(text_position(content, L"RAISED"), L"25% 100%");   // +3 pt at 12 pt
	EXPECT_EQ(text_position(content, L"LOWERED"), L"-20% 100%"); // -2 pt at the default 10 pt
	EXPECT_EQ(text_position(content, L"PSTYLE"), L"25% 100%");   // +5 pt at the paragraph style's 20 pt
	EXPECT_EQ(text_position(content, L"CSTYLE"), L"25% 100%");   // +2 pt at the character style's 8 pt
}

// STALE has no size of its own and follows BIGRUN (20 pt) in a 10 pt paragraph: its +2 pt are 20 %, not 10 %.
TEST(DocxToOdt, RunPositionUsesTheParagraphSizeNotThePreviousRun)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	EXPECT_EQ(text_position(content, L"STALE"), L"20% 100%");
}

// As in LibreOffice, a non-zero w:position replaces the run's w:vertAlign; 0 means no position.
TEST(DocxToOdt, RunPositionAndVerticalAlignment)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	EXPECT_EQ(text_position(content, L"BOTH"), L"25% 100%");
	EXPECT_EQ(text_position(content, L"ZERO"), L"sub 58%");
	EXPECT_EQ(text_position(content, L"PLAIN"), L"-");
}

// The size a run without one falls back to is the document's default or its paragraph's, never one that a numbering
// level (converted before the styles) or the section's footer (converted with the first paragraph) left behind.
// Document default 10 pt, paragraph style Big 20 pt, bullet level and the footer's last run 8 pt.
TEST(DocxToOdt, RunPositionIgnoresSizesOfListLevelsAndFooters)
{
	const std::wstring content = convert_fixture(L"ExampleFiles/run_position_streams.docx");
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	EXPECT_EQ(text_position(content, L"FIRST"), L"15% 100%");  // +3 pt at Big's 20 pt, not the footer's 8 pt
	EXPECT_EQ(text_position(content, L"LIST"), L"20% 100%");   // +2 pt at the default 10 pt, not the bullet's 8 pt
	EXPECT_EQ(text_position(content, L"NORMAL"), L"20% 100%");
}

// A tracked change of the paragraph mark converts the old run properties too; their size is not the paragraph's.
TEST(DocxToOdt, RunPositionIgnoresTrackedOldParagraphMarkSize)
{
	const std::wstring content = convert_fixture(L"ExampleFiles/run_position_streams.docx");
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	EXPECT_EQ(text_position(content, L"MARKCHANGE"), L"20% 100%"); // +2 pt at the default 10 pt, not the old 8 pt
}
