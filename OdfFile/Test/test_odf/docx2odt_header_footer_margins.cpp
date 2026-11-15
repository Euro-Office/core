// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: page margins with a header and a footer. Word measures w:top
// (w:bottom) from the page edge to the body and w:header (w:footer) to the
// header (footer); in ODF the page margin is the header (footer) distance and
// the header (footer) takes the rest of the Word margin as a minimum height.

#include "gtest/gtest.h"

#include "common.h"

#include <stdexcept>
#include <string>

namespace
{
	// The first element "name" whose start tag has the attribute text "attribute" (any if empty), up to its end
	// ("/>" or "</name>"), or an empty string.
	std::wstring element(const std::wstring& xml, const std::wstring& name, const std::wstring& attribute = L"")
	{
		for (size_t start = xml.find(L"<" + name); start != std::wstring::npos; start = xml.find(L"<" + name, start + 1))
		{
			const size_t gt = xml.find(L'>', start);
			if (gt == std::wstring::npos)
				return L"";
			const wchar_t next = xml[start + name.size() + 1];
			if (next != L' ' && next != L'>' && next != L'/')
				continue; // a longer element name
			if (!attribute.empty() && xml.substr(start, gt - start).find(L" " + attribute) == std::wstring::npos)
				continue;
			if (xml[gt - 1] == L'/')
				return xml.substr(start, gt + 1 - start);
			const size_t end = xml.find(L"</" + name + L">", gt);
			if (end == std::wstring::npos)
				return L"";
			return xml.substr(start, end + name.size() + 3 - start);
		}
		return L"";
	}

	// The start tag of the first element "name" in xml, or an empty string.
	std::wstring start_tag(const std::wstring& xml, const std::wstring& name)
	{
		const size_t start = xml.find(L"<" + name);
		if (start == std::wstring::npos)
			return L"";
		const size_t gt = xml.find(L'>', start);
		return gt == std::wstring::npos ? L"" : xml.substr(start, gt + 1 - start);
	}

	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t start = tag.find(key);
		if (start == std::wstring::npos)
			return L"";
		const size_t end = tag.find(L'"', start + key.size());
		return end == std::wstring::npos ? L"" : tag.substr(start + key.size(), end - start - key.size());
	}

	// A length attribute in cm; -1 if it is missing or has an unknown unit.
	double length_cm(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring value = attribute(tag, name);
		if (value.empty())
			return -1;
		size_t pos = 0;
		double number = 0;
		try
		{
			number = std::stod(value, &pos);
		}
		catch (const std::exception&)
		{
			return -1;
		}
		const std::wstring unit = value.substr(pos);
		if (unit == L"cm")
			return number;
		if (unit == L"mm")
			return number / 10;
		if (unit == L"in")
			return number * 2.54;
		if (unit == L"pt")
			return number * 2.54 / 72;
		return -1;
	}

	// The style:page-layout used by the master page "name", or an empty string.
	std::wstring page_layout(const std::wstring& styles, const std::wstring& name)
	{
		const std::wstring master = start_tag(element(styles, L"style:master-page", L"style:name=\"" + name + L"\""),
			L"style:master-page");
		const std::wstring layout = attribute(master, L"style:page-layout-name");
		if (layout.empty())
			return L"";
		return element(styles, L"style:page-layout", L"style:name=\"" + layout + L"\"");
	}

	// The style:header-footer-properties start tag inside style:header-style or style:footer-style.
	std::wstring header_footer_properties(const std::wstring& layout, const std::wstring& style)
	{
		return start_tag(element(layout, style), L"style:header-footer-properties");
	}

	struct Converted
	{
		std::wstring page;   // style:page-layout-properties start tag
		std::wstring header; // style:header-footer-properties of the header style
		std::wstring footer; // style:header-footer-properties of the footer style
	};

	Converted convert(const std::wstring& docx, const std::wstring& master_page = L"Standard")
	{
		Converted result;
		const std::wstring dir = convert_ooxml_to_odf_dir(docx);
		EXPECT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";
		const std::wstring styles = read_part(dir, L"styles.xml");
		EXPECT_FALSE(styles.empty()) << "styles.xml is missing";
		EXPECT_TRUE(is_well_formed(styles));
		const std::wstring layout = page_layout(styles, master_page);
		EXPECT_FALSE(layout.empty()) << "no page layout for the master page " << master_page;
		result.page = start_tag(layout, L"style:page-layout-properties");
		result.header = header_footer_properties(layout, L"style:header-style");
		result.footer = header_footer_properties(layout, L"style:footer-style");
		return result;
	}
}

