// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODF -> OOXML conversion of text:meta and text:meta-field (inline text with RDF metadata): the text inside is
// written where the element stood, in text documents, spreadsheets and presentations. Before, the elements were
// not read at all, so their text was lost.

#include "gtest/gtest.h"
#include "common.h"

#include <map>
#include <string>
#include <vector>

namespace
{
std::wstring part(const std::wstring& file, const std::wstring& name)
{
    static std::map<std::wstring, std::wstring> dirs;
    auto it = dirs.find(file);
    if (it == dirs.end())
        it = dirs.emplace(file, convert_odf_to_ooxml_dir(L"ExampleFiles/" + file)).first;
    return it->second.empty() ? std::wstring() : read_part(it->second, name);
}

bool starts_tag(const std::wstring& xml, size_t pos, const std::wstring& tag)
{
    const size_t end = pos + tag.size();
    return xml.compare(pos, tag.size(), tag) == 0 && end < xml.size() && (xml[end] == L'>' || xml[end] == L' ');
}

// The elements <outer ...>...</outer>, in document order. Fits these fixtures only: no nesting of outer.
std::vector<std::wstring> elements(const std::wstring& xml, const std::wstring& outer)
{
    std::vector<std::wstring> items;
    const std::wstring open = L"<" + outer, close = L"</" + outer + L">";
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

// The text of an element: its <inner> parts joined (no entity decoding, enough for these fixtures).
std::wstring joined(const std::wstring& element, const std::wstring& inner)
{
    std::wstring text;
    for (const auto& t : elements(element, inner))
    {
        const size_t open_end = t.find(L'>');
        text += t.substr(open_end + 1, t.size() - open_end - 1 - inner.size() - 3);
    }
    return text;
}

// The element <outer> whose joined <inner> text is exactly text, or an empty string.
std::wstring element_with_text(const std::wstring& xml, const std::wstring& outer, const std::wstring& inner,
                               const std::wstring& text)
{
    for (const auto& e : elements(xml, outer))
        if (joined(e, inner) == text) return e;
    return std::wstring();
}

std::vector<std::wstring> texts(const std::wstring& xml, const std::wstring& outer, const std::wstring& inner)
{
    std::vector<std::wstring> items;
    for (const auto& e : elements(xml, outer))
        items.push_back(joined(e, inner));
    return items;
}
}

TEST(OdfTextMeta, OdtMetaTextStaysBetweenTheTextAround)
{
    const std::wstring xml = part(L"text_meta.odt", L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"Before kept after").empty()) << xml;
}

TEST(OdfTextMeta, OdtMetaFieldKeepsItsValue)
{
    const std::wstring xml = part(L"text_meta.odt", L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"Field: 42").empty()) << xml;
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"xnested").empty()) << xml;
}

TEST(OdfTextMeta, OdtSpanInsideMetaKeepsItsFormatting)
{
    const std::wstring xml = part(L"text_meta.odt", L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    ASSERT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"bold rest").empty()) << xml;
    const std::wstring run = element_with_text(xml, L"w:r", L"w:t", L"bold");
    ASSERT_FALSE(run.empty()) << xml;
    EXPECT_NE(run.find(L"<w:b/>"), std::wstring::npos) << run;
}

TEST(OdfTextMeta, OdtMetaInsideSpanTakesTheSpanFormatting)
{
    const std::wstring xml = part(L"text_meta.odt", L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    ASSERT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"in span").empty()) << xml;
    const std::wstring paragraph = element_with_text(xml, L"w:p", L"w:t", L"in span");
    for (const auto& run : elements(paragraph, L"w:r"))
        if (!joined(run, L"w:t").empty())
            EXPECT_NE(run.find(L"<w:b/>"), std::wstring::npos) << run;
}

TEST(OdfTextMeta, OdtSpacesAndMetaOnlyParagraphs)
{
    const std::wstring xml = part(L"text_meta.odt", L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"a  b").empty()) << xml;
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"only").empty()) << xml;
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"plain").empty()) << xml;
}

TEST(OdfTextMeta, OdtHeaderKeepsMetaText)
{
    const std::wstring xml = part(L"text_meta.odt", L"word/header1.xml");
    ASSERT_FALSE(xml.empty()) << "word/header1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_FALSE(element_with_text(xml, L"w:p", L"w:t", L"Head mark end").empty()) << xml;
}

TEST(OdfTextMeta, OdsCellsKeepMetaText)
{
    const std::wstring xml = part(L"text_meta.ods", L"xl/sharedStrings.xml");
    ASSERT_FALSE(xml.empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
    const std::vector<std::wstring> expected = {L"Cell kept", L"field 7", L"rich tail", L"plain"};
    EXPECT_EQ(texts(xml, L"si", L"t"), expected) << xml;
}

TEST(OdfTextMeta, OdsSpanInsideMetaKeepsItsFormatting)
{
    const std::wstring xml = part(L"text_meta.ods", L"xl/sharedStrings.xml");
    ASSERT_FALSE(xml.empty()) << "ODS -> XLSX conversion failed";
    const std::wstring run = element_with_text(xml, L"r", L"t", L"rich");
    ASSERT_FALSE(run.empty()) << xml;
    EXPECT_NE(run.find(L"<b"), std::wstring::npos) << run;
}

TEST(OdfTextMeta, OdsHeaderKeepsMetaText)
{
    const std::wstring xml = part(L"text_meta.ods", L"xl/worksheets/sheet1.xml");
    ASSERT_FALSE(xml.empty()) << "ODS -> XLSX conversion failed";
    EXPECT_NE(xml.find(L"<oddHeader>&amp;CTop mark end</oddHeader>"), std::wstring::npos) << xml;
}

TEST(OdfTextMeta, OdpTextBoxKeepsMetaText)
{
    const std::wstring xml = part(L"text_meta.odp", L"ppt/slides/slide1.xml");
    ASSERT_FALSE(xml.empty()) << "ODP -> PPTX conversion failed";
    EXPECT_TRUE(is_well_formed(xml));
    const std::vector<std::wstring> expected = {L"Slide kept text", L"Field 9", L"plain"};
    EXPECT_EQ(texts(xml, L"a:p", L"a:t"), expected) << xml;
}
