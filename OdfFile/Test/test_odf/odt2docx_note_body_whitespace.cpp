// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of footnotes and endnotes whose text:note-body is
// pretty-printed (whitespace between its paragraphs). That whitespace is not
// content; writing it as a run outside a paragraph breaks the notes part, and a
// broken part loses every note of the document.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The fixture has a footnote with two paragraphs ("Footnote first", "Footnote second")
// and an endnote with one ("Endnote text"), each body indented with newlines and spaces.
const std::wstring& package()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/note_body_whitespace.odt");
    return dir;
}

std::wstring part(const std::wstring& name)
{
    return package().empty() ? std::wstring() : read_part(package(), name);
}

// The text of the first note element (w:footnote or w:endnote) with w:id="1".
std::wstring note(const std::wstring& xml, const std::wstring& tag)
{
    const std::wstring open = L"<" + tag + L" w:id=\"1\">";
    const size_t begin = xml.find(open);
    const size_t end   = xml.find(L"</" + tag + L">", begin);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";
    return xml.substr(begin + open.size(), end - begin - open.size());
}

void expect_note(const std::wstring& partName, const std::wstring& tag, const std::wstring& refTag)
{
    const std::wstring xml = part(partName);
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed or " << partName << " missing";
    EXPECT_TRUE(is_well_formed(xml)) << xml;
    const std::wstring body = note(xml, tag);
    ASSERT_FALSE(body.empty()) << xml;
    // The note starts with a paragraph, and its reference mark sits in a run of that paragraph.
    EXPECT_EQ(body.rfind(L"<w:p>", 0), 0u) << body;
    EXPECT_NE(body.find(L"<w:r><" + refTag), std::wstring::npos) << body;
}

TEST(OdtToDocxNoteBodyWhitespace, FootnotesPartIsWellFormed)
{
    expect_note(L"word/footnotes.xml", L"w:footnote", L"w:footnoteRef");
}

TEST(OdtToDocxNoteBodyWhitespace, EndnotesPartIsWellFormed)
{
    expect_note(L"word/endnotes.xml", L"w:endnote", L"w:endnoteRef");
}

TEST(OdtToDocxNoteBodyWhitespace, KeepsNoteText)
{
    const std::wstring footnote = note(part(L"word/footnotes.xml"), L"w:footnote");
    EXPECT_NE(footnote.find(L"Footnote first"), std::wstring::npos) << footnote;
    EXPECT_NE(footnote.find(L"Footnote second"), std::wstring::npos) << footnote;
    const std::wstring endnote = note(part(L"word/endnotes.xml"), L"w:endnote");
    EXPECT_NE(endnote.find(L"Endnote text"), std::wstring::npos) << endnote;
}

TEST(OdtToDocxNoteBodyWhitespace, BodyTextIsKept)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"after the footnote."), std::wstring::npos);
    EXPECT_NE(xml.find(L"after the endnote."), std::wstring::npos);
}
} // namespace
