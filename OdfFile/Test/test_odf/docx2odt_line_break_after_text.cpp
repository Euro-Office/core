// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: a line break (w:br, no type or textWrapping) that follows text in the same run.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// True if a text:line-break lies between the first `before` and the next `after` in content.xml.
	bool line_break_between(const std::wstring& content, const std::wstring& before, const std::wstring& after)
	{
		const size_t b = content.find(before);
		if (b == std::wstring::npos)
			return false;
		const size_t a = content.find(after, b + before.size());
		const size_t br = content.find(L"<text:line-break", b + before.size());
		return a != std::wstring::npos && br != std::wstring::npos && br < a;
	}
}

// <w:r><w:t>ALPHA</w:t><w:br/><w:t>BETA</w:t></w:r>, and the same with w:type="textWrapping" (GAMMA, DELTA).
TEST(DocxToOdt, LineBreakAfterTextInSameRun)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/line_break_after_text.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_TRUE(line_break_between(content, L"ALPHA", L"BETA"));
	EXPECT_TRUE(line_break_between(content, L"GAMMA", L"DELTA"));
}

// <w:r><w:t>BEFORE</w:t><w:br w:type="page"/></w:r>: a page break after text does not become a line break.
// It still takes the deferred path, which today writes no break at all; this only checks it is no line break.
TEST(DocxToOdt, PageBreakAfterTextIsNoLineBreak)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/page_break_after_text.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_NE(content.find(L"BEFORE"), std::wstring::npos);
	EXPECT_NE(content.find(L"AFTER"), std::wstring::npos);
	EXPECT_EQ(content.find(L"<text:line-break"), std::wstring::npos);
}

// <w:r><w:t>BEFORE</w:t><w:br w:type="column"/></w:r>: the column break type has the value 0, which convert(CRun) maps
// to a line break for a break before any text. After text it must keep the deferred path and not become a line break.
TEST(DocxToOdt, ColumnBreakAfterTextIsNoLineBreak)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/column_break_after_text.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_NE(content.find(L"BEFORE"), std::wstring::npos);
	EXPECT_NE(content.find(L"AFTER"), std::wstring::npos);
	EXPECT_EQ(content.find(L"<text:line-break"), std::wstring::npos);
}
