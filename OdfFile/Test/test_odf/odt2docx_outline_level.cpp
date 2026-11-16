// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of heading outline levels: ODF counts them from 1 (up to 10),
// w:outlineLvl counts from 0 and has 9 levels.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// w:outlineLvl of the paragraph whose text is `text`, or -1 when it has none.
int paragraph_outline_level(const std::wstring& document, const std::wstring& text)
{
    const size_t marker = document.find(L">" + text + L"</w:t>");
    if (marker == std::wstring::npos)
        return -2;
    const size_t start = document.rfind(L"<w:p>", marker);
    if (start == std::wstring::npos)
        return -2;
    const std::wstring paragraph = document.substr(start, marker - start);

    const std::wstring tag = L"<w:outlineLvl w:val=\"";
    const size_t level = paragraph.find(tag);
    if (level == std::wstring::npos)
        return -1;
    return std::stoi(paragraph.substr(level + tag.size()));
}

// w:ilvl of the paragraph whose text is `text`, or -1 when it has none.
int paragraph_list_level(const std::wstring& document, const std::wstring& text)
{
    const size_t marker = document.find(L">" + text + L"</w:t>");
    if (marker == std::wstring::npos)
        return -2;
    const size_t start = document.rfind(L"<w:p>", marker);
    if (start == std::wstring::npos)
        return -2;
    const std::wstring paragraph = document.substr(start, marker - start);

    const std::wstring tag = L"<w:ilvl w:val=\"";
    const size_t level = paragraph.find(tag);
    if (level == std::wstring::npos)
        return -1;
    return std::stoi(paragraph.substr(level + tag.size()));
}

// The paragraph whose text is `text`, from <w:p> up to its text, or an empty string.
std::wstring paragraph_xml(const std::wstring& document, const std::wstring& text)
{
    const size_t marker = document.find(L">" + text + L"</w:t>");
    if (marker == std::wstring::npos)
        return std::wstring();
    const size_t start = document.rfind(L"<w:p>", marker);
    if (start == std::wstring::npos)
        return std::wstring();
    return document.substr(start, marker - start);
}

size_t count_of(const std::wstring& text, const std::wstring& what)
{
    size_t count = 0;
    for (size_t pos = text.find(what); pos != std::wstring::npos; pos = text.find(what, pos + what.size()))
        ++count;
    return count;
}

// w:outlineLvl of the style named `name`, or -1 when it has none.
int style_outline_level(const std::wstring& styles, const std::wstring& name)
{
    const size_t marker = styles.find(L"<w:name w:val=\"" + name + L"\"/>");
    if (marker == std::wstring::npos)
        return -2;
    const size_t end = styles.find(L"</w:style>", marker);
    if (end == std::wstring::npos)
        return -2;
    const std::wstring style = styles.substr(marker, end - marker);

    const std::wstring tag = L"<w:outlineLvl w:val=\"";
    const size_t level = style.find(tag);
    if (level == std::wstring::npos)
        return -1;
    return std::stoi(style.substr(level + tag.size()));
}

struct ConvertedParts
{
    std::wstring document;
    std::wstring styles;
};

ConvertedParts convert(const std::wstring& file)
{
    ConvertedParts result;
    const std::wstring dir = convert_odf_to_ooxml_dir(file);
    if (!dir.empty())
    {
        result.document = read_part(dir, L"word/document.xml");
        result.styles   = read_part(dir, L"word/styles.xml");
    }
    return result;
}

// Each fixture is converted once for all tests that use it.
const ConvertedParts& converted()
{
    static const ConvertedParts parts = convert(L"ExampleFiles/outline_levels.odt");
    return parts;
}

// Visible outline numbering ("1" on every level): a heading on its style's level, a
// heading in a numbered list and a heading on outline level 0 (body text).
const ConvertedParts& converted_with_outline_numbering()
{
    static const ConvertedParts parts = convert(L"ExampleFiles/outline_levels_numbered.odt");
    return parts;
}

// The same document with a text:outline-style, which LibreOffice writes into every
// text document, plus a level-10 heading whose style has no outline level, a heading
// in a numbered list and a heading whose automatic style turns lists off.
const ConvertedParts& converted_with_outline_style()
{
    static const ConvertedParts parts = convert(L"ExampleFiles/outline_levels_outline_style.odt");
    return parts;
}

class OdtToDocxOutlineLevel : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_FALSE(document_.empty()) << "ODT -> DOCX conversion failed or word/document.xml is missing";
        ASSERT_FALSE(styles_.empty()) << "word/styles.xml is missing";
    }

    const std::wstring& document_ = converted().document;
    const std::wstring& styles_   = converted().styles;
};

TEST_F(OdtToDocxOutlineLevel, HeadingStylesStartAtZero)
{
    EXPECT_EQ(style_outline_level(styles_, L"Heading 1"), 0);
    EXPECT_EQ(style_outline_level(styles_, L"Heading 2"), 1);
}

TEST_F(OdtToDocxOutlineLevel, HeadingsStartAtZero)
{
    EXPECT_EQ(paragraph_outline_level(document_, L"Level one"), 0);
    EXPECT_EQ(paragraph_outline_level(document_, L"Level two"), 1);
    EXPECT_EQ(paragraph_outline_level(document_, L"Automatic two"), 1);
}

// text:outline-level on the heading wins over the level of its paragraph style.
TEST_F(OdtToDocxOutlineLevel, HeadingKeepsItsOwnLevel)
{
    EXPECT_EQ(paragraph_outline_level(document_, L"Own level three"), 2);
    EXPECT_EQ(paragraph_outline_level(document_, L"Custom two"), 1);
}

