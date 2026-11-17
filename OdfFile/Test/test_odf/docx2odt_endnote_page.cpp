// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: Word prints endnotes at the end of the document, after the
// content of the last section. A last section that starts on a new page and
// holds only one empty paragraph is the page setup of the endnotes: its
// paragraph is not written, and its page style becomes the endnote page style
// (text:notes-configuration). Every other last section is kept as it is.

#include "gtest/gtest.h"

#include "common.h"

#include <cstdlib>
#include <string>

namespace
{
	std::wstring attribute(const std::wstring& xml, size_t element, const std::wstring& name)
	{
		const size_t end = xml.find(L'>', element);
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = xml.find(key, element);
		if (end == std::wstring::npos || pos == std::wstring::npos || pos > end)
			return L"";
		const size_t value = pos + key.size();
		const size_t quote = xml.find(L'"', value);
		return quote == std::wstring::npos ? L"" : xml.substr(value, quote - value);
	}

	// The opening tag of the endnote configuration, or npos.
	size_t endnote_configuration(const std::wstring& styles)
	{
		for (size_t pos = styles.find(L"<text:notes-configuration"); pos != std::wstring::npos;
			 pos = styles.find(L"<text:notes-configuration", pos + 1))
		{
			if (attribute(styles, pos, L"text:note-class") == L"endnote")
				return pos;
		}
		return std::wstring::npos;
	}

	std::wstring master_page(const std::wstring& styles, const std::wstring& name)
	{
		const size_t start = styles.find(L"<style:master-page style:name=\"" + name + L"\"");
		if (start == std::wstring::npos)
			return L"";
		const size_t end = styles.find(L"</style:master-page>", start);
		return styles.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
	}

	// True if the page layout of the master page is wider than high.
	bool is_landscape(const std::wstring& styles, const std::wstring& master)
	{
		const std::wstring page = master_page(styles, master);
		if (page.empty())
			return false;
		const std::wstring layout = attribute(page, 0, L"style:page-layout-name");
		const size_t start = styles.find(L"<style:page-layout style:name=\"" + layout + L"\"");
		if (layout.empty() || start == std::wstring::npos)
			return false;
		const size_t props = styles.find(L"<style:page-layout-properties", start);
		if (props == std::wstring::npos)
			return false;
		const double width = std::wcstod(attribute(styles, props, L"fo:page-width").c_str(), nullptr);
		const double height = std::wcstod(attribute(styles, props, L"fo:page-height").c_str(), nullptr);
		return width > 0 && height > 0 && width > height;
	}

	// Paragraphs of the body, without those inside notes.
	int body_paragraphs(const std::wstring& content)
	{
		const size_t start = content.find(L"<office:text");
		const size_t end = content.find(L"</office:text>");
		if (start == std::wstring::npos || end == std::wstring::npos)
			return -1;
		std::wstring body = content.substr(start, end - start);
		for (size_t note = body.find(L"<text:note-body"); note != std::wstring::npos;
			 note = body.find(L"<text:note-body"))
		{
			const size_t close = body.find(L"</text:note-body>", note);
			if (close == std::wstring::npos)
				return -1;
			body.erase(note, close + 17 - note);
		}
		int count = 0;
		for (size_t pos = body.find(L"<text:p"); pos != std::wstring::npos; pos = body.find(L"<text:p", pos + 1))
		{
			const wchar_t next = pos + 7 < body.size() ? body[pos + 7] : L'\0';
			if (next == L' ' || next == L'>' || next == L'/')
				count++;
		}
		return count;
	}

	struct converted
	{
		std::wstring styles;
		std::wstring content;
	};

	converted convert(const std::wstring& name)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/" + name + L".docx");
		converted result;
		if (dir.empty())
			return result;
		result.styles = read_part(dir, L"styles.xml");
		result.content = read_part(dir, L"content.xml");
		return result;
	}

	void expect_endnote_page(const converted& odt, const std::wstring& num_format, const std::wstring& start_value,
							 const std::wstring& header = L"ENDNOTE-HEADER")
	{
		ASSERT_FALSE(odt.styles.empty()) << "DOCX -> ODT conversion failed";
		EXPECT_TRUE(is_well_formed(odt.styles));
		EXPECT_TRUE(is_well_formed(odt.content));

		const size_t config = endnote_configuration(odt.styles);
		ASSERT_NE(config, std::wstring::npos) << "no endnote configuration";
		const std::wstring master = attribute(odt.styles, config, L"text:master-page-name");
		EXPECT_FALSE(master.empty());
		EXPECT_TRUE(is_landscape(odt.styles, master)) << "the endnote page has not the last section's page setup";
		EXPECT_NE(master_page(odt.styles, master).find(header), std::wstring::npos) << "the header is not " << header;
		EXPECT_EQ(attribute(odt.styles, config, L"style:num-format"), num_format);
		EXPECT_EQ(attribute(odt.styles, config, L"text:start-value"), start_value);

		EXPECT_EQ(body_paragraphs(odt.content), 2) << "the empty paragraph of the endnote page is written";
		EXPECT_NE(odt.content.find(L"NOTE text"), std::wstring::npos);
	}

	void expect_section_kept(const converted& odt)
	{
		ASSERT_FALSE(odt.styles.empty()) << "DOCX -> ODT conversion failed";
		EXPECT_EQ(endnote_configuration(odt.styles), std::wstring::npos);
		EXPECT_EQ(body_paragraphs(odt.content), 3);
	}
}

TEST(DocxToOdtEndnotePage, LastSectionBecomesTheEndnotePage)
{
	expect_endnote_page(convert(L"endnote_page_section"), L"i", L"0");
}

TEST(DocxToOdtEndnotePage, SectionWithoutTypeStartsANewPage)
{
	expect_endnote_page(convert(L"endnote_page_section_no_type"), L"i", L"0");
}

TEST(DocxToOdtEndnotePage, EndnoteNumberingComesFromTheSettings)
{
	expect_endnote_page(convert(L"endnote_page_section_numbering"), L"1", L"2");
}

TEST(DocxToOdtEndnotePage, SectionKeptWithoutEndnotes)
{
	expect_section_kept(convert(L"endnote_page_section_no_endnotes"));
}

TEST(DocxToOdtEndnotePage, SectionKeptWhenItHasText)
{
	const converted odt = convert(L"endnote_page_section_with_text");
	expect_section_kept(odt);
	EXPECT_NE(odt.content.find(L"TAIL"), std::wstring::npos);
}

TEST(DocxToOdtEndnotePage, SectionKeptWhenContinuous)
{
	expect_section_kept(convert(L"endnote_page_section_continuous"));
}

TEST(DocxToOdtEndnotePage, SectionKeptWhenEndnotesEndEachSection)
{
	expect_section_kept(convert(L"endnote_page_section_section_end"));
}

// The last section has no header of its own: as in Word, it shows the header
// of the section before it.
TEST(DocxToOdtEndnotePage, EndnotePageInheritsTheHeaderOfEarlierSections)
{
	expect_endnote_page(convert(L"endnote_page_section_inherited_header"), L"i", L"0", L"HEADER-BODY");
}

TEST(DocxToOdtEndnotePage, SectionKeptWhenItsEndnotesEndTheSection)
{
	expect_section_kept(convert(L"endnote_page_section_section_end_in_section"));
}

TEST(DocxToOdtEndnotePage, SectionKeptWhenTheEndnoteIsMissing)
{
	expect_section_kept(convert(L"endnote_page_section_missing_endnote"));
}
