// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: a number format with sections (positive;negative;zero) keeps every
// section, its colour and its own sign, written as LibreOffice writes it: one
// sub-style <name>P<n> per section but the last, the last section as the style
// itself, and a style:map per sub-style.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	struct DataStyle
	{
		std::wstring name;
		std::wstring xml;
	};

	std::wstring attribute(const std::wstring& xml, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = xml.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + key.size();
		return xml.substr(start, xml.find(L'"', start) - start);
	}

	// Every number:number-style, number:currency-style and number:percentage-style of styles.xml.
	std::vector<DataStyle> data_styles(const std::wstring& xml)
	{
		std::vector<DataStyle> styles;
		for (const std::wstring kind : {L"number-style", L"currency-style", L"percentage-style"})
		{
			const std::wstring open = L"<number:" + kind;
			const std::wstring close = L"</number:" + kind + L">";
			for (size_t pos = xml.find(open); pos != std::wstring::npos; pos = xml.find(open, pos + 1))
			{
				const size_t tag_end = xml.find(L'>', pos);
				const bool empty = tag_end != std::wstring::npos && xml[tag_end - 1] == L'/';
				const size_t end = empty ? tag_end + 1 : xml.find(close, pos) + close.size();
				const std::wstring element = xml.substr(pos, end - pos);
				styles.push_back({attribute(element.substr(0, element.find(L'>') + 1), L"style:name"), element});
			}
		}
		return styles;
	}

	const DataStyle* find_style(const std::vector<DataStyle>& styles, const std::wstring& name)
	{
		for (const DataStyle& style : styles)
			if (style.name == name)
				return &style;
		return nullptr;
	}

	// The style that maps the given condition to a sub-style and contains every given piece of XML.
	const DataStyle* find_mapped(const std::vector<DataStyle>& styles, const std::wstring& condition,
								 const std::vector<std::wstring>& pieces)
	{
		for (const DataStyle& style : styles)
		{
			if (style.xml.find(L"style:condition=\"" + condition + L"\"") == std::wstring::npos)
				continue;
			bool all = true;
			for (const std::wstring& piece : pieces)
				all = all && style.xml.find(piece) != std::wstring::npos;
			if (all)
				return &style;
		}
		return nullptr;
	}

	std::wstring map_target(const DataStyle& style, const std::wstring& condition)
	{
		const size_t pos = style.xml.find(L"style:condition=\"" + condition + L"\"");
		if (pos == std::wstring::npos)
			return L"";
		const size_t end = style.xml.find(L"/>", pos);
		return attribute(style.xml.substr(pos - 1, end - pos + 1), L"style:apply-style-name");
	}

	std::vector<DataStyle> converted_styles()
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/number_format_sections.xlsx");
		if (dir.empty())
			return {};
		const std::wstring xml = read_part(dir, L"styles.xml");
		EXPECT_TRUE(is_well_formed(xml));
		return data_styles(xml);
	}

	const std::wstring kRed = L"fo:color=\"#ff0000\"";
	const std::wstring kMinus = L"<number:text>-</number:text>";
}

// #,##0.00;[Red]\-#,##0.00 and 0.00;[Red]"-"0.00 (as the ODS reader writes it): the negative section is red and keeps
// its minus sign in front of the number.
TEST(XlsxToOdsNumberFormatSections, RedNegativeSection)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	int found = 0;
	for (const DataStyle& style : styles)
	{
		if (style.xml.find(kRed) == std::wstring::npos || style.xml.find(L"number:decimal-places=\"2\"") == std::wstring::npos ||
			map_target(style, L"value()&gt;=0").empty() || style.xml.find(L"<number:percentage-style") == 0)
			continue;
		++found;
		EXPECT_NE(style.xml.find(kMinus), std::wstring::npos) << style.xml;
		EXPECT_LT(style.xml.find(kMinus), style.xml.find(L"<number:number ")) << style.xml;

		const DataStyle* positive = find_style(styles, map_target(style, L"value()&gt;=0"));
		ASSERT_NE(positive, nullptr);
		EXPECT_EQ(positive->name, style.name + L"P0");
		EXPECT_EQ(positive->xml.find(L"fo:color"), std::wstring::npos);
		EXPECT_EQ(positive->xml.find(kMinus), std::wstring::npos);
		EXPECT_NE(positive->xml.find(L"number:decimal-places=\"2\""), std::wstring::npos);
	}
	EXPECT_EQ(found, 2);
}