// w:top 3.0 cm (more than 2.4 cm), w:header 1.0 cm, w:bottom 2.0 cm, w:footer 0.5 cm.
TEST(DocxToOdt, HeaderAndFooterTakeTheRestOfThePageMargins)
{
	const Converted odt = convert(L"ExampleFiles/header_footer_page_margins.docx");
	ASSERT_FALSE(odt.page.empty());
	ASSERT_FALSE(odt.header.empty()) << "no header style";
	ASSERT_FALSE(odt.footer.empty()) << "no footer style";

	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-top"), 567 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.header, L"fo:min-height"), (1701 - 567) * 2.54 / 1440, 0.002);
	EXPECT_EQ(attribute(odt.header, L"svg:height"), L"") << "the header must grow with its content";
	EXPECT_EQ(attribute(odt.header, L"style:dynamic-spacing"), L"true");

	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-bottom"), 284 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.footer, L"fo:min-height"), (1134 - 284) * 2.54 / 1440, 0.002);
	EXPECT_EQ(attribute(odt.footer, L"svg:height"), L"") << "the footer must grow with its content";
	EXPECT_EQ(attribute(odt.footer, L"style:dynamic-spacing"), L"true");
}

// w:top 2.0 cm (less than 2.4 cm), w:header 1.25 cm; no footer, so the bottom margin is w:bottom (2.54 cm), not
// w:footer (1.27 cm).
TEST(DocxToOdt, HeaderWithoutFooterKeepsTheBottomMargin)
{
	const Converted odt = convert(L"ExampleFiles/header_page_margins_no_footer.docx");
	ASSERT_FALSE(odt.page.empty());
	ASSERT_FALSE(odt.header.empty()) << "no header style";

	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-top"), 709 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.header, L"fo:min-height"), (1134 - 709) * 2.54 / 1440, 0.002);
	EXPECT_EQ(attribute(odt.header, L"svg:height"), L"");
	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-bottom"), 2.54, 0.002);
}

// No header and no footer: the page margins are w:top and w:bottom.
TEST(DocxToOdt, PageWithoutHeaderOrFooterKeepsItsMargins)
{
	const Converted odt = convert(L"ExampleFiles/page_margins_no_header_footer.docx");
	ASSERT_FALSE(odt.page.empty());

	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-top"), 1134 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-bottom"), 2.54, 0.002);
}

// w:top 1.06 cm is less than w:header 1.25 cm: the header keeps the minimum height of 0.1 cm. w:bottom is negative
// (-2.0 cm, Word keeps the body clear of the footer): the converter uses its absolute value.
TEST(DocxToOdt, SmallAndNegativeMarginsWithHeaderAndFooter)
{
	const Converted odt = convert(L"ExampleFiles/header_footer_small_and_negative_margins.docx");
	ASSERT_FALSE(odt.page.empty());
	ASSERT_FALSE(odt.header.empty()) << "no header style";
	ASSERT_FALSE(odt.footer.empty()) << "no footer style";

	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-top"), 709 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.header, L"fo:min-height"), 0.1, 0.002);
	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-bottom"), 284 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.footer, L"fo:min-height"), (1134 - 284) * 2.54 / 1440, 0.002);
}

// Section 1 has a title page, a header and a footer (w:top 3.0 cm, w:header 1.0 cm, w:bottom 2.0 cm, w:footer
// 0.5 cm). The body section is continuous with two columns and no title page, so it is converted into the page
// layout of section 1, which already has the header and the footer and does not get them again. The page margins
// and the header and footer heights must all come from the body section's w:pgMar (w:top 2.0 cm, w:header 1.25 cm,
// w:bottom 2.54 cm, w:footer 1.27 cm), not the margins from one section and the heights from the other.
TEST(DocxToOdt, ContinuousSectionSetsMarginsAndHeightsTogether)
{
	const Converted odt = convert(L"ExampleFiles/continuous_section_page_margins.docx", L"MasterPage2");
	ASSERT_FALSE(odt.page.empty());
	ASSERT_FALSE(odt.header.empty()) << "no header style";
	ASSERT_FALSE(odt.footer.empty()) << "no footer style";

	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-top"), 709 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.header, L"fo:min-height"), (1134 - 709) * 2.54 / 1440, 0.002);
	EXPECT_EQ(attribute(odt.header, L"svg:height"), L"");
	EXPECT_NEAR(length_cm(odt.page, L"fo:margin-bottom"), 720 * 2.54 / 1440, 0.002);
	EXPECT_NEAR(length_cm(odt.footer, L"fo:min-height"), (1440 - 720) * 2.54 / 1440, 0.002);
	EXPECT_EQ(attribute(odt.footer, L"svg:height"), L"");
}
