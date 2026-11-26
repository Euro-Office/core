// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of hyperlinks (text:a). A target frame
// (office:target-frame-name, e.g. "open in a new window") does not make a
// link internal: an external URL must stay an external relationship, and only
// "#..." references become w:anchor bookmarks.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
TEST(OdtToDocxHyperlink, TargetFrameKeepsExternalLink)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/hyperlink_target_frame.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";
    EXPECT_NE(document.find(L"<w:hyperlink r:id=\""), std::wstring::npos);
    EXPECT_NE(document.find(L"w:tgtFrame=\"_blank\""), std::wstring::npos);
    EXPECT_EQ(document.find(L"w:anchor=\"ttps:"), std::wstring::npos);

    const std::wstring rels = read_part(dir, L"word/_rels/document.xml.rels");
    const size_t       link = rels.find(L"Target=\"https://example.org/page\"");
    ASSERT_NE(link, std::wstring::npos) << "no relationship for the link";
    const size_t begin = rels.rfind(L"<Relationship ", link);
    const size_t end   = rels.find(L">", link);
    ASSERT_NE(begin, std::wstring::npos);
    const std::wstring relationship = rels.substr(begin, end - begin);
    EXPECT_NE(relationship.find(L"TargetMode=\"External\""), std::wstring::npos) << relationship;
    EXPECT_NE(relationship.find(L"/relationships/hyperlink\""), std::wstring::npos) << relationship;
}

TEST(OdtToDocxHyperlink, TargetFrameOnBookmarkLinkKeepsAnchor)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/hyperlink_target_frame.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    EXPECT_NE(document.find(L"<w:hyperlink w:anchor=\"Bookmark1\""), std::wstring::npos);
}
} // namespace
