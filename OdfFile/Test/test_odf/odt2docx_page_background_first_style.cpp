// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX page background when the first page style has no page colour, but another page
// style has one.

#include "gtest/gtest.h"

#include "common.h"
#include "DesktopEditor/common/File.h"

#include <string>

namespace
{
    // Value of attribute name="..." in the first element that starts with tagStart, or empty.
    std::wstring attribute_of(const std::wstring& xml, const std::wstring& tagStart, const std::wstring& name)
    {
        const size_t tag = xml.find(tagStart);
        if (tag == std::wstring::npos)
            return L"";
        const size_t tagEnd = xml.find(L'>', tag);
        const std::wstring key = L" " + name + L"=\"";
        const size_t pos = xml.find(key, tag);
        if (pos == std::wstring::npos || pos > tagEnd)
            return L"";
        const size_t begin = pos + key.size();
        const size_t end = xml.find(L'"', begin);
        return end == std::wstring::npos ? L"" : xml.substr(begin, end - begin);
    }
}

// The first page style has a picture fill (draw:fill="bitmap") and no fo:background-color, the
// second page style a grey colour. The first page style's picture is the page background; the grey
// of the other page style must not replace it.
TEST(OdtToDocxPageBackgroundFirstStyle, OwnPictureFillIsKept)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_first_style_picture.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";

    const size_t background = document.find(L"<w:background");
    ASSERT_NE(background, std::wstring::npos) << document.substr(0, 2000);
    EXPECT_EQ(attribute_of(document, L"<w:background", L"w:color"), L"ffffff");

    const std::wstring rId = attribute_of(document.substr(background), L"<v:fill", L"r:id");
    ASSERT_FALSE(rId.empty()) << document.substr(background, 1000);

    const std::wstring rels = read_part(dir, L"word/_rels/document.xml.rels");
    const std::wstring relTag = L"Id=\"" + rId + L"\"";
    const size_t rel = rels.find(relTag);
    ASSERT_NE(rel, std::wstring::npos) << rels;
    const size_t relStart = rels.rfind(L'<', rel);
    const std::wstring target = attribute_of(rels.substr(relStart), L"<Relationship", L"Target");
    ASSERT_FALSE(target.empty()) << rels;
    EXPECT_TRUE(NSFile::CFileBinary::Exists(dir + FILE_SEPARATOR_STR + L"word" + FILE_SEPARATOR_STR + target))
        << "missing word/" << std::string(target.begin(), target.end());
}

// A first page style with no background of its own still takes the colour of the other page style.
TEST(OdtToDocxPageBackgroundFirstStyle, NoBackgroundTakesOtherStyleColour)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_first_style_none.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";
    ASSERT_NE(document.find(L"<w:background"), std::wstring::npos) << document.substr(0, 2000);
    EXPECT_EQ(attribute_of(document, L"<w:background", L"w:color"), L"C0C0C0");
}
