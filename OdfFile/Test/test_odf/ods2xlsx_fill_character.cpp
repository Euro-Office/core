// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of number:fill-character: the character repeats to fill the cell, "*"
// and the character in the XLSX format code.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
std::wstring styles_xml()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/number_fill_character.ods");
    return dir.empty() ? std::wstring() : read_part(dir, L"xl/styles.xml");
}

bool has_format(const std::wstring& code)
{
    return styles_xml().find(L"formatCode=\"" + code + L"\"") != std::wstring::npos;
}
}

// "€", a space, a fill space, the number: LibreOffice's accounting layout.
TEST(OdsToXlsxFillCharacter, CurrencyWithFillSpace)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_format(L"[$\u20AC] * #,##0.00")) << styles_xml();
}

TEST(OdsToXlsxFillCharacter, NumberWithFillDot)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_format(L"&quot;Total&quot;*.0.00")) << styles_xml();
}

TEST(OdsToXlsxFillCharacter, NumberWithoutFillUnchanged)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_format(L"0.00")) << styles_xml();
}
