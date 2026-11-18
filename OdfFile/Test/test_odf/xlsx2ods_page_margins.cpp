// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: page margins of a sheet with and without a header and footer.
// XLSX gives the top margin (where the cells start) and the header distance
// from the edge of the page; ODF puts the header between the page margin and
// the cells. All fixtures use top="1" bottom="1.1" header="0.4" footer="0.3"
// (inches).

#include "gtest/gtest.h"

#include "common.h"

#include <cmath>
#include <cwchar>
#include <string>

namespace
{
	const double cm_per_inch = 2.54;

	// The first style:page-layout element, up to its end tag.
	std::wstring first_page_layout(const std::wstring& styles)
	{
		const size_t start = styles.find(L"<style:page-layout ");
		if (start == std::wstring::npos)
			return L"";
		const size_t end = styles.find(L"</style:page-layout>", start);
		if (end == std::wstring::npos)
			return L"";
		return styles.substr(start, end - start);
	}

	// The start tag of the first element `name` after `after` in `xml`.
	std::wstring start_tag(const std::wstring& xml, const std::wstring& name, const std::wstring& after = L"")
	{
		size_t from = 0;
		if (!after.empty())
		{
			from = xml.find(after);
			if (from == std::wstring::npos)
				return L"";
		}
		const size_t start = xml.find(L"<" + name, from);
		if (start == std::wstring::npos)
			return L"";
		const size_t end = xml.find(L'>', start);
		if (end == std::wstring::npos)
			return L"";
		return xml.substr(start, end - start + 1);
	}

	// The start tag of the first element `name` inside the first element `parent` of `xml` (empty when
	// `parent` has no such child, so that a sibling's child is never taken instead).
	std::wstring child_start_tag(const std::wstring& xml, const std::wstring& parent, const std::wstring& name)
	{
		const size_t start = xml.find(L"<" + parent + L">");
		if (start == std::wstring::npos)
			return L"";
		const size_t end = xml.find(L"</" + parent + L">", start);
		if (end == std::wstring::npos)
			return L"";
		return start_tag(xml.substr(start, end - start), name);
	}

	// Whether `xml` contains an element `name` (start tag or empty element).
	bool has_element(const std::wstring& xml, const std::wstring& name)
	{
		const std::wstring open = L"<" + name;
		for (size_t pos = xml.find(open); pos != std::wstring::npos; pos = xml.find(open, pos + 1))
		{
			const wchar_t next = pos + open.size() < xml.size() ? xml[pos + open.size()] : L'\0';
			if (next == L'>' || next == L' ' || next == L'/')
				return true;
		}
		return false;
	}

	bool has_attribute(const std::wstring& tag, const std::wstring& attribute)
	{
		return tag.find(L" " + attribute + L"=\"") != std::wstring::npos;
	}

	// A length attribute in centimetres, NAN when it is missing or not in cm.
	double length_cm(const std::wstring& tag, const std::wstring& attribute)
	{
		const std::wstring key = L" " + attribute + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return NAN;
		const size_t value = pos + key.size();
		const size_t quote = tag.find(L'"', value);
		if (quote == std::wstring::npos)
			return NAN;
		const std::wstring text = tag.substr(value, quote - value);
		wchar_t* unit = nullptr;
		const double number = std::wcstod(text.c_str(), &unit);
		if (unit == text.c_str() || std::wstring(unit) != L"cm")
			return NAN;
		return number;
	}
}

// With a header and a footer the ODF page margins are the header and footer
// distances, and the header and footer reach to where the cells start and end.
TEST(XlsxToOds, PageMarginsWithHeaderAndFooter)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_margins_header_footer.xlsx");
	ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::wstring layout = first_page_layout(styles);
	ASSERT_FALSE(layout.empty()) << "no style:page-layout";

	const std::wstring page = start_tag(layout, L"style:page-layout-properties");
	EXPECT_NEAR(length_cm(page, L"fo:margin-top"), 0.4 * cm_per_inch, 0.001) << page;
	EXPECT_NEAR(length_cm(page, L"fo:margin-bottom"), 0.3 * cm_per_inch, 0.001) << page;

	const std::wstring header = child_start_tag(layout, L"style:header-style", L"style:header-footer-properties");
	ASSERT_FALSE(header.empty()) << "no header properties";
	EXPECT_NEAR(length_cm(header, L"fo:min-height"), (1.0 - 0.4) * cm_per_inch, 0.001) << header;
	// a minimum height only, so that a taller header still grows
	EXPECT_FALSE(has_attribute(header, L"svg:height")) << header;

	const std::wstring footer = child_start_tag(layout, L"style:footer-style", L"style:header-footer-properties");
	ASSERT_FALSE(footer.empty()) << "no footer properties";
	EXPECT_NEAR(length_cm(footer, L"fo:min-height"), (1.1 - 0.3) * cm_per_inch, 0.001) << footer;
}

// Control: without a header or footer the XLSX margins stay the page margins.
TEST(XlsxToOds, PageMarginsWithoutHeaderAndFooter)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_margins_no_header_footer.xlsx");
	ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::wstring layout = first_page_layout(styles);
	ASSERT_FALSE(layout.empty()) << "no style:page-layout";

	const std::wstring page = start_tag(layout, L"style:page-layout-properties");
	EXPECT_NEAR(length_cm(page, L"fo:margin-top"), 1.0 * cm_per_inch, 0.001) << page;
	EXPECT_NEAR(length_cm(page, L"fo:margin-bottom"), 1.1 * cm_per_inch, 0.001) << page;
}

// A different first page with only a first-page header and footer: the other pages get an empty header
// and footer, so that their cells start at the XLSX top margin too, and the margins follow the header.
TEST(XlsxToOds, PageMarginsWithFirstPageHeaderOnly)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_margins_first_header.xlsx");
	ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	EXPECT_TRUE(has_element(styles, L"style:header-first"));
	EXPECT_TRUE(has_element(styles, L"style:header")) << "style:header-first needs a style:header";
	EXPECT_TRUE(has_element(styles, L"style:footer-first"));
	EXPECT_TRUE(has_element(styles, L"style:footer")) << "style:footer-first needs a style:footer";

	const std::wstring layout = first_page_layout(styles);
	ASSERT_FALSE(layout.empty()) << "no style:page-layout";

	const std::wstring page = start_tag(layout, L"style:page-layout-properties");
	EXPECT_NEAR(length_cm(page, L"fo:margin-top"), 0.4 * cm_per_inch, 0.001) << page;
	EXPECT_NEAR(length_cm(page, L"fo:margin-bottom"), 0.3 * cm_per_inch, 0.001) << page;

	const std::wstring header = child_start_tag(layout, L"style:header-style", L"style:header-footer-properties");
	ASSERT_FALSE(header.empty()) << "no header properties";
	EXPECT_NEAR(length_cm(header, L"fo:min-height"), (1.0 - 0.4) * cm_per_inch, 0.001) << header;

	const std::wstring footer = child_start_tag(layout, L"style:footer-style", L"style:header-footer-properties");
	ASSERT_FALSE(footer.empty()) << "no footer properties";
	EXPECT_NEAR(length_cm(footer, L"fo:min-height"), (1.1 - 0.3) * cm_per_inch, 0.001) << footer;
}
