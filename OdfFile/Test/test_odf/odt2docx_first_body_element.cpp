// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX inputs whose body starts with an element that has no reader class.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// text:numbered-paragraph and text:page-sequence are allowed children of
// office:text but have no reader class, so reading them adds nothing to the
// body. When one of them comes first, the document must still convert.
TEST(OdtToDocxFirstBodyElement, UnsupportedNumberedParagraphFirst)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/numbered_paragraph_first.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"Next paragraph"), std::wstring::npos);
}

TEST(OdtToDocxFirstBodyElement, UnsupportedPageSequenceFirst)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_sequence_first.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"Frame text"), std::wstring::npos);
}
