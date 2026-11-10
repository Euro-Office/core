// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of text:chapter: a STYLEREF field to the heading of
// the outline level, with the cached chapter text as its result; and back.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The fixture has one paragraph per variant, reading "F<n>: " and the field;
// the page header holds one more field.
std::wstring part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/chapter_fields.odt");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The w:p element that holds the given text.
std::wstring paragraph_with(const std::wstring& xml, const std::wstring& text)
{
    const size_t at = xml.find(text);
    if (at == std::wstring::npos)
        return L"";
    const size_t begin = xml.rfind(L"<w:p>", at);
    const size_t end   = xml.find(L"</w:p>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";
    return xml.substr(begin, end + 6 - begin);
}

std::wstring instr_text(const std::wstring& paragraph)
{
    const std::wstring open = L"<w:instrText>";
    const size_t begin = paragraph.find(open);
    if (begin == std::wstring::npos)
        return L"(none)";
    const size_t end = paragraph.find(L"</w:instrText>", begin);
    return paragraph.substr(begin + open.size(), end - begin - open.size());
}

// The text between the field's separate and end characters.
std::wstring field_result(const std::wstring& paragraph)
{
    const size_t separate = paragraph.find(L"w:fldCharType=\"separate\"");
    const size_t end      = paragraph.find(L"w:fldCharType=\"end\"");
    if (separate == std::wstring::npos || end == std::wstring::npos)
        return L"(none)";
    std::wstring text;
    for (size_t at = paragraph.find(L"<w:t", separate); at < end; at = paragraph.find(L"<w:t", at + 1))
    {
        const size_t begin = paragraph.find(L">", at) + 1;
        if (paragraph.compare(at, 5, L"<w:t>") != 0 && paragraph.compare(at, 5, L"<w:t ") != 0)
            continue;
        text += paragraph.substr(begin, paragraph.find(L"</w:t>", begin) - begin);
    }
    return text;
}

void expect_field(int n, const std::wstring& instr, const std::wstring& result)
{
    const std::wstring document = part(L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring paragraph = paragraph_with(document, L">F" + std::to_wstring(n) + L": <");
    ASSERT_FALSE(paragraph.empty()) << "paragraph F" << n << " not found";
    EXPECT_EQ(instr_text(paragraph), instr) << paragraph;
    EXPECT_EQ(field_result(paragraph), result) << paragraph;
}
}

// text:display="name": the text of the heading of that level.
TEST(OdtToDocxChapterField, Name)
{
    expect_field(0, L" STYLEREF \"Heading 1\" ", L"Alpha");
    expect_field(1, L" STYLEREF \"Heading 2\" ", L"Beta");
}

// text:display="number" and "plain-number": the heading's number.
TEST(OdtToDocxChapterField, Number)
{
    expect_field(2, L" STYLEREF \"Heading 1\" \\n ", L"1");
    expect_field(3, L" STYLEREF \"Heading 2\" \\n ", L"1.1");
}

// number-and-name has no single-field equivalent: the field refers to the name,
// and the cached result keeps both. A field without attributes is read the same.
TEST(OdtToDocxChapterField, NumberAndName)
{
    expect_field(4, L" STYLEREF \"Heading 1\" ", L"1 Alpha");
    expect_field(5, L" STYLEREF \"Heading 1\" ", L"1 Alpha");
}

// A chapter field in a page header, where it matters most.
TEST(OdtToDocxChapterField, InHeader)
{
    const std::wstring document = part(L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring header = part(L"word/header1.xml");
    ASSERT_FALSE(header.empty());
    EXPECT_TRUE(is_well_formed(header));
    EXPECT_EQ(instr_text(paragraph_with(header, L"HEADER")), L" STYLEREF \"Heading 1\" ") << header;
    EXPECT_EQ(header.find(L"BIBLIOGRAPHY"), std::wstring::npos) << header;
    EXPECT_EQ(document.find(L"BIBLIOGRAPHY"), std::wstring::npos);
}

// DOCX -> ODT: a STYLEREF field (as written above, or by Word) keeps its result
// as text. It is not a reference to a bookmark (the instruction contains "REF"),
// and a style name containing HYPERLINK, the first field name searched for,
// does not make it a link.
TEST(DocxToOdtStyleRefField, ResultKeptAsText)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/styleref_fields.docx");
    ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";
    const std::wstring content = read_part(dir, L"content.xml");
    ASSERT_TRUE(is_well_formed(content));

    for (const wchar_t* result : {L">Alpha<", L">1.1<", L">Gamma<", L">Delta<"})
        EXPECT_NE(content.find(result), std::wstring::npos) << result;
    for (const wchar_t* element : {L"<text:bookmark-ref", L"<text:a "})
        EXPECT_EQ(content.find(element), std::wstring::npos) << element << "\n" << content;
}
