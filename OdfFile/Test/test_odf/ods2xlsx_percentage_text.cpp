// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of literal text in a percentage style: text other than "%" and spaces must
// be quoted in the XLSX format code, or its letters are read as format tokens.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
std::wstring styles_xml()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/number_percentage_text.ods");
    return dir.empty() ? std::wstring() : read_part(dir, L"xl/styles.xml");
}

bool has_format(const std::wstring& code)
{
    return styles_xml().find(L"formatCode=\"" + code + L"\"") != std::wstring::npos;
}
}

// "Rendite " 0.00 "%": unquoted, the d and e of "Rendite" would be date tokens.
TEST(OdsToXlsxPercentageText, TextBeforeNumberIsQuoted)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_format(L"&quot;Rendite &quot;0.00%")) << styles_xml();
}

// 0.00 "% p.a.": the "%" stays a percent sign, the rest is literal text.
TEST(OdsToXlsxPercentageText, TextAfterPercentIsQuoted)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_format(L"0.00%&quot; p.a.&quot;")) << styles_xml();
}

// 0.00 " %": a space before the percent sign needs no quotes and stays as it was.
TEST(OdsToXlsxPercentageText, SpaceBeforePercentUnchanged)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_format(L"0.00 %")) << styles_xml();
}

// 0.00 "%": still the built-in percentage format, no custom code.
TEST(OdsToXlsxPercentageText, PlainPercentStaysBuiltIn)
{
    ASSERT_FALSE(styles_xml().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_NE(styles_xml().find(L"numFmtId=\"10\""), std::wstring::npos) << styles_xml();
    EXPECT_FALSE(has_format(L"0.00%")) << styles_xml();
}
