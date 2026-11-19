// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT -> DOCX round trip of single spaces that an ODF reader drops
// when they go out as character data: a space at the start of a paragraph,
// and a space that follows another space at a run boundary. The ODF writer
// must write them as text:s.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

#include "DesktopEditor/common/Directory.h"
#include "OfficeUtils/src/OfficeUtils.h"

namespace
{
// The text of the w:p element of word/document.xml that contains the given
// text: all w:t contents of that paragraph, in order.
std::wstring paragraph_text(const std::wstring& document, const std::wstring& text)
{
    const size_t at = document.find(text);
    if (at == std::wstring::npos)
        return L"";
    // "<w:p>" and "<w:p " but not "<w:pPr>".
    size_t begin = document.rfind(L"<w:p>", at);
    const size_t begin_attrs = document.rfind(L"<w:p ", at);
    if (begin == std::wstring::npos || (begin_attrs != std::wstring::npos && begin_attrs > begin))
        begin = begin_attrs;
    const size_t end = document.find(L"</w:p>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";

    std::wstring result;
    for (size_t t = document.find(L"<w:t", begin); t != std::wstring::npos && t < end;
         t = document.find(L"<w:t", t + 1))
    {
        const wchar_t next = document[t + 4];
        if (next != L'>' && next != L' ')
            continue; // <w:tab/>, <w:tc>, ...
        const size_t open_end = document.find(L'>', t);
        const size_t close = document.find(L"</w:t>", open_end);
        if (open_end == std::wstring::npos || close == std::wstring::npos || close > end)
            break;
        result += document.substr(open_end + 1, close - open_end - 1);
    }
    return result;
}

class DocxToOdtWhiteSpace : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const std::wstring odtDir = convert_ooxml_to_odf_dir(L"ExampleFiles/white_space_runs.docx");
        ASSERT_FALSE(odtDir.empty()) << "DOCX -> ODT conversion failed";

        m_sTempDir = NSDirectory::CreateDirectoryWithUniqueName(L"ExampleFiles");
        ASSERT_FALSE(m_sTempDir.empty());
        const std::wstring odt = m_sTempDir + FILE_SEPARATOR_STR + L"white_space_runs.odt";
        COfficeUtils oCOfficeUtils(NULL);
        ASSERT_EQ(S_OK, oCOfficeUtils.CompressFileOrDirectory(odtDir, odt, true));

        const std::wstring docxDir = convert_odf_to_ooxml_dir(odt);
        ASSERT_FALSE(docxDir.empty()) << "ODT -> DOCX conversion failed";
        m_sDocument = read_part(docxDir, L"word/document.xml");
    }
    void TearDown() override
    {
        if (!m_sTempDir.empty())
            NSDirectory::DeleteDirectory(m_sTempDir);
    }

    std::wstring m_sTempDir;
    std::wstring m_sDocument;
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
