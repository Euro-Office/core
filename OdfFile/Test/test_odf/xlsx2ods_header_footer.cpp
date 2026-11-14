// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: header and footer content must sit in <text:p> or <style:region-*>,
// also when the XLSX header string has no &L/&C/&R section code.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// styles.xml of the converted fixture, converted once.
	const std::wstring& styles()
	{
		static const std::wstring xml = []()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_header_footer_no_section.xlsx");
			return dir.empty() ? std::wstring() : read_part(dir, L"styles.xml");
		}();
		return xml;
	}

	// The <style:master-page> element that contains marker.
	std::wstring master_page_with(const std::wstring& marker)
	{
		const std::wstring& xml = styles();
		const size_t at = xml.find(marker);
		if (at == std::wstring::npos)
			return L"";
		const size_t begin = xml.rfind(L"<style:master-page ", at);
		const size_t end = xml.find(L"</style:master-page>", at);
		if (begin == std::wstring::npos || end == std::wstring::npos)
			return L"";
		return xml.substr(begin, end - begin);
	}

	size_t count_of(const std::wstring& text, const std::wstring& what)
	{
		size_t n = 0;
		for (size_t at = text.find(what); at != std::wstring::npos; at = text.find(what, at + what.size()))
			++n;
		return n;
	}
}

TEST(XlsxToOdsHeaderFooter, ConvertsToWellFormedStyles)
{
	ASSERT_FALSE(styles().empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(styles()));
	EXPECT_EQ(styles().find(L"<style:header><text:span"), std::wstring::npos);
	EXPECT_EQ(styles().find(L"<style:footer><text:span"), std::wstring::npos);
}

// "&A" / "Page &P": fields without a section code go into one plain paragraph each.
TEST(XlsxToOdsHeaderFooter, FieldsWithoutSectionInParagraph)
{
	const std::wstring page = master_page_with(L"<text:sheet-name");
	ASSERT_FALSE(page.empty());
	EXPECT_NE(page.find(L"<style:header><text:p><text:span"), std::wstring::npos);
	EXPECT_NE(page.find(L"<style:footer><text:p><text:span"), std::wstring::npos);
	EXPECT_NE(page.find(L"<text:page-number"), std::wstring::npos);
	EXPECT_EQ(page.find(L"<style:region-"), std::wstring::npos);
}

// A line break without a section code starts a second paragraph and keeps its text.
TEST(XlsxToOdsHeaderFooter, LineBreakWithoutSectionKeepsBothLines)
{
	const std::wstring page = master_page_with(L"First line");
	ASSERT_FALSE(page.empty());
	EXPECT_NE(page.find(L"Second line"), std::wstring::npos);
	EXPECT_EQ(count_of(page, L"<text:p>"), 2u);
	EXPECT_EQ(page.find(L"<style:region-"), std::wstring::npos);
}

// Text before the first section code is the centre section.
TEST(XlsxToOdsHeaderFooter, LeadingTextIsCentreRegion)
{
	const std::wstring page = master_page_with(L"Before");
	ASSERT_FALSE(page.empty());
	const size_t center = page.find(L"<style:region-center><text:p>");
	const size_t right = page.find(L"<style:region-right><text:p>");
	ASSERT_NE(center, std::wstring::npos);
	ASSERT_NE(right, std::wstring::npos);
	EXPECT_LT(center, page.find(L"Before"));
	EXPECT_LT(page.find(L"Before"), right);
	EXPECT_LT(right, page.find(L">Right<"));
}

// Control: a string with all three section codes keeps its three regions.
TEST(XlsxToOdsHeaderFooter, SectionsKeepRegions)
{
	const std::wstring page = master_page_with(L">Left<");
	ASSERT_FALSE(page.empty());
	EXPECT_NE(page.find(L"<style:region-left><text:p>"), std::wstring::npos);
	EXPECT_NE(page.find(L"<style:region-center><text:p>"), std::wstring::npos);
	EXPECT_NE(page.find(L"<style:region-right><text:p>"), std::wstring::npos);
}
