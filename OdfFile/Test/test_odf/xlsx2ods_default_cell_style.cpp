// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: the font of the "Default" cell style.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The <style:style> element named "Default" of the table-cell family, or an
	// empty string.
	std::wstring default_cell_style(const std::wstring& styles)
	{
		size_t pos = 0;
		while ((pos = styles.find(L"<style:style ", pos)) != std::wstring::npos)
		{
			const size_t tag_end = styles.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				return L"";
			// A self-closing style ends at its own tag, not at the next </style:style>.
			const bool empty_element = styles[tag_end - 1] == L'/';
			const size_t end = empty_element ? tag_end + 1 : styles.find(L"</style:style>", pos);
			if (end == std::wstring::npos)
				return L"";
			const std::wstring tag = styles.substr(pos, tag_end - pos);
			if (tag.find(L"style:name=\"Default\"") != std::wstring::npos &&
				tag.find(L"style:family=\"table-cell\"") != std::wstring::npos)
				return styles.substr(pos, end - pos);
			pos = end;
		}
		return L"";
	}

	std::wstring converted_default_cell_style(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		const std::wstring styles = read_part(dir, L"styles.xml");
		if (!is_well_formed(styles))
			return L"";
		return default_cell_style(styles);
	}
}

// The first cell style format has no fontId: the Default cell style takes the
// workbook's first font (Calibri 11pt), as LibreOffice does.
TEST(XlsxToOds, DefaultCellStyleWithoutFontUsesFirstFont)
{
	const std::wstring style = converted_default_cell_style(L"ExampleFiles/default_cell_style_without_font.xlsx");
	ASSERT_FALSE(style.empty()) << "no well-formed styles.xml with a Default table-cell style";
	EXPECT_NE(style.find(L"fo:font-size=\"11pt\""), std::wstring::npos) << style;
	EXPECT_NE(style.find(L"fo:font-family=\"Calibri\""), std::wstring::npos);
}

// Control: a fontId on the first cell style format still wins over the first font.
TEST(XlsxToOds, DefaultCellStyleKeepsItsOwnFont)
{
	const std::wstring style = converted_default_cell_style(L"ExampleFiles/default_cell_style_with_font.xlsx");
	ASSERT_FALSE(style.empty()) << "no well-formed styles.xml with a Default table-cell style";
	EXPECT_NE(style.find(L"fo:font-size=\"14pt\""), std::wstring::npos) << style;
	EXPECT_NE(style.find(L"fo:font-family=\"Arial\""), std::wstring::npos);
	EXPECT_EQ(style.find(L"Calibri"), std::wstring::npos);
}
