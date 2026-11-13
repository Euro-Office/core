// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of styles without a parent: the w:basedOn the
// WordprocessingML writer puts into word/styles.xml. A style without a parent
// is based on the default paragraph style only when it is a paragraph style;
// w:basedOn must name a style of the same type.

#include "gtest/gtest.h"

#include "common.h"

#include <map>
#include <string>

namespace
{
// Value of attribute `name` in the start tag `tag`, or "" when it is missing.
std::wstring attribute(const std::wstring& tag, const std::wstring& name)
{
    const std::wstring key = L" " + name + L"=\"";
    const size_t at        = tag.find(key);
    if (at == std::wstring::npos)
        return L"";
    const size_t begin = at + key.size();
    const size_t end   = tag.find(L'"', begin);
    return end == std::wstring::npos ? L"" : tag.substr(begin, end - begin);
}

struct Style
{
    std::wstring type;
    std::wstring name;
    std::wstring basedOn;
};

// Every w:style of word/styles.xml by w:styleId.
std::map<std::wstring, Style> styles_by_id(const std::wstring& styles)
{
    std::map<std::wstring, Style> result;
    for (size_t at = styles.find(L"<w:style "); at != std::wstring::npos; at = styles.find(L"<w:style ", at + 1))
    {
        const size_t tagEnd = styles.find(L'>', at);
        const size_t end    = styles.find(L"</w:style>", at);
        if (tagEnd == std::wstring::npos || end == std::wstring::npos)
            break;
        const std::wstring tag  = styles.substr(at, tagEnd - at);
        const std::wstring body = styles.substr(tagEnd, end - tagEnd);

        Style style;
        style.type = attribute(tag, L"w:type");
        const size_t name = body.find(L"<w:name ");
        if (name != std::wstring::npos)
            style.name = attribute(body.substr(name, body.find(L'>', name) - name), L"w:val");
        const size_t basedOn = body.find(L"<w:basedOn ");
        if (basedOn != std::wstring::npos)
            style.basedOn = attribute(body.substr(basedOn, body.find(L'>', basedOn) - basedOn), L"w:val");
        result[attribute(tag, L"w:styleId")] = style;
    }
    return result;
}

const Style* style_named(const std::map<std::wstring, Style>& styles, const std::wstring& name)
{
    for (const auto& entry : styles)
        if (entry.second.name == name)
            return &entry.second;
    return nullptr;
}
} // namespace

TEST(OdtToDocxStyleBasedOn, CharacterStyleIsNotBasedOnAParagraphStyle)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/character_style_without_parent.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml                     = read_part(dir, L"word/styles.xml");
    const std::map<std::wstring, Style> styles = styles_by_id(xml);

    const Style* strong = style_named(styles, L"Strong");
    ASSERT_NE(strong, nullptr) << xml;
    EXPECT_EQ(strong->type, L"character") << xml;
    EXPECT_EQ(strong->basedOn, L"") << xml;

    // A paragraph style without a parent is still based on the default paragraph style.
    const Style* plain = style_named(styles, L"Plain");
    ASSERT_NE(plain, nullptr) << xml;
    ASSERT_NE(plain->basedOn, L"") << xml;
    ASSERT_EQ(styles.count(plain->basedOn), 1u) << xml;
    EXPECT_EQ(styles.at(plain->basedOn).type, L"paragraph") << xml;

    // No style is based on a style of another type.
    for (const auto& entry : styles)
    {
        if (entry.second.basedOn.empty())
            continue;
        ASSERT_EQ(styles.count(entry.second.basedOn), 1u) << "unknown w:basedOn of " << entry.first;
        EXPECT_EQ(styles.at(entry.second.basedOn).type, entry.second.type) << "w:basedOn of " << entry.first;
    }

    // The run still uses the character style.
    std::wstring strongId;
    for (const auto& entry : styles)
        if (&entry.second == strong)
            strongId = entry.first;
    const std::wstring document = read_part(dir, L"word/document.xml");
    EXPECT_NE(document.find(L"<w:rStyle w:val=\"" + strongId + L"\""), std::wstring::npos) << document;
}
