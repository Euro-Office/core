// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX: a tiled page picture is written as a tiled VML fill.

#include "gtest/gtest.h"

#include "common.h"

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

// The page style has a picture fill with style:repeat="repeat". In VML, type="tile" repeats the
// picture; type="pattern" means a two-colour pattern that is recoloured with the fill colours.
TEST(OdtToDocxPageBackgroundTile, TiledPictureIsWrittenAsTile)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_tiled_picture.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";

    const size_t background = document.find(L"<v:background");
    ASSERT_NE(background, std::wstring::npos) << document.substr(0, 2000);
    const std::wstring fill = document.substr(background);
    ASSERT_FALSE(attribute_of(fill, L"<v:fill", L"r:id").empty()) << fill.substr(0, 1000);
    EXPECT_EQ(attribute_of(fill, L"<v:fill", L"type"), L"tile") << fill.substr(0, 1000);
}
