// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of a heading style's outline numbering: whether the style gets the
// outline numbering or w:numId 0 must depend on the number format of its own outline level.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// Value of the attribute `tag` (e.g. L"<w:numId w:val=\"") inside the style named `name`,
// -1 when the style has none, -2 when the style is missing.
int style_value(const std::wstring& styles, const std::wstring& name, const std::wstring& tag)
{
    const size_t marker = styles.find(L"<w:name w:val=\"" + name + L"\"/>");
    if (marker == std::wstring::npos)
        return -2;
    const size_t end = styles.find(L"</w:style>", marker);
    if (end == std::wstring::npos)
        return -2;
    const std::wstring style = styles.substr(marker, end - marker);

    const size_t value = style.find(tag);
    if (value == std::wstring::npos)
        return -1;
    return std::stoi(style.substr(value + tag.size()));
}

int style_num_id(const std::wstring& styles, const std::wstring& name)
{
    return style_value(styles, name, L"<w:numId w:val=\"");
}

int style_list_level(const std::wstring& styles, const std::wstring& name)
{
    return style_value(styles, name, L"<w:ilvl w:val=\"");
}

// Outline numbering on levels 1 and 3 only, level 2 has no number format. The heading
// styles have paragraph properties and the outline levels 1, 2 and 3.
const std::wstring& converted_styles()
{
    static const std::wstring styles = []
    {
        const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/outline_numbering_gaps.odt");
        return dir.empty() ? std::wstring() : read_part(dir, L"word/styles.xml");
    }();
    return styles;
}

class OdtToDocxOutlineNumFormat : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_FALSE(styles_.empty()) << "ODT -> DOCX conversion failed or word/styles.xml is missing";
    }

    const std::wstring& styles_ = converted_styles();
};
} // namespace

TEST_F(OdtToDocxOutlineNumFormat, NumberedLevelKeepsOutlineNumbering)
{
    EXPECT_GT(style_num_id(styles_, L"Heading 1"), 0);
    EXPECT_EQ(style_list_level(styles_, L"Heading 1"), 0);
}

TEST_F(OdtToDocxOutlineNumFormat, LevelWithoutNumberFormatGetsNoNumbering)
{
    EXPECT_EQ(style_num_id(styles_, L"Heading 2"), 0);
}

TEST_F(OdtToDocxOutlineNumFormat, LastLevelKeepsOutlineNumbering)
{
    EXPECT_EQ(style_num_id(styles_, L"Heading 3"), style_num_id(styles_, L"Heading 1"));
    EXPECT_EQ(style_list_level(styles_, L"Heading 3"), 2);
}
