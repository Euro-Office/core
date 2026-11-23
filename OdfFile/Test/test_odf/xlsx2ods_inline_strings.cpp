// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: inline-string cells (t="inlineStr", text in <is>) next to each other in a row.

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
	std::wstring convert_xlsx_content(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}

	std::wstring strip_tags(const std::wstring& xml)
	{
		std::wstring text;
		bool in_tag = false;
		for (wchar_t c : xml)
		{
			if (c == L'<')
				in_tag = true;
			else if (c == L'>')
				in_tag = false;
			else if (!in_tag)
				text += c;
		}
		return text;
	}

	size_t attr_count(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring attr = name + L"=\"";
		const size_t pos = tag.find(attr);
		return pos == std::wstring::npos ? 1 : std::stoul(tag.substr(pos + attr.size()));
	}

	// The start of the next <table:table-row> element at or after pos (not <table:table-rows>).
	size_t find_row(const std::wstring& xml, size_t pos)
	{
		for (pos = xml.find(L"<table:table-row", pos); pos != std::wstring::npos; pos = xml.find(L"<table:table-row", pos + 1))
		{
			const wchar_t next = pos + 16 < xml.size() ? xml[pos + 16] : L'\0';
			if (next == L' ' || next == L'>' || next == L'/')
				return pos;
		}
		return std::wstring::npos;
	}

	// The start of the next table cell, covered or not, at or after pos.
	size_t find_cell(const std::wstring& xml, size_t pos)
	{
		return std::min(xml.find(L"<table:table-cell", pos), xml.find(L"<table:covered-table-cell", pos));
	}

	// The row element with the given 0-based index in the first table (table:number-rows-repeated expanded).
	std::wstring row_xml(const std::wstring& xml, size_t row)
	{
		const size_t table = xml.find(L"<table:table ");
		if (table == std::wstring::npos)
			return L"";
		size_t index = 0;
		for (size_t pos = find_row(xml, table); pos != std::wstring::npos; pos = find_row(xml, pos + 1))
		{
			const size_t tag_end = xml.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				return L"";
			const bool empty = xml[tag_end - 1] == L'/';
			const size_t end = empty ? tag_end + 1 : xml.find(L"</table:table-row>", tag_end);
			if (end == std::wstring::npos)
				return L"";
			index += attr_count(xml.substr(pos, tag_end + 1 - pos), L"table:number-rows-repeated");
			if (row < index)
				return xml.substr(pos, end - pos);
		}
		return L"";
	}

	// The text of the first max_cols cells of the row with the given 0-based index in the first table,
	// one entry per column (table:number-columns-repeated expanded, covered cells count as columns).
	std::vector<std::wstring> row_texts(const std::wstring& xml, size_t row, size_t max_cols = 4)
	{
		std::vector<std::wstring> texts;
		const std::wstring row_element = row_xml(xml, row);

		size_t cell = find_cell(row_element, 0);
		while (cell != std::wstring::npos && texts.size() < max_cols)
		{
			const size_t tag_end = row_element.find(L'>', cell);
			if (tag_end == std::wstring::npos)
				break;
			const std::wstring tag = row_element.substr(cell, tag_end + 1 - cell);
			const std::wstring close_tag = tag.compare(0, 25, L"<table:covered-table-cell") == 0
				? L"</table:covered-table-cell>" : L"</table:table-cell>";

			std::wstring text;
			size_t next = tag_end + 1;
			if (tag[tag.size() - 2] != L'/')
			{
				const size_t close = row_element.find(close_tag, tag_end);
				if (close == std::wstring::npos)
					break;
				text = strip_tags(row_element.substr(tag_end + 1, close - tag_end - 1));
				next = close;
			}

			const size_t repeat = attr_count(tag, L"table:number-columns-repeated");
			for (size_t i = 0; i < repeat && texts.size() < max_cols; ++i)
				texts.push_back(text);

			cell = find_cell(row_element, next);
		}
		return texts;
	}
}

// Each inline-string cell keeps its own text and column.
TEST(XlsxToOds, AdjacentInlineStringCellsKeepTheirText)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_inline_strings.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	const std::vector<std::vector<std::wstring>> expected = {
		{L"Item", L"Region", L"Amount", L""},
		{L"apple", L"East", L"5", L""},
		{L"styled one", L"styled two", L"", L""},
		{L"plain", L"bold part", L"", L""},
		{L"left", L"", L"right", L""},
	};
	for (size_t row = 0; row < expected.size(); ++row)
	{
		EXPECT_EQ(row_texts(xml, row), expected[row]) << "row " << row + 1;
	}
}

// The runs of a rich inline string stay apart: the bold run keeps its own text style.
TEST(XlsxToOds, InlineRichTextRunsKeptAfterAnotherInlineCell)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_inline_strings.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";

	const size_t bold = xml.find(L">bold</text:span>");
	ASSERT_NE(bold, std::wstring::npos) << "the bold run is missing or not a span";
	const size_t span = xml.rfind(L"<text:span", bold);
	ASSERT_NE(span, std::wstring::npos);
	EXPECT_NE(xml.substr(span, bold - span).find(L"text:style-name=\""), std::wstring::npos);
}

// A text or formula cell followed by empty cells of the same style keeps its row and its content:
// the empty cells are not folded into it, and the row is not merged into the empty row above.
TEST(XlsxToOds, CellContentKeptBeforeEmptyCellsOfTheSameStyle)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_inline_strings.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";

	const std::vector<std::wstring> empty_row = {L"", L"", L"", L""};
	EXPECT_EQ(row_texts(xml, 5), empty_row) << "row 6";
	EXPECT_EQ(row_texts(xml, 6), (std::vector<std::wstring>{L"txt", L"", L"", L""})) << "row 7";
	EXPECT_EQ(row_texts(xml, 7), empty_row) << "row 8";

	const std::wstring row9 = row_xml(xml, 8);
	const size_t first_cell = find_cell(row9, 0);
	ASSERT_NE(first_cell, std::wstring::npos) << "row 9 is missing";
	const std::wstring first_tag = row9.substr(first_cell, row9.find(L'>', first_cell) - first_cell);
	EXPECT_NE(first_tag.find(L"table:formula=\""), std::wstring::npos) << "row 9: the formula of A9 is missing";
}
