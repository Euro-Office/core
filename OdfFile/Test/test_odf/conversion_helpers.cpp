// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Controls for the conversion helpers in common.h: one plain document per
// direction converts in-process and the expected part is readable and
// well-formed, and is_well_formed() rejects the malformed XML that converter
// bugs produce. If one of these fails, the tests built on the helpers cannot
// be trusted either.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

TEST(OdfToOoxml, PlainOdtParagraph)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/plain_paragraph.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"Plain ODT paragraph"), std::wstring::npos);

    EXPECT_TRUE(read_part(dir, L"word/no-such-part.xml").empty());
}

// The first OOXML -> ODF conversion in this suite: proves the writer runs
// headless (no fonts directory, no display).
TEST(OoxmlToOdf, PlainDocxParagraph)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/plain_paragraph.docx");
    ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

    const std::wstring xml = read_part(dir, L"content.xml");
    ASSERT_FALSE(xml.empty()) << "content.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"Plain DOCX paragraph"), std::wstring::npos);
}

TEST(IsWellFormed, RejectsUnescapedAmpersand)
{
    EXPECT_TRUE(is_well_formed(L"<a>x &amp; y</a>"));
    EXPECT_FALSE(is_well_formed(L"<a>x & y</a>"));
}

TEST(IsWellFormed, RejectsBrokenMarkupAfterValidContent)
{
    EXPECT_TRUE(is_well_formed(L"<?xml version=\"1.0\"?><a><b/><c>text</c></a>"));
    EXPECT_FALSE(is_well_formed(L"<a><b/><c>text</c>< val=\"\"/></a>"));
    EXPECT_FALSE(is_well_formed(L"<a><b>text</a>"));
    EXPECT_FALSE(is_well_formed(L"<a>"));
    EXPECT_FALSE(is_well_formed(L""));
}
