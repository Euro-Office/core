// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of master pages without a header or footer. A DOCX
// section without w:headerReference (w:footerReference) shows the header
// (footer) of the section before it; an ODF master page without one shows none.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
// Value of an attribute of the element that starts at 'from'.
std::wstring attribute(const std::wstring& xml, size_t from, const std::wstring& name)
{
    const size_t end = xml.find(L'>', from);
    const size_t at  = xml.find(L" " + name + L"=\"", from);
    if (end == std::wstring::npos || at == std::wstring::npos || at > end)
        return L"";
    const size_t begin = at + name.size() + 3;
    return xml.substr(begin, xml.find(L'"', begin) - begin);
}

// The text of all w:t elements.
std::wstring text_of(const std::wstring& xml)
{
    std::wstring text;
    for (size_t at = xml.find(L"<w:t"); at != std::wstring::npos; at = xml.find(L"<w:t", at + 1))
    {
        if (xml[at + 4] != L'>' && xml[at + 4] != L' ')
            continue; // w:tab, w:tbl, ...
        const size_t begin = xml.find(L'>', at) + 1;
        text += xml.substr(begin, xml.find(L"</w:t>", begin) - begin);
    }
    return text;
}

// What every section of the document refers to, in document order: its header
// and footer references as "<header|footer>:<type>=<text of the part>" and
// "titlePg", separated by blanks. An empty part gives "header:default=".
std::vector<std::wstring> sections(const std::wstring& dir)
{
    const std::wstring document = read_part(dir, L"word/document.xml");
    const std::wstring rels     = read_part(dir, L"word/_rels/document.xml.rels");

    std::vector<std::wstring> result;
    for (size_t at = document.find(L"<w:sectPr"); at != std::wstring::npos;
         at = document.find(L"<w:sectPr", at + 1))
    {
        const size_t end = document.find(L"</w:sectPr>", at);
        const std::wstring sectPr = document.substr(at, end - at);

        std::wstring refs;
        for (size_t ref = sectPr.find(L"Reference "); ref != std::wstring::npos;
             ref = sectPr.find(L"Reference ", ref + 1))
        {
            const bool header = sectPr.compare(ref - 9, 9, L"<w:header") == 0;
            const size_t rel  = rels.find(L"Id=\"" + attribute(sectPr, ref, L"r:id") + L"\"");
            if (rel == std::wstring::npos)
            {
                ADD_FAILURE() << "reference without relationship in " << sectPr;
                continue;
            }
            const std::wstring target = attribute(rels, rels.rfind(L'<', rel), L"Target");
            const std::wstring part   = read_part(dir, L"word/" + target);
            EXPECT_TRUE(is_well_formed(part)) << target;
            EXPECT_NE(part.find(header ? L"<w:hdr " : L"<w:ftr "), std::wstring::npos) << target;

            refs += (header ? L"header:" : L"footer:") + attribute(sectPr, ref, L"w:type") + L"=";
            refs += text_of(part) + L" ";
        }
        if (sectPr.find(L"<w:titlePg") != std::wstring::npos)
            refs += L"titlePg ";
        if (!refs.empty())
            refs.erase(refs.size() - 1);
        result.push_back(refs);
    }
    return result;
}

class OdtToDocxHeaderFooterNotInherited : public ::testing::Test
{
protected:
    void SetUp() override
    {
        dir = convert_odf_to_ooxml_dir(L"ExampleFiles/header_footer_not_inherited.odt");
        ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
        all = sections(dir);
        // master pages A B C A D E F G H, one section each; S1 up to its page break; the rest of S1 with S2; B
        ASSERT_EQ(all.size(), 12u) << read_part(dir, L"word/document.xml");
    }

    std::wstring dir;
    std::vector<std::wstring> all;
};
}

// B has a header and no footer: it gets an empty footer in place of A's.
TEST_F(OdtToDocxHeaderFooterNotInherited, MissingFooterIsEmpty)
{
    EXPECT_EQ(all[0], L"header:default=A-HEADER footer:default=A-FOOTER");
    EXPECT_EQ(all[1], L"header:default=B-HEADER footer:default=");
}

// C has neither. The footer is already empty since B, only the header needs one.
TEST_F(OdtToDocxHeaderFooterNotInherited, MissingHeaderIsEmptyOnce)
{
    EXPECT_EQ(all[2], L"header:default=");
    EXPECT_EQ(all[3], L"header:default=A-HEADER footer:default=A-FOOTER");
}

// D hands over to A after its first page, and its section also holds the pages
// of A: it is left as it is. E, which has nothing, then hides both.
TEST_F(OdtToDocxHeaderFooterNotInherited, MasterPageWithAnotherNextStyleIsLeftAlone)
{
    EXPECT_EQ(all[4], L"header:default=D-HEADER");
    EXPECT_EQ(all[5], L"header:default= footer:default=");
}

// G has footers for the first and the other pages and no header: both headers
// of F are hidden. H has no first page of its own, so only the footer of the
// other pages is.
TEST_F(OdtToDocxHeaderFooterNotInherited, FirstPageVariants)
{
    EXPECT_EQ(all[6], L"header:default=F-HEADER footer:default=F-FOOTER header:first=F-HEADER-FIRST "
                      L"footer:first=F-FOOTER-FIRST titlePg");
    EXPECT_EQ(all[7], L"footer:default=G-FOOTER footer:first=G-FOOTER-FIRST header:default= header:first= titlePg");
    EXPECT_EQ(all[8], L"footer:default=");
}

// S1 and S2 share a page layout, so no section starts between them: the section
// that ends with the page of S2 begins with a page of S1 and keeps its header.
TEST_F(OdtToDocxHeaderFooterNotInherited, MasterPagesOnOnePageLayoutAreLeftAlone)
{
    EXPECT_EQ(all[9], L"header:default=S-HEADER");
    EXPECT_EQ(all[10], L"");
}

// B is used a second time and refers to the empty footer it already has.
TEST_F(OdtToDocxHeaderFooterNotInherited, StyleUsedAgainKeepsItsEmptyFooter)
{
    EXPECT_EQ(all[11], L"header:default=B-HEADER footer:default=");
}

// The empty header or footer takes next to no room on the page: its only
// paragraph is as low as a paragraph can be.
TEST_F(OdtToDocxHeaderFooterNotInherited, EmptyPartTakesNoRoom)
{
    size_t empty = 0;
    for (int i = 1; i <= 40; ++i)
    {
        for (const wchar_t* kind : { L"word/header", L"word/footer" })
        {
            const std::wstring part = read_part(dir, kind + std::to_wstring(i) + L".xml");
            if (part.empty() || !text_of(part).empty())
                continue;
            ++empty;
            EXPECT_NE(part.find(L"w:line=\"1\" w:lineRule=\"exact\""), std::wstring::npos) << part;
        }
    }
    // B footer, C header, E header and footer, G header and first-page header, H footer
    EXPECT_EQ(empty, 7u);
}
