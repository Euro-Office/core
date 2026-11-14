// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of a '&' in header and footer text. In the XLSX header and footer strings
// '&' starts a code ("&D" is the date, "&P" the page number), so a '&' of the text must be written
// as "&&", or "R&D" comes back as "R" followed by the date.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet whose page style has
	// a header with regions: left "R&D" (plain text), center a bold span "Q&"A" and a page number,
	//                        right a title field "Smith & Sons";
	// a footer without regions: "A & B &".
	std::wstring sheet_xml()
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_header_footer_ampersand.ods");
		if (dir.empty())
			return std::wstring();
		return read_part(dir, L"xl/worksheets/sheet1.xml");
	}

	void replace_all(std::wstring& text, const std::wstring& from, const std::wstring& to)
	{
		for (size_t pos = text.find(from); pos != std::wstring::npos; pos = text.find(from, pos + to.size()))
			text.replace(pos, from.size(), to);
	}

	// The text of <tag> as Excel reads it, with the XML entities decoded.
	std::wstring header_footer(const std::wstring& sheet, const std::wstring& tag)
	{
		const std::wstring open = L"<" + tag + L">";
		const size_t start = sheet.find(open);
		if (start == std::wstring::npos)
			return std::wstring();
		const size_t end = sheet.find(L"</" + tag + L">", start);
		if (end == std::wstring::npos)
			return std::wstring();
		std::wstring text = sheet.substr(start + open.size(), end - start - open.size());
		replace_all(text, L"&quot;", L"\"");
		replace_all(text, L"&apos;", L"'");
		replace_all(text, L"&lt;", L"<");
		replace_all(text, L"&gt;", L">");
		replace_all(text, L"&amp;", L"&");
		return text;
	}
}

TEST(OdsToXlsxHeaderFooterAmpersand, PlainText)
{
	const std::wstring sheet = sheet_xml();
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring header = header_footer(sheet, L"oddHeader");
	EXPECT_NE(header.find(L"&LR&&D"), std::wstring::npos) << header;
}

// The span's text is escaped; the font codes of its style and the page number code are not.
TEST(OdsToXlsxHeaderFooterAmpersand, SpanTextAndCodes)
{
	const std::wstring sheet = sheet_xml();
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring header = header_footer(sheet, L"oddHeader");
	EXPECT_NE(header.find(L"Q&&\"A"), std::wstring::npos) << header;
	EXPECT_NE(header.find(L"&P"), std::wstring::npos) << header;
	EXPECT_EQ(header.find(L"&&P"), std::wstring::npos) << header;
}

TEST(OdsToXlsxHeaderFooterAmpersand, TitleField)
{
	const std::wstring sheet = sheet_xml();
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring header = header_footer(sheet, L"oddHeader");
	EXPECT_NE(header.find(L"&RSmith && Sons"), std::wstring::npos) << header;
}

TEST(OdsToXlsxHeaderFooterAmpersand, FooterWithoutRegions)
{
	const std::wstring sheet = sheet_xml();
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring footer = header_footer(sheet, L"oddFooter");
	EXPECT_NE(footer.find(L"A && B &&"), std::wstring::npos) << footer;
}
