// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of text:database-display, the mail merge field that
// shows the current value of a database column. The converter must keep that
// value as text in the run properties around the field; dropping the element
// loses the text (e.g. every "<Last Name>" placeholder of a form letter).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The fixture has four paragraphs "D<n>: <field> END<n>":
// D0 the field inside a bold span, D1 a value with an ampersand, D2 an empty field,
// D3 a hidden field (text:display="none").
std::wstring document()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/database_display_fields.odt");
    return dir.empty() ? std::wstring() : read_part(dir, L"word/document.xml");
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

// The w:r element that holds the given text.
std::wstring run_with(const std::wstring& xml, const std::wstring& text)
{
    const size_t at = xml.find(text);
    if (at == std::wstring::npos)
        return L"";
    const size_t begin = xml.rfind(L"<w:r>", at);
    const size_t end   = xml.find(L"</w:r>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";
    return xml.substr(begin, end + 6 - begin);
}

TEST(OdtToDocxDatabaseDisplay, DocumentIsWellFormed)
{
    const std::wstring xml = document();
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
}

TEST(OdtToDocxDatabaseDisplay, KeepsValueInSpanFormatting)
{
    const std::wstring paragraph = paragraph_with(document(), L"D0: ");
    ASSERT_FALSE(paragraph.empty()) << "paragraph D0 not found";
    const std::wstring run = run_with(paragraph, L"&lt;Last Name&gt;");
    ASSERT_FALSE(run.empty()) << paragraph;
    EXPECT_NE(run.find(L"<w:b/>"), std::wstring::npos) << run;
    EXPECT_NE(paragraph.find(L"END0"), std::wstring::npos) << paragraph;
}

TEST(OdtToDocxDatabaseDisplay, EscapesValue)
{
    const std::wstring paragraph = paragraph_with(document(), L"D1: ");
    ASSERT_FALSE(paragraph.empty()) << "paragraph D1 not found";
    EXPECT_NE(paragraph.find(L"A &amp; B"), std::wstring::npos) << paragraph;
    EXPECT_NE(paragraph.find(L"END1"), std::wstring::npos) << paragraph;
}

TEST(OdtToDocxDatabaseDisplay, EmptyFieldKeepsParagraph)
{
    const std::wstring paragraph = paragraph_with(document(), L"D2: ");
    ASSERT_FALSE(paragraph.empty()) << "paragraph D2 not found";
    EXPECT_NE(paragraph.find(L"END2"), std::wstring::npos) << paragraph;
}

TEST(OdtToDocxDatabaseDisplay, HiddenFieldIsNotShown)
{
    const std::wstring paragraph = paragraph_with(document(), L"D3: ");
    ASSERT_FALSE(paragraph.empty()) << "paragraph D3 not found";
    EXPECT_EQ(paragraph.find(L"Hidden Value"), std::wstring::npos) << paragraph;
    EXPECT_NE(paragraph.find(L"END3"), std::wstring::npos) << paragraph;
}
} // namespace
