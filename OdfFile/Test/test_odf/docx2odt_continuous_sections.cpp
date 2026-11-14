// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: a continuous section break keeps the page. In ODF a paragraph with a master page name starts a new
// page, so such a section must not get one, unless the orientation changes (Word and LibreOffice break there).

#include "gtest/gtest.h"

#include "common.h"

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
