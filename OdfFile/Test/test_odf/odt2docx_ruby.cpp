// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of text:ruby: the base text is written as normal runs and the annotation is
// dropped, in the body and in page headers. Before, both were lost. A drop cap can
// start with a ruby.

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

// The elements <tag ...>...</tag>, in document order. Fits this fixture only: no nesting of tag.
std::vector<std::wstring> elements(const std::wstring& xml, const std::wstring& tag)
{
    std::vector<std::wstring> items;
    const std::wstring open = L"<" + tag, close = L"</" + tag + L">";
    size_t pos = 0;
    while ((pos = xml.find(open, pos)) != std::wstring::npos)
    {
        if (!starts_tag(xml, pos, open)) { pos += open.size(); continue; }
        const size_t end = xml.find(close, pos);
        if (end == std::wstring::npos) break;
        items.push_back(xml.substr(pos, end + close.size() - pos));
        pos = end;
    }
    return items;
}

// The text of a paragraph, with its <w:t> parts joined. No entity decoding, enough for this fixture.
std::wstring paragraph_text(const std::wstring& paragraph)
{
    std::wstring text;
    for (const auto& t : elements(paragraph, L"w:t"))
    {
        const size_t open_end = t.find(L'>');
        text += t.substr(open_end + 1, t.size() - open_end - 1 - 6);
    }
    return text;
}

// The text of every <w:p>. No paragraphs nested in text boxes in this fixture.
std::vector<std::wstring> paragraphs(const std::wstring& xml)
{
    std::vector<std::wstring> items;
    for (const auto& p : elements(xml, L"w:p"))
        items.push_back(paragraph_text(p));
    return items;
}

bool has_paragraph(const std::wstring& xml, const std::wstring& text)
{
    for (const auto& item : paragraphs(xml))
        if (item == text) return true;
    return false;
}

// The <w:p> element whose text is exactly text, or an empty string.
std::wstring paragraph_with(const std::wstring& xml, const std::wstring& text)
{
    for (const auto& p : elements(xml, L"w:p"))
        if (paragraph_text(p) == text) return p;
    return std::wstring();
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
    // the element must still be open at the text, not closed before it
    if (xml.find(L"</" + tag + L">", open) < at) return std::wstring();
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

// A drop cap paragraph that starts with a ruby takes its letter from the ruby base (also through a span inside the
// base, or a span around the ruby) and splits the base text there. Before, the ruby's runs were written first and
// the drop cap paragraph properties then ended up inside a run (<w:r>...<w:pPr>, invalid DOCX).
TEST(OdtToDocxRuby, DropCapTakesItsLetterFromRubyBase)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
    for (const std::wstring letter : {L"大", L"神", L"札"})
    {
        const std::wstring paragraph = paragraph_with(xml, letter);
        ASSERT_FALSE(paragraph.empty()) << "no paragraph " << letter << " in\n" << xml;
        EXPECT_NE(paragraph.find(L"w:dropCap=\"drop\""), std::wstring::npos) << paragraph;
        // the paragraph properties come first in the paragraph, not inside a run
        EXPECT_EQ(paragraph.find(L"<w:pPr>"), paragraph.find(L'>') + 1) << paragraph;
    }
    EXPECT_TRUE(has_paragraph(xml, L"阪 drop one")) << xml;
    EXPECT_TRUE(has_paragraph(xml, L"戸 drop two")) << xml;
    EXPECT_TRUE(has_paragraph(xml, L"幌 drop three")) << xml;
}

TEST(OdtToDocxRuby, DropCapParagraphPropertiesStayOutsideRuns)
{
    const std::wstring xml = part(L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    for (const auto& run : elements(xml, L"w:r"))
        EXPECT_EQ(run.find(L"<w:pPr"), std::wstring::npos) << run;
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
