// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of the Default cell style's font: the first cell style format, which
// the Default cell style uses, names font 0, the document's default font. Excel reads a missing
// fontId as font 0, but other readers then leave the Default cell style without a font.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// One sheet, the Default cell style names Liberation Sans.
	std::wstring styles_xml()
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_font_size_column_width.ods");
		return dir.empty() ? std::wstring() : read_part(dir, L"xl/styles.xml");
	}

	// The text from the first `open` up to the next `close` after it (without `close`).
	std::wstring between(const std::wstring& xml, const std::wstring& open, const std::wstring& close)
	{
		const size_t begin = xml.find(open);
		if (begin == std::wstring::npos)
			return L"";
		const size_t end = xml.find(close, begin + open.size());
		return end == std::wstring::npos ? std::wstring() : xml.substr(begin, end - begin);
	}
}

TEST(OdsToXlsxDefaultCellStyleFont, FirstCellStyleFormatNamesFontZero)
{
	const std::wstring styles = styles_xml();
	ASSERT_FALSE(styles.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring xf = between(between(styles, L"<cellStyleXfs", L"</cellStyleXfs>"), L"<xf ", L">");
	ASSERT_FALSE(xf.empty()) << styles;
	EXPECT_NE(xf.find(L"fontId=\"0\""), std::wstring::npos) << xf;
}

// Control: font 0 is the Default cell style's font.
TEST(OdsToXlsxDefaultCellStyleFont, FontZeroIsTheDefaultStyleFont)
{
	const std::wstring styles = styles_xml();
	ASSERT_FALSE(styles.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring font = between(between(styles, L"<fonts", L"</fonts>"), L"<font>", L"</font>");
	ASSERT_FALSE(font.empty()) << styles;
	EXPECT_NE(font.find(L"<name val=\"Liberation Sans\"/>"), std::wstring::npos) << font;
}
