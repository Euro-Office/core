// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of a space that sits between two spans of a heading
// (text:h). It is a text node of its own that holds only white space; it must
// reach word/document.xml like it does for an ordinary paragraph (text:p).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The text of the w:p element of word/document.xml that contains the given
// text: all w:t contents of that paragraph, in order.
std::wstring paragraph_text(const std::wstring& document, const std::wstring& text)
{
    const size_t at = document.find(text);
    if (at == std::wstring::npos)
        return L"";
    // "<w:p>" and "<w:p " but not "<w:pPr>".
    size_t begin = document.rfind(L"<w:p>", at);
    const size_t begin_attrs = document.rfind(L"<w:p ", at);
    if (begin == std::wstring::npos || (begin_attrs != std::wstring::npos && begin_attrs > begin))
        begin = begin_attrs;
    const size_t end = document.find(L"</w:p>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";

    std::wstring result;
    for (size_t t = document.find(L"<w:t", begin); t != std::wstring::npos && t < end;
         t = document.find(L"<w:t", t + 1))
    {
        const wchar_t next = document[t + 4];
        if (next != L'>' && next != L' ')
            continue; // <w:tab/>, <w:tc>, ...
        const size_t open_end = document.find(L'>', t);
        const size_t close = document.find(L"</w:t>", open_end);
        if (open_end == std::wstring::npos || close == std::wstring::npos || close > end)
            break;
        result += document.substr(open_end + 1, close - open_end - 1);
    }
    return result;
}
} // namespace

TEST(OdtToDocxHeadingSpaces, SpaceBetweenSpansKept)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/heading_space_between_spans.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring document = read_part(dir, L"word/document.xml");

    EXPECT_EQ(paragraph_text(document, L"Alpha"), L"Alpha Beta");
    EXPECT_EQ(paragraph_text(document, L"Gamma"), L"Gamma Delta Epsilon");
}

TEST(OdtToDocxHeadingSpaces, ParagraphUnchanged)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/heading_space_between_spans.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring document = read_part(dir, L"word/document.xml");

    EXPECT_EQ(paragraph_text(document, L"Zeta"), L"Zeta Eta");
}
