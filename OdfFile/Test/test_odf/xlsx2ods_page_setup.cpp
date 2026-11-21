// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS conversion of the paper size and orientation of pageSetup. XLSX gives the paper
// upright and turns it with orientation="landscape"; ODF gives fo:page-width x fo:page-height as
// the page lies, and style:print-orientation does not turn it.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, each with paperSize and orientation in its pageSetup:
	// 1 A3 landscape, 2 Letter landscape, 3 A4 landscape, 4 A4 portrait, 5 landscape without paperSize,
	// 6 A4 small (paperSize 10) landscape, 7 landscape with a paper size id the converter does not map (41)
	const std::wstring& converted_dir()
	{
		static const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_page_setup_landscape.xlsx");
		return dir;
	}

	// Value of an attribute in a start tag, or "(none)" if it is absent.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"(none)";
		const size_t begin = pos + key.size();
		const size_t end = tag.find(L"\"", begin);
		return end == std::wstring::npos ? L"(none)" : tag.substr(begin, end - begin);
	}

	// The start tag of the first element whose attribute has the given value, empty if none. The
	// element's end position is returned in end (npos if none).
	std::wstring start_tag(const std::wstring& xml, const std::wstring& element, const std::wstring& name,
		const std::wstring& value, size_t* end = nullptr)
	{
		const std::wstring open = L"<" + element + L" ";
		for (size_t pos = xml.find(open); pos != std::wstring::npos; pos = xml.find(open, pos + 1))
		{
			const size_t close = xml.find(L">", pos);
			if (close == std::wstring::npos)
				break;
			const std::wstring tag = xml.substr(pos, close - pos + 1);
			if (attribute(tag, name) == value)
			{
				if (end)
					*end = xml.find(L"</" + element + L">", pos);
				return tag;
			}
		}
		return std::wstring();
	}

	// The style:page-layout-properties start tag of the named sheet:
	// table -> table style -> master page -> page layout.
	std::wstring page_layout_properties(const std::wstring& sheet_name)
	{
		const std::wstring& dir = converted_dir();
		if (dir.empty())
			return std::wstring();
		const std::wstring content = read_part(dir, L"content.xml");
		const std::wstring styles = read_part(dir, L"styles.xml");

		const std::wstring table = start_tag(content, L"table:table", L"table:name", sheet_name);
		const std::wstring style = start_tag(content, L"style:style", L"style:name", attribute(table, L"table:style-name"));
		const std::wstring master_page = start_tag(styles, L"style:master-page", L"style:name",
			attribute(style, L"style:master-page-name"));
		const std::wstring layout_name = attribute(master_page, L"style:page-layout-name");

		size_t end = std::wstring::npos;
		const std::wstring layout = start_tag(styles, L"style:page-layout", L"style:name", layout_name, &end);
		if (layout.empty() || end == std::wstring::npos)
			return std::wstring();
		const size_t begin = styles.find(layout);
		const size_t props = styles.find(L"<style:page-layout-properties", begin);
		if (props == std::wstring::npos || props > end)
			return std::wstring();
		return styles.substr(props, styles.find(L">", props) - props + 1);
	}

	// A length such as "42cm" in millimetres, or -1 if it has no known unit.
	double millimetres(const std::wstring& value)
	{
		if (value == L"(none)")
			return -1;
		const double number = std::stod(value);
		if (value.find(L"cm") != std::wstring::npos)
			return number * 10;
		if (value.find(L"mm") != std::wstring::npos)
			return number;
		if (value.find(L"in") != std::wstring::npos)
			return number * 25.4;
		if (value.find(L"pt") != std::wstring::npos)
			return number * 25.4 / 72;
		return -1;
	}

	void expect_page(const std::wstring& sheet_name, double width_mm, double height_mm, const std::wstring& orientation)
	{
		const std::wstring props = page_layout_properties(sheet_name);
		ASSERT_FALSE(props.empty()) << "no page layout for sheet " << std::string(sheet_name.begin(), sheet_name.end());
		EXPECT_NEAR(millimetres(attribute(props, L"fo:page-width")), width_mm, 1.0) << std::string(props.begin(), props.end());
		EXPECT_NEAR(millimetres(attribute(props, L"fo:page-height")), height_mm, 1.0) << std::string(props.begin(), props.end());
		EXPECT_EQ(attribute(props, L"style:print-orientation"), orientation);
	}
}

TEST(XlsxToOdsPageSetup, A3LandscapeIsWide)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"A3Landscape", 420, 297, L"landscape");
}

TEST(XlsxToOdsPageSetup, LetterLandscapeIsWide)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"LetterLandscape", 279.4, 215.9, L"landscape");
}

TEST(XlsxToOdsPageSetup, A4LandscapeIsWide)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"A4Landscape", 297, 210, L"landscape");
}

TEST(XlsxToOdsPageSetup, A4PortraitIsUpright)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"A4Portrait", 210, 297, L"portrait");
}

// Without a paper size the paper is Letter, the default of paperSize: the landscape sheet gets
// Letter turned, not only the orientation flag.
TEST(XlsxToOdsPageSetup, LandscapeWithoutPaperSizeIsLetterTurned)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"NoSizeLandscape", 279.4, 215.9, L"landscape");
}

TEST(XlsxToOdsPageSetup, A4SmallLandscapeIsA4Turned)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"A4SmallLandscape", 297, 210, L"landscape");
}

TEST(XlsxToOdsPageSetup, UnmappedPaperSizeLandscapeIsLetterTurned)
{
	ASSERT_FALSE(converted_dir().empty()) << "XLSX -> ODS conversion failed";
	expect_page(L"UnmappedLandscape", 279.4, 215.9, L"landscape");
}
