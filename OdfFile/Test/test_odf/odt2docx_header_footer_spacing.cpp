// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of the distance between a header or footer and the
// page body (fo:margin-bottom of the header style, fo:margin-top of the footer
// style). WordprocessingML has no such distance in w:pgMar; it is written as
// paragraph spacing of the header's last and the footer's first paragraph.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// All header and footer parts of the package in one string.
std::wstring headers_and_footers(const std::wstring& dir)
{
    std::wstring all;
    for (int i = 1; i <= 40; ++i)
    {
        all += read_part(dir, L"word/header" + std::to_wstring(i) + L".xml");
        all += read_part(dir, L"word/footer" + std::to_wstring(i) + L".xml");
    }
    return all;
}

// The w:p element that holds the given text.
std::wstring paragraph_with(const std::wstring& xml, const std::wstring& text)
{
    const size_t at = xml.find(text);
    if (at == std::wstring::npos)
        return L"";
    // "<w:p>" and not "<w:p": that would also match "<w:pPr>".
    const size_t begin = xml.rfind(L"<w:p>", at);
    const size_t end   = xml.find(L"</w:p>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";
    return xml.substr(begin, end + 6 - begin);
}

size_t count_of(const std::wstring& xml, const std::wstring& what)
{
    size_t n = 0;
    for (size_t at = xml.find(what); at != std::wstring::npos; at = xml.find(what, at + what.size()))
        ++n;
    return n;
}

bool has(const std::wstring& xml, const std::wstring& what)
{
    return xml.find(what) != std::wstring::npos;
}

class OdtToDocxHeaderFooterSpacing : public ::testing::Test
{
protected:
    void SetUp() override
    {
        dir = convert_odf_to_ooxml_dir(L"ExampleFiles/header_footer_spacing.odt");
        ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
        parts = headers_and_footers(dir);
        ASSERT_FALSE(parts.empty()) << "no header or footer part";
    }

    // The paragraph with the text; fails the test when it is missing or has more than one w:spacing.
    std::wstring paragraph(const std::wstring& text)
    {
        const std::wstring p = paragraph_with(parts, text);
        EXPECT_FALSE(p.empty()) << "paragraph " << text << " not found";
        EXPECT_LE(count_of(p, L"<w:spacing"), 1u) << p;
        return p;
    }

    std::wstring dir;
    std::wstring parts;
};
}

// 0.5cm = 283 twips below the header and above the footer.
TEST_F(OdtToDocxHeaderFooterSpacing, GrowingHeaderAndFooterKeepTheirDistance)
{
    // automatic paragraph style with paragraph properties
    EXPECT_TRUE(has(paragraph(L"GROW-HEADER"), L"w:after=\"283\"")) << paragraph(L"GROW-HEADER");
    // common paragraph style
    EXPECT_TRUE(has(paragraph(L"GROW-FOOTER"), L"w:before=\"283\"")) << paragraph(L"GROW-FOOTER");
    EXPECT_FALSE(has(paragraph(L"GROW-FOOTER"), L"w:after=")) << paragraph(L"GROW-FOOTER");
}

// The page margins stay as they are: the distance is not part of them.
TEST_F(OdtToDocxHeaderFooterSpacing, PageMarginsAreUnchanged)
{
    const std::wstring document = read_part(dir, L"word/document.xml");
    EXPECT_EQ(count_of(document, L"<w:pgMar "), 10u) << document;
    EXPECT_EQ(count_of(document, L"w:top=\"1134\""), 9u) << document; // all but the fixed height one
    EXPECT_EQ(count_of(document, L"w:header=\"1134\""), 10u) << document;
    EXPECT_EQ(count_of(document, L"w:footer=\"1134\""), 10u) << document;
}

// Only the paragraph next to the body gets the distance. In a header it replaces
// the bottom margin of that paragraph, in a footer it is added to the top margin.
TEST_F(OdtToDocxHeaderFooterSpacing, DistanceGoesToTheParagraphNextToTheBody)
{
    EXPECT_FALSE(has(paragraph(L"OWN-HEADER-FIRST"), L"w:after=")) << paragraph(L"OWN-HEADER-FIRST");

    const std::wstring last = paragraph(L"OWN-HEADER-LAST");
    EXPECT_TRUE(has(last, L"w:after=\"283\"")) << last;  // not 0.4cm = 227
    EXPECT_TRUE(has(last, L"w:before=\"113\"")) << last; // 0.2cm, its own

    const std::wstring first = paragraph(L"OWN-FOOTER-FIRST");
    EXPECT_TRUE(has(first, L"w:before=\"454\"")) << first; // 0.3cm + 0.5cm
    EXPECT_TRUE(has(first, L"w:after=\"57\"")) << first;   // 0.1cm, its own

    EXPECT_FALSE(has(paragraph(L"OWN-FOOTER-SECOND"), L"<w:spacing")) << paragraph(L"OWN-FOOTER-SECOND");
}

