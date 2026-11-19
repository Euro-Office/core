// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: inline-string cells (t="inlineStr", text in <is>) next to each other in a row.

#include "gtest/gtest.h"

#include "common.h"

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

	// The text of the first max_cols cells of the row with the given 0-based index in the first table,
	// one entry per column (table:number-columns-repeated expanded).
	std::vector<std::wstring> row_texts(const std::wstring& xml, size_t row, size_t max_cols = 4)
	{
		std::vector<std::wstring> texts;
		size_t pos = xml.find(L"<table:table ");
		for (size_t i = 0; pos != std::wstring::npos && i <= row; ++i)
		{
			pos = xml.find(L"<table:table-row", pos + 1);
		}
		if (pos == std::wstring::npos)
			return texts;
		const size_t row_end = xml.find(L"</table:table-row>", pos);
		if (row_end == std::wstring::npos)
			return texts;

		size_t cell = xml.find(L"<table:table-cell", pos);
		while (cell != std::wstring::npos && cell < row_end && texts.size() < max_cols)
		{
			const size_t tag_end = xml.find(L'>', cell);
			if (tag_end == std::wstring::npos || tag_end > row_end)
				break;
			const std::wstring tag = xml.substr(cell, tag_end + 1 - cell);

			std::wstring text;
			size_t next = tag_end + 1;
			if (tag[tag.size() - 2] != L'/')
			{
				const size_t close = xml.find(L"</table:table-cell>", tag_end);
				if (close == std::wstring::npos || close > row_end)
					break;
				text = strip_tags(xml.substr(tag_end + 1, close - tag_end - 1));
				next = close;
			}

			size_t repeat = 1;
			const std::wstring attr = L"table:number-columns-repeated=\"";
			const size_t rep = tag.find(attr);
			if (rep != std::wstring::npos)
				repeat = std::stoul(tag.substr(rep + attr.size()));
			for (size_t i = 0; i < repeat && texts.size() < max_cols; ++i)
				texts.push_back(text);

			cell = xml.find(L"<table:table-cell", next);
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
