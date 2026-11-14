// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: header and footer strings whose last code is cut short by the end of the string.

#include "gtest/gtest.h"

#include "common.h"

#include "DesktopEditor/common/Directory.h"

#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace
{
	std::wstring convert_xlsx_styles(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"styles.xml");
	}
}

// "&LTrailing&" ends in a bare '&', "&CColour&K12" has a colour code shorter than six digits.
TEST(XlsxToOdsHeaderFooterCodes, CodeCutShortAtEnd)
{
	const std::wstring xml = convert_xlsx_styles(L"ExampleFiles/sheet_header_footer_cut_short.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L">Trailing<"), std::wstring::npos);
	EXPECT_NE(xml.find(L">Colour<"), std::wstring::npos);
	EXPECT_EQ(xml.find(L"\"#12\""), std::wstring::npos);
}

// A font name without its closing quote takes the rest of the string, as in Excel and LibreOffice.
TEST(XlsxToOdsHeaderFooterCodes, UnterminatedFontName)
{
#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
	// Convert once in a child that a timer stops, so a parser that does not end fails the test
	// instead of hanging the suite.
	ASSERT_EXIT(
		{
			alarm(30);
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/sheet_header_unterminated_font.xlsx");
			if (!dir.empty())
				NSDirectory::DeleteDirectory(dir);
			_exit(dir.empty() ? 1 : 0);
		},
		::testing::ExitedWithCode(0), "");
#else
	GTEST_SKIP() << "needs death tests";
#endif
	const std::wstring xml = convert_xlsx_styles(L"ExampleFiles/sheet_header_unterminated_font.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L">Footer kept<"), std::wstring::npos);
	EXPECT_EQ(xml.find(L">Left<"), std::wstring::npos);
}
