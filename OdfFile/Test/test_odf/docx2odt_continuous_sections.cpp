// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: a continuous section break keeps the page. In ODF a paragraph with a master page name starts a new
// page, so such a section must not get one, unless the orientation changes (Word and LibreOffice break there) or the
// page numbers restart or change their format. Other left and right margins become the indents of a text section.

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
#include <string>

namespace
{
	// Master page name of the automatic paragraph style of the paragraph that holds `text`, "" if none.
	std::wstring master_page_of(const std::wstring& content, const std::wstring& text)
	{
		const size_t at = content.find(L">" + text + L"<");
		if (at == std::wstring::npos)
			return L"?";
		const size_t para = content.rfind(L"<text:p ", at);
		const std::wstring key = L"text:style-name=\"";
		const size_t name = para == std::wstring::npos ? std::wstring::npos : content.find(key, para);
		if (name == std::wstring::npos || name > at)
			return L"";
		const size_t name_end = content.find(L'"', name + key.size());
		const std::wstring style = content.substr(name + key.size(), name_end - name - key.size());

		const size_t def = content.find(L"style:name=\"" + style + L"\"");
		if (def == std::wstring::npos)
			return L"";
		const size_t start = content.rfind(L"<style:style ", def);
		const size_t end = content.find(L'>', def);
		const std::wstring tag = content.substr(start, end - start);
		const std::wstring mp = L"style:master-page-name=\"";
		const size_t m = tag.find(mp);
		if (m == std::wstring::npos)
			return L"";
		return tag.substr(m + mp.size(), tag.find(L'"', m + mp.size()) - m - mp.size());
	}

	// Value of `attr` in the section style of the text:section that holds `text`, "" if there is none.
	std::wstring section_attribute(const std::wstring& content, const std::wstring& text, const std::wstring& attr)
	{
		const size_t at = content.find(L">" + text + L"<");
		const size_t open = at == std::wstring::npos ? std::wstring::npos : content.rfind(L"<text:section ", at);
		if (open == std::wstring::npos)
			return L"";
		const size_t close = content.rfind(L"</text:section>", at);
		if (close != std::wstring::npos && close > open)
			return L"";
		const std::wstring key = L"text:style-name=\"";
		const size_t name = content.find(key, open);
		if (name == std::wstring::npos || name > content.find(L'>', open))
			return L"";
		const size_t name_end = content.find(L'"', name + key.size());
		const std::wstring style = content.substr(name + key.size(), name_end - name - key.size());

		const size_t def = content.find(L"style:name=\"" + style + L"\"");
		if (def == std::wstring::npos)
			return L"";
		// up to the end of this style, which may be an empty element
		const size_t end = (std::min)(content.find(L"</style:style>", def), content.find(L"<style:style ", def));
		const std::wstring body = content.substr(def, end - def);
		const size_t a = body.find(attr + L"=\"");
		if (a == std::wstring::npos)
			return L"";
		const size_t value = a + attr.size() + 2;
		return body.substr(value, body.find(L'"', value) - value);
	}

	// A length in cm as a number, -1000 if it is not a length in cm.
	double cm(const std::wstring& value)
	{
		if (value.size() < 3 || value.compare(value.size() - 2, 2, L"cm") != 0)
			return -1000;
		return std::stod(value.substr(0, value.size() - 2));
	}
}

// Four continuous one-column sections without headers: portrait, portrait, landscape, landscape.
TEST(DocxToOdt, ContinuousSectionKeepsPage)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/continuous_sections.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));

	EXPECT_EQ(master_page_of(content, L"SECOND"), L"") << "continuous break, same orientation";
	EXPECT_NE(master_page_of(content, L"THIRD"), L"") << "orientation changes: new page";
	EXPECT_NE(master_page_of(content, L"THIRD"), L"?");
	EXPECT_EQ(master_page_of(content, L"FOURTH"), L"") << "continuous break, same orientation";
}

