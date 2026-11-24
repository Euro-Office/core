// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT -> DOCX and PPTX -> ODP -> PPTX round trips of single spaces that an
// ODF reader drops when they go out as character data: a space at the start of a
// paragraph, and a space that follows another space at a run boundary, also at the
// edges of a hyperlink. The ODF writer must write them as text:s.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

#include "DesktopEditor/common/Directory.h"
#include "OfficeUtils/src/OfficeUtils.h"

namespace
{
// The text of the paragraph (w:p, or a:p with p = L"a") that contains the given
// text: all w:t / a:t contents of that paragraph, in order.
std::wstring paragraph_text(const std::wstring& document, const std::wstring& text, const std::wstring& p = L"w")
{
    const std::wstring para = L"<" + p + L":p", run_text = L"<" + p + L":t", end_text = L"</" + p + L":t>";
    const size_t at = document.find(text);
    if (at == std::wstring::npos)
        return L"";
    // "<w:p>" and "<w:p " but not "<w:pPr>".
    size_t begin = document.rfind(para + L">", at);
    const size_t begin_attrs = document.rfind(para + L" ", at);
    if (begin == std::wstring::npos || (begin_attrs != std::wstring::npos && begin_attrs > begin))
        begin = begin_attrs;
    const size_t end = document.find(L"</" + p + L":p>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";

    std::wstring result;
    for (size_t t = document.find(run_text, begin); t != std::wstring::npos && t < end;
         t = document.find(run_text, t + 1))
    {
        const wchar_t next = document[t + 4];
        if (next != L'>' && next != L' ')
            continue; // <w:tab/>, <w:tc>, <a:tab/>, <a:txBody>, ...
        const size_t open_end = document.find(L'>', t);
        const size_t close = document.find(end_text, open_end);
        if (open_end == std::wstring::npos || close == std::wstring::npos || close > end)
            break;
        result += document.substr(open_end + 1, close - open_end - 1);
    }
    return result;
}

// OOXML -> ODF -> OOXML; returns the part of the second OOXML package.
class OoxmlOdfRoundTrip : public ::testing::Test
{
protected:
    void round_trip(const std::wstring& src, const std::wstring& odfName, const std::wstring& part)
    {
        const std::wstring odfDir = convert_ooxml_to_odf_dir(src);
        ASSERT_FALSE(odfDir.empty()) << "OOXML -> ODF conversion failed";

        m_sTempDir = NSDirectory::CreateDirectoryWithUniqueName(L"ExampleFiles");
        ASSERT_FALSE(m_sTempDir.empty());
        const std::wstring odf = m_sTempDir + FILE_SEPARATOR_STR + odfName;
        COfficeUtils oCOfficeUtils(NULL);
        ASSERT_EQ(S_OK, oCOfficeUtils.CompressFileOrDirectory(odfDir, odf, true));

        const std::wstring ooxmlDir = convert_odf_to_ooxml_dir(odf);
        ASSERT_FALSE(ooxmlDir.empty()) << "ODF -> OOXML conversion failed";
        m_sDocument = read_part(ooxmlDir, part);
    }
    void TearDown() override
    {
        if (!m_sTempDir.empty())
            NSDirectory::DeleteDirectory(m_sTempDir);
    }

    std::wstring m_sTempDir;
    std::wstring m_sDocument;
};

class DocxToOdtWhiteSpace : public OoxmlOdfRoundTrip
{
protected:
    void SetUp() override
    {
        round_trip(L"ExampleFiles/white_space_runs.docx", L"white_space_runs.odt", L"word/document.xml");
    }
};

// Hyperlink text of a presentation goes through its own writer path.
class PptxToOdpWhiteSpace : public OoxmlOdfRoundTrip
{
protected:
    void SetUp() override
    {
        round_trip(L"ExampleFiles/white_space_hyperlink.pptx", L"white_space_hyperlink.odp", L"ppt/slides/slide1.xml");
    }
};
} // namespace

TEST_F(DocxToOdtWhiteSpace, LeadingSpaceKept)
{
    EXPECT_EQ(paragraph_text(m_sDocument, L"lead<"), L" lead");
}

TEST_F(DocxToOdtWhiteSpace, SpacesAtRunBoundaryKept)
{
    EXPECT_EQ(paragraph_text(m_sDocument, L"beta"), L"alpha  beta");
}

TEST_F(DocxToOdtWhiteSpace, OtherSpacesUnchanged)
{
    EXPECT_EQ(paragraph_text(m_sDocument, L"endrun"), L"endrun Xray");
    EXPECT_EQ(paragraph_text(m_sDocument, L"spaces"), L"two  spaces");
    EXPECT_EQ(paragraph_text(m_sDocument, L"three<"), L"   three");
    EXPECT_EQ(paragraph_text(m_sDocument, L"ypsilon"), L"xi ypsilon");
    EXPECT_EQ(paragraph_text(m_sDocument, L"tail <"), L"tail ");
}

TEST_F(PptxToOdpWhiteSpace, SpaceAfterLinkKept)
{
    EXPECT_EQ(paragraph_text(m_sDocument, L"more</a:t>", L"a"), L"See link  more");
}

TEST_F(PptxToOdpWhiteSpace, LeadingSpaceOfLinkKept)
{
    EXPECT_EQ(paragraph_text(m_sDocument, L"there</a:t>", L"a"), L"go  there");
}

TEST_F(PptxToOdpWhiteSpace, OtherLinkSpacesUnchanged)
{
    EXPECT_EQ(paragraph_text(m_sDocument, L"end</a:t>", L"a"), L"plain ref end");
}
