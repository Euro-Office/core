// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: footers of several sections. Each section keeps the inherited
// footer of a type it does not define, and every footer is converted once.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	// Text of the FOOTER-x marker in each style:master-page, in document order.
	std::vector<std::wstring> master_page_footers(const std::wstring& styles)
	{
		std::vector<std::wstring> footers;
		size_t pos = 0;
		while ((pos = styles.find(L"<style:master-page ", pos)) != std::wstring::npos)
		{
			const size_t tag_end = styles.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				break;
			if (styles[tag_end - 1] == L'/')
			{
				footers.push_back(L""); // <style:master-page .../> without a footer
				pos = tag_end;
				continue;
			}
			const size_t end = styles.find(L"</style:master-page>", tag_end);
			if (end == std::wstring::npos)
				break;
			const size_t marker = styles.find(L"FOOTER-", tag_end);
			footers.push_back(marker < end ? styles.substr(marker, 8) : L"");
			pos = end;
		}
		return footers;
	}

	// Value of the attribute name in the start tag tag, or "" if the tag has none.
	std::wstring attribute_value(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t begin = pos + key.size();
		const size_t end = tag.find(L'"', begin);
		return end == std::wstring::npos ? L"" : tag.substr(begin, end - begin);
	}

	// Names of the automatic text styles in styles.xml, whatever the order of
	// the attributes (style:parent-style-name is written before style:family).
	std::vector<std::wstring> automatic_text_styles(const std::wstring& styles)
	{
		std::vector<std::wstring> names;
		const size_t begin = styles.find(L"<office:automatic-styles>");
		const size_t end = styles.find(L"</office:automatic-styles>");
		if (begin == std::wstring::npos || end == std::wstring::npos)
			return names;

		size_t pos = begin;
		while ((pos = styles.find(L"<style:style ", pos)) != std::wstring::npos && pos < end)
		{
			const size_t tag_end = styles.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				break;
			const std::wstring tag = styles.substr(pos, tag_end - pos);
			if (attribute_value(tag, L"style:family") == L"text")
				names.push_back(attribute_value(tag, L"style:name"));
			pos = tag_end;
		}
		return names;
	}

	// The styles in names that nothing in styles.xml uses.
	std::vector<std::wstring> unused_styles(const std::wstring& styles, const std::vector<std::wstring>& names)
	{
		std::vector<std::wstring> unused;
		for (const std::wstring& name : names)
		{
			if (styles.find(L"text:style-name=\"" + name + L"\"") == std::wstring::npos)
				unused.push_back(name);
		}
		return unused;
	}
}

// Sections 1-3 reference their own default footer (one bold run each), the last
// section references none and inherits the footer of section 3.
TEST(DocxToOdt, SectionFootersAreConvertedOnce)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/section_footers.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(styles));

	const std::vector<std::wstring> footers = master_page_footers(styles);
	const std::vector<std::wstring> expected = {L"", L"FOOTER-A", L"FOOTER-B", L"FOOTER-C", L"FOOTER-C"};
	EXPECT_EQ(footers, expected);

	const std::vector<std::wstring> text_styles = automatic_text_styles(styles);
	EXPECT_GT(text_styles.size(), 0u) << "no automatic text styles found in styles.xml";
	const std::vector<std::wstring> unused = unused_styles(styles, text_styles);
	EXPECT_TRUE(unused.empty()) << unused.size() << " automatic text styles belong to discarded footers";
}

// Section 1 lists two default footers (A, then X; the last one wins), section 2
// its own footer B, and the last section none, so it inherits B. Section 2's
// footer must replace X, not A.
TEST(DocxToOdt, SectionFooterReplacesTheLastOfItsType)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/section_footers_duplicate.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring styles = read_part(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty()) << "styles.xml is missing";

	const std::vector<std::wstring> footers = master_page_footers(styles);
	const std::vector<std::wstring> expected = {L"", L"FOOTER-X", L"FOOTER-B", L"FOOTER-B"};
	EXPECT_EQ(footers, expected);
}
