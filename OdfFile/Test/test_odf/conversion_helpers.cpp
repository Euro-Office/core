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
#include <vector>

#include "DesktopEditor/common/Directory.h"
#include "DesktopEditor/common/File.h"

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

// The helpers fail a test over any malformed XML part of the converted
// package, so malformed_xml_parts() must find exactly those.
TEST(MalformedXmlParts, FindsOnlyBrokenXmlAndRelsParts)
{
    const std::wstring dir = NSDirectory::CreateDirectoryWithUniqueName(L"ExampleFiles");
    ASSERT_FALSE(dir.empty());
    ASSERT_TRUE(NSDirectory::CreateDirectory(dir + FILE_SEPARATOR_STR + L"sub"));

    ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(dir + FILE_SEPARATOR_STR + L"a.xml", L"<a>x &amp; y</a>"));
    ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(dir + FILE_SEPARATOR_STR + L"sub" + FILE_SEPARATOR_STR + L"b.rels", L"<a>x & y</a>"));
    ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(dir + FILE_SEPARATOR_STR + L"c.bin", L"<not xml"));

    const std::vector<std::wstring> malformed = malformed_xml_parts(dir);
    NSDirectory::DeleteDirectory(dir);

    ASSERT_EQ(malformed.size(), 1u);
    EXPECT_NE(malformed[0].find(L"b.rels"), std::wstring::npos) << malformed[0];
}
