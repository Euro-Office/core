// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: a chart series without its own colour gets one from a fixed table
// of default colours. A chart can have more series than the table has entries;
// those series start over at the first colour, as in LibreOffice.

#include "gtest/gtest.h"

#include "common.h"

#include <map>
#include <string>
#include <vector>

namespace
{
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + key.size();
		const size_t end = tag.find(L'"', start);
		if (end == std::wstring::npos)
			return L"";
		return tag.substr(start, end - start);
	}

	// Start tags of all elements with the given name, in document order.
	std::vector<std::wstring> start_tags(const std::wstring& xml, const std::wstring& name)
	{
		std::vector<std::wstring> tags;
		const std::wstring open = L"<" + name + L" ";
		size_t pos = 0;
		while ((pos = xml.find(open, pos)) != std::wstring::npos)
		{
			const size_t end = xml.find(L'>', pos);
			if (end == std::wstring::npos)
				break;
			tags.push_back(xml.substr(pos, end - pos + 1));
			pos = end;
		}
		return tags;
	}

	// Stroke colour of each chart style, by style name.
	std::map<std::wstring, std::wstring> stroke_colors(const std::wstring& xml)
	{
		std::map<std::wstring, std::wstring> colors;
		size_t pos = 0;
		while ((pos = xml.find(L"<style:style ", pos)) != std::wstring::npos)
		{
			const size_t end = xml.find(L"</style:style>", pos);
			if (end == std::wstring::npos)
				break;
			const std::wstring style = xml.substr(pos, end - pos);
			const std::wstring name = attribute(style.substr(0, style.find(L'>') + 1), L"style:name");
			const std::vector<std::wstring> graphic = start_tags(style, L"style:graphic-properties");
			if (!name.empty() && !graphic.empty())
				colors[name] = attribute(graphic.front(), L"svg:stroke-color");
			pos = end;
		}
		return colors;
	}

	bool is_hex_color(const std::wstring& value)
	{
		if (value.size() != 7 || value[0] != L'#')
			return false;
		for (size_t i = 1; i < value.size(); ++i)
		{
			const wchar_t c = value[i];
			if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')))
				return false;
		}
		return true;
	}
} // namespace

TEST(XlsxToOdsChartManySeries, SeriesBeyondTheDefaultColorsStartOver)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/chart_line_70_series.xlsx");
	ASSERT_FALSE(dir.empty()) << "conversion failed";
	const std::wstring xml = read_part(dir, L"Object 1/content.xml");
	ASSERT_FALSE(xml.empty()) << "Object 1/content.xml is missing";

	const std::vector<std::wstring> series = start_tags(xml, L"chart:series");
	ASSERT_EQ(series.size(), 70u);

	const std::map<std::wstring, std::wstring> colors = stroke_colors(xml);
	std::vector<std::wstring> series_colors;
	for (const std::wstring& tag : series)
	{
		const auto it = colors.find(attribute(tag, L"chart:style-name"));
		ASSERT_NE(it, colors.end()) << "no style for " << std::string(tag.begin(), tag.end());
		EXPECT_TRUE(is_hex_color(it->second))
			<< "series " << series_colors.size() + 1 << " has stroke colour '"
			<< std::string(it->second.begin(), it->second.end()) << "'";
		series_colors.push_back(it->second);
	}

	// The default colours repeat every 11 series; series 67-70 continue that cycle.
	for (size_t i = 66; i < series_colors.size(); ++i)
		EXPECT_EQ(series_colors[i], series_colors[i - 11]) << "series " << i + 1;
}
