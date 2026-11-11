// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX and ODS -> XLSX conversion of fo:font-family: a CSS font family list, whose
// names with spaces are quoted, becomes one font name without quotes.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The fixtures use these fo:font-family values, one per paragraph "F<n>" (ODT) or per row (ODS).
const wchar_t* const expected[] = {
    L"Liberation Sans Narrow", // 'Liberation Sans Narrow'
    L"DejaVu Serif",           // "DejaVu Serif", serif
    L"Arial",                  // Arial, sans-serif
    L"Gentium Basic",          // Gentium Basic
    L"Noto Sans",              //   'Noto Sans'
    L"Source Sans Pro",        // 'Source Sans Pro (unterminated quote)
};

std::wstring docx_part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/font_family.odt");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

std::wstring xlsx_part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/font_family.ods");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The w:ascii font of the run that holds the given text.
std::wstring run_font(const std::wstring& document, const std::wstring& text)
{
    const size_t at = document.find(L">" + text + L"<");
    if (at == std::wstring::npos)
        return L"(no run)";
    const size_t run = document.rfind(L"<w:r>", at);
    const size_t fonts = document.find(L"<w:rFonts", run);
    if (run == std::wstring::npos || fonts == std::wstring::npos || fonts > at)
        return L"(no w:rFonts)";
    const size_t begin = document.find(L"w:ascii=\"", fonts);
    if (begin == std::wstring::npos || begin > at)
        return L"(no w:ascii)";
    const size_t value = begin + 9;
    return document.substr(value, document.find(L'"', value) - value);
}
}

TEST(OdfFontFamily, DocxRunFontsHaveNoQuotes)
{
    const std::wstring document = docx_part(L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
        EXPECT_EQ(run_font(document, L"F" + std::to_wstring(i)), expected[i]) << "paragraph F" << i;
}

TEST(OdfFontFamily, XlsxFontNamesHaveNoQuotes)
{
    const std::wstring styles = xlsx_part(L"xl/styles.xml");
    ASSERT_FALSE(styles.empty()) << "ODS -> XLSX conversion failed";
    for (const wchar_t* name : expected)
        EXPECT_NE(styles.find(L"<name val=\"" + std::wstring(name) + L"\"/>"), std::wstring::npos)
            << "no font named " << std::wstring(name);
    EXPECT_EQ(styles.find(L"&apos;"), std::wstring::npos) << "a font name kept its quotes";
    EXPECT_EQ(styles.find(L"serif"), std::wstring::npos) << "a font name kept its fallback list";
}
