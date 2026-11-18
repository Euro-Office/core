// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// White space in the text of ODF paragraphs (ODF 1.3 part 3, 6.1.2): every
// run of SPACE, TAB, LF and CR is one space, white space at the start of a
// paragraph and after white space is ignored, also across spans; text:s,
// text:tab and text:line-break are not white space. LibreOffice Calc keeps
// the characters of cell text as they are.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The text of the paragraph element (open tag "<w:p" or "<a:p") of a part
// that contains the given text: all text elements (w:t or a:t) in order.
std::wstring paragraph_text(const std::wstring& xml, const std::wstring& paragraph, const std::wstring& text_element,
                            const std::wstring& text)
{
    const size_t at = xml.find(text);
    if (at == std::wstring::npos)
        return L"";
    size_t begin = std::wstring::npos;
    for (const std::wstring open : {paragraph + L">", paragraph + L" "}) // not "<w:pPr>"
    {
        const size_t found = xml.rfind(open, at);
        if (found != std::wstring::npos && (begin == std::wstring::npos || found > begin))
            begin = found;
    }
    const size_t end = xml.find(L"</" + paragraph.substr(1) + L">", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";

    std::wstring result;
    const std::wstring open = L"<" + text_element;
    const std::wstring close = L"</" + text_element + L">";
    for (size_t t = xml.find(open, begin); t != std::wstring::npos && t < end; t = xml.find(open, t + 1))
    {
        const wchar_t next = xml[t + open.size()];
        if (next != L'>' && next != L' ')
            continue; // <w:tab/>, <w:tc>, ...
        const size_t open_end = xml.find(L'>', t);
        const size_t text_end = xml.find(close, open_end);
        if (open_end == std::wstring::npos || text_end == std::wstring::npos || text_end > end)
            break;
        result += xml.substr(open_end + 1, text_end - open_end - 1);
    }
    return result;
}

class OdtToDocxWhiteSpace : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/white_space_paragraph.odt");
        document_ = dir.empty() ? L"" : read_part(dir, L"word/document.xml");
    }
    static std::wstring text_of(const std::wstring& text)
    {
        return paragraph_text(document_, L"<w:p", L"w:t", text);
    }
    void SetUp() override
    {
        ASSERT_FALSE(document_.empty()) << "ODT -> DOCX conversion failed";
    }
    static std::wstring document_;
};
std::wstring OdtToDocxWhiteSpace::document_;
} // namespace

TEST_F(OdtToDocxWhiteSpace, LineBreakAndTabInTextAreSpaces)
{
    EXPECT_EQ(text_of(L"Alpha"), L"Alpha Beta Gamma");
}

TEST_F(OdtToDocxWhiteSpace, WhiteSpaceBetweenSpansIsOneSpace)
{
    EXPECT_EQ(text_of(L"Delta"), L"Delta Epsilon");
}

TEST_F(OdtToDocxWhiteSpace, SpacesCollapse)
{
    EXPECT_EQ(text_of(L"Zeta"), L"Zeta Eta");
}

TEST_F(OdtToDocxWhiteSpace, LeadingWhiteSpaceIgnored)
{
    EXPECT_EQ(text_of(L"Theta"), L"Theta");
}

TEST_F(OdtToDocxWhiteSpace, TextSpacesKept)
{
    EXPECT_EQ(text_of(L"Iota"), L"Iota  Kappa");
}

TEST_F(OdtToDocxWhiteSpace, SpaceAfterLineBreakIsOneSpace)
{
    EXPECT_EQ(text_of(L"Lambda"), L"Lambda Mu");
}

TEST_F(OdtToDocxWhiteSpace, BookmarkDoesNotEndWhiteSpace)
{
    EXPECT_EQ(text_of(L"Nu"), L"Nu Xi");
}

TEST_F(OdtToDocxWhiteSpace, WhiteSpaceAcrossSpansIsOneSpace)
{
    EXPECT_EQ(text_of(L"Omicron"), L"Omicron Pi");
}

TEST(OdpToPptxWhiteSpace, LineBreaksInTextAreSpaces)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/white_space_slide.odp");
    ASSERT_FALSE(dir.empty()) << "ODP -> PPTX conversion failed";
    const std::wstring slide = read_part(dir, L"ppt/slides/slide1.xml");

    EXPECT_EQ(paragraph_text(slide, L"<a:p", L"a:t", L"Toolkit"), L"The ODF Toolkit provides a home");
}

TEST(OdsToXlsxWhiteSpace, CellTextKeepsSpaces)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/white_space_cell.ods");
    ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";
    const std::wstring strings = read_part(dir, L"xl/sharedStrings.xml");

    EXPECT_NE(strings.find(L"Rho  Sigma"), std::wstring::npos) << strings;
}
