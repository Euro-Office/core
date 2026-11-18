// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: a sheet fitted to a number of pages (sheetPr/pageSetUpPr
// fitToPage with pageSetup fitToWidth/fitToHeight) keeps the fit in the page
// style. The fixture has one sheet per case:
//   Fit1x1   fitToPage, fitToWidth="1" fitToHeight="1" scale="50"
//   Defaults fitToPage, no fitToWidth/fitToHeight (1 each by default)
//   Wide2    fitToPage, fitToWidth="2" fitToHeight="0" (no height limit)
//   Scale80  no fitToPage, fitToWidth="3" scale="80"

#include "gtest/gtest.h"

#include "common.h"

#include <string>

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

	// The start tag of the first element <element ...> in xml whose attribute
	// key has the given value, or an empty string.
	std::wstring find_tag(const std::wstring& xml, const std::wstring& element,
						  const std::wstring& key, const std::wstring& value)
	{
		const std::wstring open = L"<" + element + L" ";
		size_t pos = 0;
		while ((pos = xml.find(open, pos)) != std::wstring::npos)
		{
			const size_t end = xml.find(L'>', pos);
			if (end == std::wstring::npos)
				return L"";
			const std::wstring tag = xml.substr(pos, end - pos);
			if (attribute(tag, key) == value)
				return tag;
			pos = end;
		}
		return L"";
	}

	// The style:page-layout-properties start tag of the page style that the
	// sheet named sheet uses, or an empty string.
	std::wstring page_layout_properties(const std::wstring& content, const std::wstring& styles,
										const std::wstring& sheet)
	{
		const std::wstring table = find_tag(content, L"table:table", L"table:name", sheet);
		const std::wstring table_style =
			find_tag(content, L"style:style", L"style:name", attribute(table, L"table:style-name"));
		const std::wstring master = find_tag(styles, L"style:master-page", L"style:name",
											 attribute(table_style, L"style:master-page-name"));
		const std::wstring layout_name = attribute(master, L"style:page-layout-name");
		if (layout_name.empty())
			return L"";
		const std::wstring layout = find_tag(styles, L"style:page-layout", L"style:name", layout_name);
		const size_t start = styles.find(layout);
		const size_t end = styles.find(L"</style:page-layout>", start);
		const size_t props = styles.find(L"<style:page-layout-properties ", start);
		if (layout.empty() || props == std::wstring::npos || props > end)
			return L"";
		return styles.substr(props, styles.find(L'>', props) - props);
	}

	// scale-to-X / scale-to-Y in the style: (ODF 1.3) or loext: namespace.
	std::wstring scale_to(const std::wstring& props, const std::wstring& axis)
	{
		const std::wstring value = attribute(props, L"style:scale-to-" + axis);
		return value.empty() ? attribute(props, L"loext:scale-to-" + axis) : value;
	}

	struct Converted
	{
		std::wstring content;
		std::wstring styles;
	};

	const Converted& converted()
	{
		static Converted result = []
		{
			Converted out;
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/xlsx_fit_to_pages.xlsx");
			if (dir.empty())
				return out;
			out.content = read_part(dir, L"content.xml");
			out.styles = read_part(dir, L"styles.xml");
			return out;
		}();
		return result;
	}

	std::wstring sheet_properties(const std::wstring& sheet)
	{
		const Converted& odf = converted();
		if (!is_well_formed(odf.content) || !is_well_formed(odf.styles))
			return L"";
		return page_layout_properties(odf.content, odf.styles, sheet);
	}
}

// fitToPage with 1 x 1 pages: the page style fits the sheet to one page, and
// scale (which fitToPage switches off) is not written.
TEST(XlsxToOds, FitToPageOnePage)
{
	const std::wstring props = sheet_properties(L"Fit1x1");
	ASSERT_FALSE(props.empty()) << "no page style for sheet Fit1x1";
	EXPECT_EQ(scale_to(props, L"X"), L"1") << props;
	EXPECT_EQ(scale_to(props, L"Y"), L"1") << props;
	EXPECT_EQ(attribute(props, L"style:scale-to"), L"") << props;
}

// fitToPage without fitToWidth/fitToHeight: both default to one page.
TEST(XlsxToOds, FitToPageDefaultsToOnePage)
{
	const std::wstring props = sheet_properties(L"Defaults");
	ASSERT_FALSE(props.empty()) << "no page style for sheet Defaults";
	EXPECT_EQ(scale_to(props, L"X"), L"1") << props;
	EXPECT_EQ(scale_to(props, L"Y"), L"1") << props;
}

// fitToHeight="0" means no limit: only the width is fitted.
TEST(XlsxToOds, FitToPageZeroHeightHasNoLimit)
{
	const std::wstring props = sheet_properties(L"Wide2");
	ASSERT_FALSE(props.empty()) << "no page style for sheet Wide2";
	EXPECT_EQ(scale_to(props, L"X"), L"2") << props;
	EXPECT_EQ(scale_to(props, L"Y"), L"") << props;
}

// Without fitToPage, fitToWidth does not apply and scale stays.
TEST(XlsxToOds, ScaleWithoutFitToPage)
{
	const std::wstring props = sheet_properties(L"Scale80");
	ASSERT_FALSE(props.empty()) << "no page style for sheet Scale80";
	EXPECT_EQ(scale_to(props, L"X"), L"") << props;
	EXPECT_EQ(scale_to(props, L"Y"), L"") << props;
	EXPECT_EQ(attribute(props, L"style:scale-to"), L"80%") << props;
}
