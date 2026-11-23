// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of bullet characters that are symbols (check mark, ballot X, circles, arrows, ballot box).
// The DOCX level keeps the font of the ODF list style, so the bullet must keep its Unicode code point: a Symbol or
// Wingdings private-use code would only be right together with a Symbol or Wingdings font.

#include "gtest/gtest.h"

#include "common.h"

#include <regex>
#include <string>

namespace
{
const std::wstring& numbering()
{
    static const std::wstring xml = []
    {
        const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/symbol_bullets.odt");
        return dir.empty() ? std::wstring() : read_part(dir, L"word/numbering.xml");
    }();
    return xml;
}

// w:lvlText of level `ilvl` of the first abstract numbering, or "<missing>".
std::wstring level_text(int ilvl)
{
    std::wsmatch m;
    const std::wregex lvl(L"<w:lvl w:ilvl=\"" + std::to_wstring(ilvl) + L"\"[^>]*>[\\s\\S]*?<w:lvlText w:val=\"([^\"]*)\"");
    if (!std::regex_search(numbering(), m, lvl))
        return L"<missing>";
    return m[1].str();
}
}

class OdtToDocxSymbolBullets : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_FALSE(numbering().empty()) << "ODT -> DOCX conversion failed or word/numbering.xml is missing";
    }
};

// Levels 1-6 and 9 use the font OpenSymbol, level 7 has no font of its own.
TEST_F(OdtToDocxSymbolBullets, SymbolBulletsKeepTheirCodePoint)
{
    EXPECT_EQ(level_text(0), L"\x2714"); // heavy check mark
    EXPECT_EQ(level_text(1), L"\x2717"); // ballot X
    EXPECT_EQ(level_text(2), L"\x25CF"); // black circle
    EXPECT_EQ(level_text(3), L"\x25CB"); // white circle
    EXPECT_EQ(level_text(4), L"\x27A2"); // arrowhead
    EXPECT_EQ(level_text(5), L"\x2611"); // ballot box with check
    EXPECT_EQ(level_text(6), L"\x2794"); // heavy wide-headed arrow, no font
    EXPECT_EQ(level_text(8), L"\xE00A"); // OpenSymbol private-use bullet
}

// The plain bullet is converted as before.
TEST_F(OdtToDocxSymbolBullets, PlainBulletUnchanged)
{
    EXPECT_EQ(level_text(7), L"\x2022 ");
}