// #,##0_);\(#,##0\) and the built-in format 38 (#,##0 ;[Red](#,##0)): negatives in parentheses.
TEST(XlsxToOdsNumberFormatSections, ParenthesesNegativeSection)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring open = L"<number:text>(</number:text>";
	const std::wstring close = L"<number:text>)</number:text>";
	const DataStyle* black = nullptr;
	const DataStyle* red = nullptr;
	for (const DataStyle& style : styles)
	{
		if (style.xml.find(open) == std::wstring::npos || style.xml.find(close) == std::wstring::npos ||
			map_target(style, L"value()&gt;=0").empty())
			continue;
		EXPECT_LT(style.xml.find(open), style.xml.find(L"<number:number "));
		EXPECT_GT(style.xml.find(close), style.xml.find(L"<number:number "));
		(style.xml.find(kRed) == std::wstring::npos ? black : red) = &style;
	}
	EXPECT_NE(black, nullptr) << "#,##0_);\\(#,##0\\)";
	EXPECT_NE(red, nullptr) << "built-in format 38";
}

// 0.00;[Blue]-0.00;"zero": three sections; the zero section is the style itself.
TEST(XlsxToOdsNumberFormatSections, ThreeSections)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	const DataStyle* main = find_mapped(styles, L"value()&gt;0", {L"<number:text>zero</number:text>"});
	ASSERT_NE(main, nullptr);
	EXPECT_EQ(main->xml.find(L"<number:number "), std::wstring::npos);
	EXPECT_EQ(map_target(*main, L"value()&gt;0"), main->name + L"P0");
	EXPECT_EQ(map_target(*main, L"value()&lt;0"), main->name + L"P1");

	const DataStyle* negative = find_style(styles, main->name + L"P1");
	ASSERT_NE(negative, nullptr);
	EXPECT_NE(negative->xml.find(L"fo:color=\"#0000ff\""), std::wstring::npos);
	EXPECT_NE(negative->xml.find(kMinus), std::wstring::npos);
	EXPECT_NE(find_style(styles, main->name + L"P0"), nullptr);
}

// [>=100][Green]0;[Red]0: a written condition is kept, colours on both sections.
TEST(XlsxToOdsNumberFormatSections, Condition)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	const DataStyle* main = find_mapped(styles, L"value()&gt;=100", {kRed});
	ASSERT_NE(main, nullptr);
	const DataStyle* first = find_style(styles, map_target(*main, L"value()&gt;=100"));
	ASSERT_NE(first, nullptr);
	EXPECT_NE(first->xml.find(L"fo:color=\"#00ff00\""), std::wstring::npos);
}

// _-* #,##0.00_-;\-* #,##0.00_-;_-* "-"??_-;_-@_-: the accounting zero section shows only "-".
TEST(XlsxToOdsNumberFormatSections, AccountingZeroSection)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	const DataStyle* main = nullptr;
	for (const DataStyle& style : styles)
	{
		if (!map_target(style, L"value()&gt;0").empty() && style.xml.find(L"<number:number ") == std::wstring::npos &&
			style.xml.find(kMinus) != std::wstring::npos)
			main = &style;
	}
	ASSERT_NE(main, nullptr);
	const DataStyle* negative = find_style(styles, map_target(*main, L"value()&lt;0"));
	ASSERT_NE(negative, nullptr);
	EXPECT_LT(negative->xml.find(kMinus), negative->xml.find(L"<number:number "));
}

