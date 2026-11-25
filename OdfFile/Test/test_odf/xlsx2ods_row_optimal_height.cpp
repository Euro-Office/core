// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: which rows keep a fixed height.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	// The value of attribute name="..." inside tag, or an empty string.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + key.size();
		const size_t end = tag.find(L'"', start);
		return end == std::wstring::npos ? L"" : tag.substr(start, end - start);
	}

	// style:use-optimal-row-height of the table-row style named style_name, or an
	// empty string when the style or the attribute is missing.
	std::wstring optimal_row_height(const std::wstring& content, const std::wstring& style_name)
	{
		size_t pos = 0;
		while ((pos = content.find(L"<style:style ", pos)) != std::wstring::npos)
		{
			const size_t tag_end = content.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				return L"";
			const std::wstring tag = content.substr(pos, tag_end - pos);
			pos = tag_end;
			if (attribute(tag, L"style:name") != style_name || attribute(tag, L"style:family") != L"table-row")
				continue;
			const size_t props = content.find(L"<style:table-row-properties ", tag_end);
			const size_t style_end = content.find(L"</style:style>", tag_end);
			if (props == std::wstring::npos || style_end == std::wstring::npos || props > style_end)
				return L"";
			const size_t props_end = content.find(L'>', props);
			return attribute(content.substr(props, props_end - props), L"style:use-optimal-row-height");
		}
		return L"";
	}

	// style:use-optimal-row-height of the first rows of the first sheet, in order.
	std::vector<std::wstring> converted_row_optimal_heights(const std::wstring& fixture, size_t rows)
	{
		std::vector<std::wstring> result;
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return result;
		const std::wstring content = read_part(dir, L"content.xml");
		if (!is_well_formed(content))
			return result;
		size_t pos = 0;
		while (result.size() < rows && (pos = content.find(L"<table:table-row ", pos)) != std::wstring::npos)
		{
			const size_t tag_end = content.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				break;
			const std::wstring tag = content.substr(pos, tag_end - pos);
			result.push_back(optimal_row_height(content, attribute(tag, L"table:style-name")));
			pos = tag_end;
		}
		return result;
	}
}

// Row 1 has a row format (customFormat with s) and a stored height but no
// customHeight: the height stays automatic, as LibreOffice reads it. Row 2 has
// customHeight and keeps its fixed height; row 3 has only a height.
TEST(XlsxToOds, RowCustomFormatKeepsOptimalHeight)
{
	const std::vector<std::wstring> rows =
		converted_row_optimal_heights(L"ExampleFiles/row_custom_format_height.xlsx", 3);
	ASSERT_EQ(rows.size(), 3u) << "no well-formed content.xml with three rows";
	EXPECT_EQ(rows[0], L"true") << "row with customFormat but without customHeight";
	EXPECT_EQ(rows[1], L"false") << "row with customHeight";
	EXPECT_EQ(rows[2], L"true") << "row with only a height";
}