// Three continuous portrait sections; only the middle one has line numbering. Its settings must not get lost when the
// section keeps the page.
TEST(DocxToOdt, ContinuousSectionKeepsLineNumbering)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/continuous_sections_line_numbers.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";

	const std::wstring parts = styles + content;
	const size_t config = parts.find(L"<text:linenumbering-configuration");
	ASSERT_NE(config, std::wstring::npos) << "line numbering of the middle section is lost";
	const std::wstring tag = parts.substr(config, parts.find(L'>', config) - config);
	// x2t writes text:number-lines only when it is false (ODF default: true).
	EXPECT_EQ(tag.find(L"text:number-lines=\"false\""), std::wstring::npos);
	EXPECT_NE(tag.find(L"text:increment=\"1\""), std::wstring::npos) << "w:countBy of the middle section";
}

// Continuous sections with other left and right margins than the page (1134/1134 twips): SECOND 2268/1701, THIRD
// 1134/1134, FOURTH 1701/1134, then FIFTH on a new page. The page stays; the difference becomes the section's indents.
TEST(DocxToOdt, ContinuousSectionMarginsBecomeIndents)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/continuous_sections_margins.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));

	EXPECT_EQ(master_page_of(content, L"SECOND"), L"") << "continuous break: same page";
	EXPECT_EQ(master_page_of(content, L"THIRD"), L"") << "continuous break: same page";
	EXPECT_EQ(master_page_of(content, L"FOURTH"), L"") << "continuous break: same page";
	EXPECT_NE(master_page_of(content, L"FIFTH"), L"") << "next page break";
	EXPECT_NE(master_page_of(content, L"FIFTH"), L"?");

	EXPECT_NEAR(cm(section_attribute(content, L"SECOND", L"fo:margin-left")), 2.0, 0.01) << "1134 twips";
	EXPECT_NEAR(cm(section_attribute(content, L"SECOND", L"fo:margin-right")), 1.0, 0.01) << "567 twips";
	EXPECT_EQ(section_attribute(content, L"THIRD", L"fo:margin-left"), L"") << "the page's margins: no indent";
	EXPECT_EQ(section_attribute(content, L"THIRD", L"fo:margin-right"), L"");
	EXPECT_NEAR(cm(section_attribute(content, L"FOURTH", L"fo:margin-left")), 1.0, 0.01) << "567 twips";
	EXPECT_EQ(section_attribute(content, L"FOURTH", L"fo:margin-right"), L"");
	EXPECT_EQ(section_attribute(content, L"FIFTH", L"fo:margin-left"), L"") << "a new page ends the indents";
}

// SECOND restarts the page numbers at 5 and FOURTH changes their format; ODF can do either only with a new master page.
// THIRD changes nothing and keeps the page.
TEST(DocxToOdt, ContinuousSectionPageNumbersStartPage)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/continuous_sections_page_numbers.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";

	EXPECT_NE(master_page_of(content, L"SECOND"), L"") << "page number restart";
	EXPECT_NE(master_page_of(content, L"SECOND"), L"?");
	EXPECT_EQ(master_page_of(content, L"THIRD"), L"") << "continuous break, same page numbers";
	EXPECT_NE(master_page_of(content, L"FOURTH"), L"") << "page number format changes";
	EXPECT_NE(master_page_of(content, L"FOURTH"), L"?");
}

// A two-column section (FIRST), then a one-column section that starts with a table (CELL), then AFTER. The table's
// first paragraph opens the new text section while the table is still open; the table must keep its place after FIRST
// and not move to the document root in front of all sections. (It ends up in the previous section.)
TEST(DocxToOdt, SectionStartingWithTableKeepsOrder)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/section_starts_with_table.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));

	const size_t section = content.find(L"<text:section ");
	const size_t first = content.find(L">FIRST<");
	const size_t table = content.find(L"<table:table");
	const size_t cell = content.find(L">CELL<");
	const size_t after = content.find(L">AFTER<");
	ASSERT_NE(section, std::wstring::npos) << "the two-column section is missing";
	ASSERT_NE(first, std::wstring::npos);
	ASSERT_NE(table, std::wstring::npos);
	ASSERT_NE(cell, std::wstring::npos);
	ASSERT_NE(after, std::wstring::npos);

	EXPECT_LT(section, table) << "the table moved in front of every section";
	EXPECT_LT(first, table) << "the table moved in front of FIRST";
	EXPECT_LT(table, cell);
	EXPECT_LT(cell, after);
}