// 0.0%;[Red]-0.0%: a percentage style keeps its sections too.
TEST(XlsxToOdsNumberFormatSections, Percentage)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	const DataStyle* main = find_mapped(styles, L"value()&gt;=0", {L"<number:percentage-style", kRed, kMinus});
	ASSERT_NE(main, nullptr);
	EXPECT_LT(main->xml.find(kMinus), main->xml.find(L"<number:number "));
	EXPECT_GT(main->xml.find(L"<number:text>%</number:text>"), main->xml.find(L"<number:number "));
	EXPECT_NE(find_style(styles, main->name + L"P0"), nullptr);
}

// A format with one section stays a single style; every map points at a written style; maps come last.
TEST(XlsxToOdsNumberFormatSections, SingleSectionAndMapTargets)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	bool single = false;
	for (const DataStyle& style : styles)
	{
		if (style.xml.find(L"number:decimal-places=\"3\"") != std::wstring::npos)
		{
			single = true;
			EXPECT_EQ(style.xml.find(L"<style:map"), std::wstring::npos) << style.xml;
			EXPECT_EQ(find_style(styles, style.name + L"P0"), nullptr);
		}
		const size_t map = style.xml.find(L"<style:map");
		if (map == std::wstring::npos)
			continue;
		EXPECT_EQ(style.xml.find(L"<number:", map), std::wstring::npos) << "a map before content: " << style.xml;
		for (size_t pos = map; pos != std::wstring::npos; pos = style.xml.find(L"<style:map", pos + 1))
		{
			const std::wstring target =
				attribute(style.xml.substr(pos, style.xml.find(L"/>", pos) - pos), L"style:apply-style-name");
			EXPECT_NE(find_style(styles, target), nullptr) << target;
		}
	}
	EXPECT_TRUE(single) << "0.000";
}

// 0\ "kg";-0\ "kg", #,##0.0 "EUR";[Red]-#,##0.0 "EUR" and "ca." 0.00000;"ca." -0.00000: the space between the number and
// its text (escaped or plain, after or before the number) is kept in every section.
TEST(XlsxToOdsNumberFormatSections, SpaceBetweenNumberAndText)
{
	const std::vector<DataStyle> styles = converted_styles();
	ASSERT_FALSE(styles.empty()) << "XLSX -> ODS conversion failed";

	struct Case
	{
		std::vector<std::wstring> negative;	// pieces of the negative (main) style, the first one is the text
		std::wstring positive;				// the text of the positive sub-style
		bool after;							// the text comes after the number
	};
	const Case cases[] = {
		{{L"<number:text> kg</number:text>", kMinus}, L"<number:text> kg</number:text>", true},
		{{L"<number:text> EUR</number:text>", kRed, kMinus}, L"<number:text> EUR</number:text>", true},
		{{L"<number:text>ca. -</number:text>"}, L"<number:text>ca. </number:text>", false},
	};
	for (const Case& c : cases)
	{
		const DataStyle* main = find_mapped(styles, L"value()&gt;=0", c.negative);
		ASSERT_NE(main, nullptr) << c.negative[0];
		const size_t number = main->xml.find(L"<number:number ");
		ASSERT_NE(number, std::wstring::npos) << main->xml;
		if (c.after)
			EXPECT_GT(main->xml.find(c.negative[0]), number) << main->xml;
		else
			EXPECT_LT(main->xml.find(c.negative[0]), number) << main->xml;

		const DataStyle* positive = find_style(styles, map_target(*main, L"value()&gt;=0"));
		ASSERT_NE(positive, nullptr) << main->xml;
		EXPECT_NE(positive->xml.find(c.positive), std::wstring::npos) << positive->xml;
		EXPECT_EQ(positive->xml.find(kMinus), std::wstring::npos) << positive->xml;
	}
}