// ODF level 10 has no Word counterpart: it becomes Word's last level, 8.
TEST_F(OdtToDocxOutlineLevel, LevelTenBecomesLastWordLevel)
{
    EXPECT_EQ(style_outline_level(styles_, L"Heading 10"), 8);
    EXPECT_EQ(paragraph_outline_level(document_, L"Level ten"), 8);
}

TEST_F(OdtToDocxOutlineLevel, BodyTextHasNoLevel)
{
    EXPECT_EQ(paragraph_outline_level(document_, L"Body"), -1);
}

class OdtToDocxOutlineLevelWithOutlineStyle : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_FALSE(document_.empty()) << "ODT -> DOCX conversion failed or word/document.xml is missing";
        ASSERT_FALSE(styles_.empty()) << "word/styles.xml is missing";
    }

    const std::wstring& document_ = converted_with_outline_style().document;
    const std::wstring& styles_   = converted_with_outline_style().styles;
};

// Headings on their style's level take the level from the style.
TEST_F(OdtToDocxOutlineLevelWithOutlineStyle, HeadingStylesStartAtZero)
{
    EXPECT_EQ(style_outline_level(styles_, L"Heading 1"), 0);
    EXPECT_EQ(style_outline_level(styles_, L"Heading 2"), 1);
    EXPECT_EQ(style_outline_level(styles_, L"Heading 10"), 8);
}

TEST_F(OdtToDocxOutlineLevelWithOutlineStyle, HeadingKeepsItsOwnLevel)
{
    EXPECT_EQ(paragraph_outline_level(document_, L"Own level three"), 2);
    EXPECT_EQ(paragraph_outline_level(document_, L"Custom two"), 1);
    EXPECT_EQ(paragraph_outline_level(document_, L"Custom ten"), 8);
}

// Word list levels run from 0 to 8: level 10 gets no outline numbering at all.
TEST_F(OdtToDocxOutlineLevelWithOutlineStyle, LevelTenGetsNoListLevelNine)
{
    EXPECT_EQ(paragraph_list_level(document_, L"Custom ten"), -1);
    EXPECT_EQ(paragraph_list_level(document_, L"Own level three"), 2);
}

// A heading in a list takes its numbering from the list: one w:numPr, not a second
// one from the outline numbering of its heading style.
TEST_F(OdtToDocxOutlineLevelWithOutlineStyle, ListedHeadingHasOneNumbering)
{
    const std::wstring paragraph = paragraph_xml(document_, L"Listed one");
    ASSERT_FALSE(paragraph.empty());
    EXPECT_EQ(count_of(paragraph, L"<w:numPr>"), 1u);
    EXPECT_EQ(paragraph.find(L"<w:numId w:val=\"0\"/>"), std::wstring::npos);
    EXPECT_EQ(paragraph_outline_level(document_, L"Listed one"), 0);
}

// w:numId 0 cancels the outline numbering of a heading style. A heading whose style
// chain has no outline level has nothing to cancel and keeps its style's indentation.
TEST_F(OdtToDocxOutlineLevelWithOutlineStyle, HeadingOnPlainStyleGetsNoNumbering)
{
    const std::wstring paragraph = paragraph_xml(document_, L"Unlisted one");
    ASSERT_FALSE(paragraph.empty());
    EXPECT_EQ(paragraph.find(L"<w:numPr>"), std::wstring::npos);
    EXPECT_EQ(paragraph_outline_level(document_, L"Unlisted one"), 0);
}

class OdtToDocxOutlineLevelWithOutlineNumbering : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_FALSE(document_.empty()) << "ODT -> DOCX conversion failed or word/document.xml is missing";
    }

    const std::wstring& document_ = converted_with_outline_numbering().document;
};

// The list's numbering wins over the visible outline numbering of the heading style.
TEST_F(OdtToDocxOutlineLevelWithOutlineNumbering, ListedHeadingHasOneNumbering)
{
    const std::wstring paragraph = paragraph_xml(document_, L"Listed numbered");
    ASSERT_FALSE(paragraph.empty());
    EXPECT_EQ(count_of(paragraph, L"<w:numPr>"), 1u);
    EXPECT_EQ(paragraph_outline_level(document_, L"Listed numbered"), 0);
}

// A heading on its style's level takes level and numbering from the style.
TEST_F(OdtToDocxOutlineLevelWithOutlineNumbering, HeadingOnStyleLevelUsesTheStyle)
{
    const std::wstring paragraph = paragraph_xml(document_, L"Numbered one");
    ASSERT_FALSE(paragraph.empty());
    EXPECT_EQ(paragraph.find(L"<w:numPr>"), std::wstring::npos);
    EXPECT_EQ(paragraph_outline_level(document_, L"Numbered one"), -1);
}

// Outline level 0 is body text: no outline numbering, and never w:ilvl -1.
TEST_F(OdtToDocxOutlineLevelWithOutlineNumbering, LevelZeroGetsNoNegativeListLevel)
{
    EXPECT_EQ(paragraph_list_level(document_, L"Level zero"), 0);
    EXPECT_NE(paragraph_xml(document_, L"Level zero").find(L"<w:numId w:val=\"0\"/>"), std::wstring::npos);
    EXPECT_EQ(paragraph_outline_level(document_, L"Level zero"), 9);
}
} // namespace
