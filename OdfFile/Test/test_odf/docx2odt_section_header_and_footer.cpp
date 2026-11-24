// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: sections of a document in which a header and a footer are in
// force. Every section that starts a new page becomes a master page with its
// headers and footers, and the first paragraph of the section names that
// master page. Naming a master page starts a new page, so a continuous
// section without a title page gets its own only where a page starts anyway.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// Value of an attribute of the element that starts at 'from'.
	std::wstring attribute(const std::wstring& xml, size_t from, const std::wstring& name)
	{
		const size_t end = xml.find(L'>', from);
		const size_t at = xml.find(L" " + name + L"=\"", from);
		if (end == std::wstring::npos || at == std::wstring::npos || at > end)
			return L"";
		const size_t begin = at + name.size() + 3;
		return xml.substr(begin, xml.find(L'"', begin) - begin);
	}

	// The element 'tag' whose style:name is 'name', up to the end of its start
	// tag or, when 'closing' is given, up to that closing tag.
	std::wstring named_element(const std::wstring& xml, const std::wstring& tag, const std::wstring& name,
							   const std::wstring& closing = L"")
	{
		const std::wstring open = L"<" + tag + L" ";
		for (size_t at = xml.find(open); at != std::wstring::npos; at = xml.find(open, at + 1))
		{
			if (attribute(xml, at, L"style:name") != name)
				continue;
			const size_t tag_end = xml.find(L'>', at);
			if (closing.empty() || xml[tag_end - 1] == L'/')
				return xml.substr(at, tag_end - at + 1);
			return xml.substr(at, xml.find(closing, at) - at);
		}
		return L"";
	}

	// The master page named by the style of the paragraph that holds 'text',
	// "" if that paragraph names none, "?" if the paragraph or its style is not
	// found.
	std::wstring master_page_of(const std::wstring& content, const std::wstring& text)
	{
		const size_t at = content.find(text);
		if (at == std::wstring::npos)
			return L"?";
		size_t paragraph = content.rfind(L"<text:p", at);
		while (paragraph != std::wstring::npos && paragraph > 0
			   && content[paragraph + 7] != L' ' && content[paragraph + 7] != L'>')
			paragraph = content.rfind(L"<text:p", paragraph - 1); // text:page-number, ...
		if (paragraph == std::wstring::npos)
			return L"?";
		const std::wstring style = attribute(content, paragraph, L"text:style-name");
		if (style.empty())
			return L"";
		const std::wstring element = named_element(content, L"style:style", style);
		return element.empty() ? L"?" : attribute(element, 0, L"style:master-page-name");
	}

	struct Converted
	{
		std::wstring content;
		std::wstring styles;

		// The master page of the section whose first paragraph holds 'text'.
		std::wstring master_page(const std::wstring& text) const
		{
			const std::wstring name = master_page_of(content, text);
			if (name.empty() || name == L"?")
				return L"";
			return named_element(styles, L"style:master-page", name, L"</style:master-page>");
		}
		// A page layout property of that master page.
		std::wstring layout_property(const std::wstring& text, const std::wstring& name) const
		{
			const std::wstring master = master_page(text);
			if (master.empty())
				return L"";
			const std::wstring layout_name = attribute(master, 0, L"style:page-layout-name");
			const std::wstring layout =
				named_element(styles, L"style:page-layout", layout_name, L"</style:page-layout>");
			const size_t props = layout.find(L"<style:page-layout-properties");
			return props == std::wstring::npos ? L"" : attribute(layout, props, name);
		}
		// The left page margin of that master page.
		std::wstring margin_left(const std::wstring& text) const
		{
			return layout_property(text, L"fo:margin-left");
		}
	};

	Converted convert(const std::wstring& docx)
	{
		Converted result;
		const std::wstring dir = convert_ooxml_to_odf_dir(docx);
		if (dir.empty())
			return result;
		result.content = read_part(dir, L"content.xml");
		result.styles = read_part(dir, L"styles.xml");
		return result;
	}

	bool has(const std::wstring& xml, const std::wstring& text)
	{
		return xml.find(text) != std::wstring::npos;
	}
}

// Three sections that start on a new page, each with a header and a footer.
TEST(DocxToOdtSectionHeaderAndFooter, SectionsOnNewPagesKeepHeaderAndFooter)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(odt.content));
	EXPECT_TRUE(is_well_formed(odt.styles));

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_TRUE(has(first, L"FOOTER-A")) << first;

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-B")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-B")) << second;

	const std::wstring third = odt.master_page(L"BODY first paragraph");
	EXPECT_TRUE(has(third, L"HEADER-C")) << third;
	EXPECT_TRUE(has(third, L"FOOTER-C")) << third;

	// The paragraphs inside a section stay on its master page.
	EXPECT_EQ(master_page_of(odt.content, L"S0 last paragraph"), L"");
	EXPECT_EQ(master_page_of(odt.content, L"S1 last paragraph"), L"");
}

// Two continuous sections, each with a header and a footer of its own. The
// second one starts in the middle of a page: it stays on that page and on the
// master page of the first section.
TEST(DocxToOdtSectionHeaderAndFooter, ContinuousSectionWithOwnReferencesStaysOnThePage)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_continuous.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_TRUE(has(first, L"FOOTER-A")) << first;

	EXPECT_EQ(master_page_of(odt.content, L"BODY first paragraph"), L"");
}

// Three continuous sections, each with a header and a footer of its own. The
// second and the third one start with a paragraph that has a page break
// before it, so they can have their master page.
TEST(DocxToOdtSectionHeaderAndFooter, ContinuousSectionsAtPageBreaksKeepHeaderAndFooter)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_page_break.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_TRUE(has(first, L"FOOTER-A")) << first;

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-B")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-B")) << second;

	const std::wstring third = odt.master_page(L"BODY first paragraph");
	EXPECT_TRUE(has(third, L"HEADER-C")) << third;
	EXPECT_TRUE(has(third, L"FOOTER-C")) << third;
}

