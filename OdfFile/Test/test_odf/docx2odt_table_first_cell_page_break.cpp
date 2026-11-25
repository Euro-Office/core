// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: w:pageBreakBefore in the first paragraph of a table's first cell is a page break before the table.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// Value of `attribute` in the last `tag` start tag before the first `text`, or "" if there is none.
	std::wstring attribute_before(const std::wstring& content, const std::wstring& tag, const std::wstring& attribute,
		const std::wstring& text)
	{
		const size_t at = content.find(text);
		if (at == std::wstring::npos)
			return L"";
		const size_t begin = content.rfind(tag, at);
		if (begin == std::wstring::npos)
			return L"";
		const size_t end = content.find(L'>', begin);
		const size_t value = content.find(attribute + L"=\"", begin);
		if (value == std::wstring::npos || value > end)
			return L"";
		const size_t value_begin = value + attribute.size() + 2;
		return content.substr(value_begin, content.find(L'"', value_begin) - value_begin);
	}

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

	bool has_page_break(const std::wstring& style)
	{
		return style.find(L"fo:break-before=\"page\"") != std::wstring::npos;
	}

	std::wstring table_style(const std::wstring& content, const std::wstring& text)
	{
		return style_element(content, attribute_before(content, L"<table:table ", L"table:style-name", text));
	}

	// Style of the first table after `text` (for a table whose first cell starts with a nested table).
	std::wstring table_style_after(const std::wstring& content, const std::wstring& text)
	{
		const size_t at = content.find(text);
		if (at == std::wstring::npos)
			return L"";
		const size_t table = content.find(L"<table:table ", at);
		if (table == std::wstring::npos)
			return L"";
		const size_t end = content.find(L'>', table);
		const std::wstring attribute = L"table:style-name=\"";
		const size_t value = content.find(attribute, table);
		if (value == std::wstring::npos || value > end)
			return L"";
		const size_t value_begin = value + attribute.size();
		return style_element(content, content.substr(value_begin, content.find(L'"', value_begin) - value_begin));
	}

	// "" for a paragraph without a style.
	std::wstring paragraph_style(const std::wstring& content, const std::wstring& text)
	{
		return style_element(content, attribute_before(content, L"<text:p", L"text:style-name", text));
	}

	std::wstring convert_fixture()
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/table_first_cell_page_break.docx");
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}
}

// Table 1: FIRSTCELL and SECONDCELL both have w:pageBreakBefore. Only the first one starts the table on a new page
// (fo:break-before of a paragraph in a cell has no effect), so that break goes on the table and off the paragraph.
TEST(DocxToOdt, TableFirstCellPageBreakGoesOnTable)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(content));

	const std::wstring table = table_style(content, L"FIRSTCELL");
	ASSERT_FALSE(table.empty()) << "no style for the table of FIRSTCELL";
	EXPECT_TRUE(has_page_break(table)) << table;
	EXPECT_FALSE(has_page_break(paragraph_style(content, L"FIRSTCELL")));
	EXPECT_TRUE(has_page_break(paragraph_style(content, L"SECONDCELL")));
}

// Table 2 has no break. Table 3 starts with OUTER (no break) and holds a nested table whose first paragraph NESTED
// has w:pageBreakBefore: neither table gets a break, and NESTED keeps its own.
TEST(DocxToOdt, TableWithoutFirstCellPageBreakGetsNone)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring plain = table_style(content, L"PLAIN");
	const std::wstring outer = table_style(content, L"OUTER");
	const std::wstring nested = table_style(content, L"NESTED");
	ASSERT_FALSE(plain.empty() || outer.empty() || nested.empty()) << "a table has no style";
	EXPECT_NE(outer, nested);
	EXPECT_FALSE(has_page_break(plain)) << plain;
	EXPECT_FALSE(has_page_break(outer)) << outer;
	EXPECT_FALSE(has_page_break(nested)) << nested;
	EXPECT_TRUE(has_page_break(paragraph_style(content, L"NESTED")));
}

// Table 4 starts its first cell with a nested table whose first paragraph NESTFIRST has w:pageBreakBefore. That is
// the first paragraph of table 4 as well, so the break goes on table 4 (as in LibreOffice), not on the nested table
// and not on NESTFIRST.
TEST(DocxToOdt, TableStartingWithNestedTableGetsItsPageBreak)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring outer = table_style_after(content, L"BEFORE4");
	const std::wstring nested = table_style(content, L"NESTFIRST");
	ASSERT_FALSE(outer.empty() || nested.empty()) << "a table has no style";
	EXPECT_NE(outer, nested);
	EXPECT_TRUE(has_page_break(outer)) << outer;
	EXPECT_FALSE(has_page_break(paragraph_style(content, L"NESTFIRST")));
	EXPECT_FALSE(has_page_break(nested)) << nested;
}

// Table 5 starts its first cell with a floating nested table: it does not start the text of the cell, so table 5
// gets no break and NESTFLOAT keeps its own.
TEST(DocxToOdt, TableStartingWithFloatingTableGetsNoPageBreak)
{
	const std::wstring content = convert_fixture();
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring outer = table_style_after(content, L"BEFORE5");
	ASSERT_FALSE(outer.empty()) << "table 5 has no style";
	EXPECT_FALSE(has_page_break(outer)) << outer;
	EXPECT_TRUE(has_page_break(paragraph_style(content, L"NESTFLOAT")));
}
