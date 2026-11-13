// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX and ODS -> XLSX conversion of fo:font-family: a CSS font family list, whose
// names with spaces or commas are quoted, becomes one font name without quotes or escapes.

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
// The fixtures use these fo:font-family values, one per paragraph "F<n>" (ODT) or per row (ODS).
const wchar_t* const expected[] = {
    L"Liberation Sans Narrow",           // 'Liberation Sans Narrow'
    L"DejaVu Serif",                     // "DejaVu Serif", serif
    L"Arial",                            // Arial, sans-serif
    L"Gentium Basic",                    // Gentium Basic
    L"Noto Sans",                        //   'Noto Sans'
    L"Source Sans Pro",                  // 'Source Sans Pro (unterminated quote)
    L"Joe's Font",                       // 'Joe's Font' (as LibreOffice writes it)
    L"Joe's Font",                       // 'Joe's Font', serif
    L"Sugar, Spice and Everything Nice", // "Sugar, Spice and Everything Nice", serif
    L"Joe's Font",                       // 'Joe\'s Font'
    L"Sugar, Spice",                     // Sugar\, Spice, serif
    L"Café Sans",                   // 'Caf\E9  Sans'
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

// An XML attribute value as text.
std::wstring unescape_xml(const std::wstring& value)
{
    static const wchar_t* const entities[][2] = {
        {L"&apos;", L"'"}, {L"&quot;", L"\""}, {L"&lt;", L"<"}, {L"&gt;", L">"}, {L"&amp;", L"&"}};
    std::wstring result;
    for (size_t i = 0; i < value.size();)
    {
        bool replaced = false;
        for (const auto& entity : entities)
        {
            const std::wstring name = entity[0];
            if (value.compare(i, name.size(), name) == 0)
            {
                result += entity[1];
                i += name.size();
                replaced = true;
                break;
            }
        }
        if (!replaced)
        {
            result += value[i++];
        }
    }
    return result;
}

// The values of all attributes that follow the given prefix, for example L"<name val=\"".
std::vector<std::wstring> attribute_values(const std::wstring& xml, const std::wstring& prefix)
{
    std::vector<std::wstring> values;
    for (size_t at = xml.find(prefix); at != std::wstring::npos; at = xml.find(prefix, at))
    {
        at += prefix.size();
        values.push_back(unescape_xml(xml.substr(at, xml.find(L'"', at) - at)));
    }
    return values;
}

// The w:ascii font of the run that holds the given text.
std::wstring run_font(const std::wstring& document, const std::wstring& text)
{
    const size_t at = document.find(L">" + text + L"<");
    if (at == std::wstring::npos)
    {
        return L"(no run)";
    }
    const size_t run = document.rfind(L"<w:r>", at);
    const size_t fonts = document.find(L"<w:rFonts", run);
    if (run == std::wstring::npos || fonts == std::wstring::npos || fonts > at)
    {
        return L"(no w:rFonts)";
    }
    const size_t begin = document.find(L"w:ascii=\"", fonts);
    if (begin == std::wstring::npos || begin > at)
    {
        return L"(no w:ascii)";
    }
    const size_t value = begin + 9;
    return unescape_xml(document.substr(value, document.find(L'"', value) - value));
}
}

TEST(OdfFontFamily, DocxRunFontsHaveNoQuotes)
{
    const std::wstring document = docx_part(L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i)
    {
        EXPECT_EQ(run_font(document, L"F" + std::to_wstring(i)), expected[i]) << "paragraph F" << i;
    }
}

TEST(OdfFontFamily, XlsxFontNamesHaveNoQuotes)
{
    const std::wstring styles = xlsx_part(L"xl/styles.xml");
    ASSERT_FALSE(styles.empty()) << "ODS -> XLSX conversion failed";
    const std::vector<std::wstring> names = attribute_values(styles, L"<name val=\"");
    for (const wchar_t* name : expected)
    {
        EXPECT_NE(std::find(names.begin(), names.end(), name), names.end()) << "no font named " << std::wstring(name);
    }
    for (const std::wstring& name : names)
    {
        EXPECT_FALSE(!name.empty() && (name[0] == L'\'' || name[0] == L'"')) << "font name with quotes: " << name;
        EXPECT_EQ(name.find(L'\\'), std::wstring::npos) << "font name with an escape: " << name;
        EXPECT_EQ(name.find(L"serif"), std::wstring::npos) << "font name with its fallback list: " << name;
    }
}