// Three continuous sections. The first paragraph of the second one has a
// page break before it and a tracked change of its paragraph properties whose
// old properties have none: the section starts a page and gets its master
// page. The third one is the reverse: only the old properties have the page
// break, so it stays on the page.
TEST(DocxToOdtSectionHeaderAndFooter, TrackedChangeOfTheFirstParagraphDoesNotDecideThePageStart)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_tracked_change.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-B")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-B")) << second;

	EXPECT_EQ(master_page_of(odt.content, L"BODY first paragraph"), L"");
}

// Continuous sections after a section with a header and a footer. The second
// one starts with a page break but has no references, the third one has
// references but its page break is switched off: neither names a master page.
TEST(DocxToOdtSectionHeaderAndFooter, ContinuousSectionWithoutReferencesOrPageBreakNamesNoMasterPage)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_no_page_start.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;

	EXPECT_EQ(master_page_of(odt.content, L"S1 first paragraph"), L"");
	EXPECT_EQ(master_page_of(odt.content, L"BODY first paragraph"), L"");
}

// A landscape section with a header and a footer, then two continuous
// portrait sections without references. The change of orientation starts a
// new page, so the second section gets its master page; the third one stays
// on it.
TEST(DocxToOdtSectionHeaderAndFooter, ContinuousSectionWithAnotherOrientationGetsItsMasterPage)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_orientation.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_EQ(odt.layout_property(L"S0 first paragraph", L"style:print-orientation"), L"landscape");

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-A")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-A")) << second;
	EXPECT_NE(odt.layout_property(L"S1 first paragraph", L"style:print-orientation"), L"landscape");
	EXPECT_FALSE(odt.layout_property(L"S1 first paragraph", L"fo:page-width").empty());
	EXPECT_EQ(odt.layout_property(L"S1 first paragraph", L"fo:page-width"),
			  odt.layout_property(L"S0 first paragraph", L"fo:page-height"));

	EXPECT_EQ(master_page_of(odt.content, L"BODY first paragraph"), L"");
}

// The first section has only a header, the second one a header and a footer.
TEST(DocxToOdtSectionHeaderAndFooter, SectionAfterASectionWithOnlyAHeader)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_after_header_only.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_FALSE(has(first, L"FOOTER-B")) << first;

	const std::wstring second = odt.master_page(L"BODY first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-B")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-B")) << second;
}

// The second and third section start on a new page without header or footer
// references: they show those of the first section. The second one has a
// wider left margin.
TEST(DocxToOdtSectionHeaderAndFooter, SectionOnANewPageInheritsAndKeepsItsMargins)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_inherited.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	EXPECT_EQ(odt.margin_left(L"S0 first paragraph"), L"2.54cm");

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-A")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-A")) << second;
	EXPECT_EQ(odt.margin_left(L"S1 first paragraph"), L"5.08cm");

	const std::wstring third = odt.master_page(L"BODY first paragraph");
	EXPECT_TRUE(has(third, L"HEADER-A")) << third;
	EXPECT_EQ(odt.margin_left(L"BODY first paragraph"), L"2.54cm");
}

// A header and a footer are in force. Continuous sections without header or
// footer references then stay on the page: their first paragraph names no
// master page, which would start a new page.
TEST(DocxToOdtSectionHeaderAndFooter, ContinuousSectionWithoutReferencesStaysOnThePage)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_same_page.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	EXPECT_EQ(master_page_of(odt.content, L"S1 first paragraph"), L"");
	EXPECT_EQ(master_page_of(odt.content, L"BODY first paragraph"), L"");
}

// Only the first section has a title page with a header of its own. The
// first page of the later sections shows their ordinary header.
TEST(DocxToOdtSectionHeaderAndFooter, TitlePageOfTheFirstSectionIsNotInherited)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_title_page.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_TRUE(has(first, L"HEADER-T")) << first;
	EXPECT_TRUE(has(first, L"<style:header-first")) << first;

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-B")) << second;
	EXPECT_TRUE(has(second, L"FOOTER-B")) << second;
	EXPECT_FALSE(has(second, L"<style:header-first")) << second;
	EXPECT_FALSE(has(second, L"<style:footer-first")) << second;
	EXPECT_FALSE(has(second, L"HEADER-T")) << second;

	const std::wstring third = odt.master_page(L"BODY first paragraph");
	EXPECT_TRUE(has(third, L"HEADER-C")) << third;
	EXPECT_FALSE(has(third, L"<style:header-first")) << third;
}

// Only the second section has a title page with a header of its own.
TEST(DocxToOdtSectionHeaderAndFooter, TitlePageOfALaterSectionIsKept)
{
	const Converted odt = convert(L"ExampleFiles/section_header_and_footer_title_page_later.docx");
	ASSERT_FALSE(odt.content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring first = odt.master_page(L"S0 first paragraph");
	EXPECT_TRUE(has(first, L"HEADER-A")) << first;
	EXPECT_FALSE(has(first, L"<style:header-first")) << first;

	const std::wstring second = odt.master_page(L"S1 first paragraph");
	EXPECT_TRUE(has(second, L"HEADER-B")) << second;
	EXPECT_TRUE(has(second, L"HEADER-U")) << second;
	EXPECT_TRUE(has(second, L"<style:header-first")) << second;

	const std::wstring third = odt.master_page(L"BODY first paragraph");
	EXPECT_TRUE(has(third, L"HEADER-C")) << third;
	EXPECT_FALSE(has(third, L"<style:header-first")) << third;
	EXPECT_FALSE(has(third, L"HEADER-U")) << third;
}