TEST_F(OdtToDocxHeaderFooterSpacing, StyleWithoutParagraphProperties)
{
    EXPECT_TRUE(has(paragraph(L"PLAIN-HEADER"), L"<w:spacing w:after=\"283\"/>")) << paragraph(L"PLAIN-HEADER");
    EXPECT_TRUE(has(paragraph(L"PLAIN-FOOTER"), L"<w:spacing w:before=\"283\"/>")) << paragraph(L"PLAIN-FOOTER");
}

// A paragraph without a style has no properties of its own, the distance is all it gets.
TEST_F(OdtToDocxHeaderFooterSpacing, ParagraphWithoutStyle)
{
    EXPECT_TRUE(has(paragraph(L"NOSTYLE-HEADER"), L"<w:pPr><w:spacing w:after=\"283\"/></w:pPr>"))
        << paragraph(L"NOSTYLE-HEADER");
}

// The frame paragraph of a drop cap keeps its fixed spacing, the text after it gets the distance.
TEST_F(OdtToDocxHeaderFooterSpacing, DropCapParagraphGetsOneSpacing)
{
    std::wstring header;
    for (int i = 1; i <= 40 && header.empty(); ++i)
    {
        const std::wstring part = read_part(dir, L"word/header" + std::to_wstring(i) + L".xml");
        if (has(part, L"and more text"))
            header = part;
    }
    ASSERT_FALSE(header.empty());

    for (size_t at = header.find(L"<w:pPr>"); at != std::wstring::npos; at = header.find(L"<w:pPr>", at + 1))
    {
        const std::wstring pPr = header.substr(at, header.find(L"</w:pPr>", at) - at);
        EXPECT_LE(count_of(pPr, L"<w:spacing"), 1u) << header;
    }
    EXPECT_TRUE(has(header, L"<w:framePr")) << header;
    EXPECT_EQ(count_of(header, L"w:after=\"283\""), 1u) << header;
}

// A heading gets the distance like a paragraph. Where it also gets outline
// properties, the distance stands after w:numPr and before w:outlineLvl.
TEST_F(OdtToDocxHeaderFooterSpacing, HeadingGetsTheDistance)
{
    const std::wstring heading = paragraph(L"HEADING-HEADER");
    const size_t spacing = heading.find(L"<w:spacing w:after=\"283\"/>");
    ASSERT_NE(spacing, std::wstring::npos) << heading;

    const size_t numPr = heading.find(L"<w:numPr>");
    const size_t outlineLvl = heading.find(L"<w:outlineLvl");
    if (numPr != std::wstring::npos)
        EXPECT_LT(numPr, spacing) << heading;
    if (outlineLvl != std::wstring::npos)
        EXPECT_LT(spacing, outlineLvl) << heading;
}

// A dynamic distance shrinks as the header grows and a fixed height includes
// the distance: in both cases the page margins already describe the layout.
TEST_F(OdtToDocxHeaderFooterSpacing, DynamicDistanceAndFixedHeightGetNone)
{
    EXPECT_FALSE(has(paragraph(L"DYNAMIC-HEADER"), L"<w:spacing")) << paragraph(L"DYNAMIC-HEADER");
    EXPECT_FALSE(has(paragraph(L"FIXED-HEADER"), L"<w:spacing")) << paragraph(L"FIXED-HEADER");
}

// A table or a list next to the body is left as it is: only a plain paragraph
// carries the distance, and no other paragraph of the header gets it instead.
TEST_F(OdtToDocxHeaderFooterSpacing, TableOrListNextToTheBodyGetsNone)
{
    EXPECT_FALSE(has(paragraph(L"TABLE-HEADER"), L"<w:spacing")) << paragraph(L"TABLE-HEADER");
    EXPECT_FALSE(has(paragraph(L"TABLE-CELL"), L"<w:spacing")) << paragraph(L"TABLE-CELL");

    EXPECT_FALSE(has(paragraph(L"LISTED-HEADER-FIRST"), L"<w:spacing")) << paragraph(L"LISTED-HEADER-FIRST");
    EXPECT_FALSE(has(paragraph(L"LISTED-ITEM-ONE"), L"<w:spacing")) << paragraph(L"LISTED-ITEM-ONE");
    EXPECT_FALSE(has(paragraph(L"LISTED-ITEM-LAST"), L"<w:spacing")) << paragraph(L"LISTED-ITEM-LAST");
}

// With the compatibility setting HeaderSpacingBelowLastPara the last paragraph
// of a header keeps its own bottom margin, and the distance is added to it.
TEST(OdtToDocxHeaderFooterSpacingBelowLastPara, HeaderKeepsTheBottomMarginOfItsLastParagraph)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/header_footer_spacing_below_last_para.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring last = paragraph_with(headers_and_footers(dir), L"OWN-HEADER-LAST");
    EXPECT_TRUE(has(last, L"w:after=\"510\"")) << last;  // 0.4cm + 0.5cm
    EXPECT_TRUE(has(last, L"w:before=\"113\"")) << last; // 0.2cm, its own
    EXPECT_EQ(count_of(last, L"<w:spacing"), 1u) << last;
}
