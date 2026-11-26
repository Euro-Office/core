// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: the body section (the w:sectPr of w:body, written as the Standard
// page) uses the headers and footers of earlier sections for the types it does
// not define itself.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The text of the Standard master page in styles.xml, or an empty string.
	std::wstring standard_master_page(const std::wstring& styles)
	{
		const size_t start = styles.find(L"<style:master-page style:name=\"Standard\"");
		if (start == std::wstring::npos)
			return L"";
		const size_t end = styles.find(L"</style:master-page>", start);
		if (end == std::wstring::npos)
			return L"";
		return styles.substr(start, end - start);
	}

	// The master page name in the paragraph style of the paragraph that holds `text`: "" if it has none,
	// "?" if the text or its style is not found.
	std::wstring master_page_of(const std::wstring& content, const std::wstring& text)
	{
		const size_t at = content.find(L">" + text + L"<");
		const size_t para = at == std::wstring::npos ? std::wstring::npos : content.rfind(L"<text:p ", at);
		if (para == std::wstring::npos)
			return L"?";
		const std::wstring key = L"text:style-name=\"";
		const size_t name = content.find(key, para);
		if (name == std::wstring::npos || name > content.find(L'>', para))
			return L"";
		const size_t name_end = content.find(L'"', name + key.size());
		const std::wstring style = content.substr(name + key.size(), name_end - name - key.size());

		const size_t def = content.find(L"style:name=\"" + style + L"\"");
		if (def == std::wstring::npos)
			return L"?";
		const size_t start = content.rfind(L"<style:style ", def);
		const std::wstring tag = content.substr(start, content.find(L'>', def) - start);
		const std::wstring mp = L"style:master-page-name=\"";
		const size_t m = tag.find(mp);
		if (m == std::wstring::npos)
			return L"";
		return tag.substr(m + mp.size(), tag.find(L'"', m + mp.size()) - m - mp.size());
	}
}

// Three continuous sections; only the first has a header and a footer.
TEST(DocxToOdt, BodySectionInheritsHeaderAndFooter)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/body_section_inherited_header_footer.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::wstring standard = standard_master_page(styles);
	ASSERT_FALSE(standard.empty()) << "no Standard master page";
	EXPECT_NE(standard.find(L"Inherited header"), std::wstring::npos);
	EXPECT_NE(standard.find(L"Inherited footer"), std::wstring::npos);
}

// The body section has its own header but no footer: it keeps its header and
// takes the footer of the earlier section.
TEST(DocxToOdt, BodySectionKeepsOwnHeaderAndInheritsFooter)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/body_section_own_header.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::wstring standard = standard_master_page(styles);
	ASSERT_FALSE(standard.empty()) << "no Standard master page";
	EXPECT_NE(standard.find(L"Body header"), std::wstring::npos);
	EXPECT_EQ(standard.find(L"Earlier header"), std::wstring::npos);
	EXPECT_NE(standard.find(L"Inherited footer"), std::wstring::npos);
}

// Two earlier sections have a header of the same type: the body section takes
// the one of the closest section, as Word does.
TEST(DocxToOdt, BodySectionTakesClosestEarlierHeader)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/body_section_closest_header.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::wstring standard = standard_master_page(styles);
	ASSERT_FALSE(standard.empty()) << "no Standard master page";
	EXPECT_NE(standard.find(L"Closest header"), std::wstring::npos);
	EXPECT_EQ(standard.find(L"Older header"), std::wstring::npos);
}

// The earlier section lists two default headers: the body section takes the
// last one, as the converter does for a section's own references.
TEST(DocxToOdt, BodySectionTakesLastDuplicateHeader)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/body_section_duplicate_header.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::wstring standard = standard_master_page(styles);
	ASSERT_FALSE(standard.empty()) << "no Standard master page";
	EXPECT_NE(standard.find(L"Last listed header"), std::wstring::npos);
	EXPECT_EQ(standard.find(L"First listed header"), std::wstring::npos);
}

// Two continuous sections: the first has a header and no footer, the body section has neither. Its
// paragraph has properties, so it gets a paragraph style that could carry a master page, and with no
// footer in force a section that does not keep the page names one. The body section borrows the first
// section's header for the Standard page; it still keeps the page.
TEST(DocxToOdt, ContinuousBodySectionAfterHeaderSectionKeepsPage)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/body_section_continuous_after_header.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));

	EXPECT_EQ(master_page_of(content, L"SECOND"), L"") << "continuous body section without own references";
}
