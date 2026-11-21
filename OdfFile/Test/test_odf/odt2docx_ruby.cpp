// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of text:ruby: the base text is written as normal runs and the annotation is
// dropped, in the body and in page headers. Before, both were lost.

#include "gtest/gtest.h"
#include "common.h"

#include <string>
#include <vector>

namespace
{
const std::wstring& out_dir()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/ruby_base_text.odt");
    return dir;
}

std::wstring part(const std::wstring& name)
{
    return out_dir().empty() ? std::wstring() : read_part(out_dir(), name);
}

bool starts_tag(const std::wstring& xml, size_t pos, const std::wstring& tag)
{
    const size_t end = pos + tag.size();
    return xml.compare(pos, tag.size(), tag) == 0 && end < xml.size() && (xml[end] == L'>' || xml[end] == L' ');
}

// The text of every <w:p>, with its <w:t> parts joined. Fits this fixture only: no entity decoding and no
// paragraphs nested in text boxes.
std::vector<std::wstring> paragraphs(const std::wstring& xml)
{
    std::vector<std::wstring> items;
    size_t pos = 0;
    while ((pos = xml.find(L"<w:p", pos)) != std::wstring::npos)
    {
        if (!starts_tag(xml, pos, L"<w:p")) { pos += 4; continue; }
        const size_t end = xml.find(L"</w:p>", pos);
        if (end == std::wstring::npos) break;
        std::wstring text;
        for (size_t t = xml.find(L"<w:t", pos); t != std::wstring::npos && t < end; t = xml.find(L"<w:t", t + 4))
        {
            if (!starts_tag(xml, t, L"<w:t")) continue;
            const size_t open_end = xml.find(L'>', t);
            const size_t close = xml.find(L"</w:t>", open_end);
            if (close == std::wstring::npos || close > end) break;
            text += xml.substr(open_end + 1, close - open_end - 1);
        }
        items.push_back(text);
        pos = end;
    }
    return items;
}

bool has_paragraph(const std::wstring& xml, const std::wstring& text)
{
    for (const auto& item : paragraphs(xml))
        if (item == text) return true;
    return false;
}

// The element <tag ...>...</tag> that holds the text, or an empty string.
std::wstring element_with(const std::wstring& xml, const std::wstring& tag, const std::wstring& text)
{
    const size_t at = xml.find(text);
    if (at == std::wstring::npos) return std::wstring();
    size_t open = at;
    while ((open = xml.rfind(L"<" + tag, open)) != std::wstring::npos && !starts_tag(xml, open, L"<" + tag))
    {
        if (open == 0) return std::wstring();
        --open;
    }
    if (open == std::wstring::npos) return std::wstring();
    const size_t close = xml.find(L"</" + tag + L">", at);
    if (close == std::wstring::npos) return std::wstring();
    return xml.substr(open, close + tag.size() + 3 - open);
}
}

TEST(OdtToDocxRuby, BaseTextStaysBetweenTheTextAround)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_TRUE(has_paragraph(xml, L"Tokyo: 東京 station")) << xml;
}

TEST(OdtToDocxRuby, BaseTextInsideSpanIsKept)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(has_paragraph(xml, L"漢字 text")) << xml;
}

TEST(OdtToDocxRuby, BaseTextInsideSpanKeepsSpanFormatting)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring run = element_with(xml, L"w:r", L"漢字");
    ASSERT_FALSE(run.empty()) << xml;
    EXPECT_NE(run.find(L"<w:b/>"), std::wstring::npos) << run;
}

TEST(OdtToDocxRuby, BaseTextInsideHyperlinkStaysInTheLink)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(has_paragraph(xml, L"Link: 字 end")) << xml;
    EXPECT_FALSE(element_with(xml, L"w:hyperlink", L">字</w:t>").empty()) << xml;
}

TEST(OdtToDocxRuby, ParagraphWithOnlyRubyKeepsItsText)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(has_paragraph(xml, L"上")) << xml;
}

TEST(OdtToDocxRuby, AnnotationIsDropped)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(xml.find(L"とうきょう"), std::wstring::npos) << xml;
    EXPECT_EQ(xml.find(L"かんじ"), std::wstring::npos) << xml;
    EXPECT_EQ(xml.find(L"shang"), std::wstring::npos) << xml;
    EXPECT_EQ(xml.find(L"じ"), std::wstring::npos) << xml;
}

TEST(OdtToDocxRuby, ParagraphWithoutRubyUnchanged)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(has_paragraph(xml, L"plain")) << xml;
}

TEST(OdtToDocxRuby, HeaderKeepsBaseTextOnly)
{
    const std::wstring xml = part(L"word/header1.xml");
    ASSERT_FALSE(xml.empty()) << "word/header1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_TRUE(has_paragraph(xml, L"Head 本 end")) << xml;
    EXPECT_EQ(xml.find(L"ほん"), std::wstring::npos) << xml;
}
