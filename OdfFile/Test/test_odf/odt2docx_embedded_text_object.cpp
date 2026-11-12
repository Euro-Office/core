// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX of a text document embedded in a text document (draw:object of an ODF text
// object, with a replacement image): it is shown as its replacement image and embedded as a Word
// document, like an embedded spreadsheet. Before, the frame was dropped and nothing was drawn.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
std::wstring part(const std::wstring& name)
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/embedded_text_object.odt");
    return dir.empty() ? std::wstring() : read_part(dir, name);
}

// The values of every r:id="..." in xml.
std::vector<std::wstring> relationship_ids(const std::wstring& xml)
{
    std::vector<std::wstring> ids;
    const std::wstring key = L"r:id=\"";
    for (size_t pos = xml.find(key); pos != std::wstring::npos; pos = xml.find(key, pos + 1))
    {
        const size_t begin = pos + key.size();
        ids.push_back(xml.substr(begin, xml.find(L"\"", begin) - begin));
    }
    return ids;
}
} // namespace

TEST(OdtToDocxEmbeddedTextObject, EmbeddedAsWordDocument)
{
    const std::wstring document = part(L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_TRUE(is_well_formed(document));
    EXPECT_NE(document.find(L"Word.Document.12"), std::wstring::npos) << document;
}

// The replacement image and the embedded document are in the package, and every relationship
// the document uses resolves.
TEST(OdtToDocxEmbeddedTextObject, ReplacementImageAndDocumentAreInThePackage)
{
    const std::wstring document = part(L"word/document.xml");
    const std::wstring rels = part(L"word/_rels/document.xml.rels");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_NE(rels.find(L"media/"), std::wstring::npos) << rels;
    EXPECT_NE(rels.find(L"embeddings/msObject1.docx"), std::wstring::npos) << rels;
    EXPECT_FALSE(part(L"word/embeddings/msObject1.docx").empty()) << "the embedded document is missing";
    EXPECT_NE(part(L"[Content_Types].xml").find(L"Extension=\"docx\""), std::wstring::npos);

    const std::vector<std::wstring> ids = relationship_ids(document);
    EXPECT_GE(ids.size(), 2u) << document;
    for (const std::wstring& id : ids)
        EXPECT_NE(rels.find(L"Id=\"" + id + L"\""), std::wstring::npos) << id << " has no relationship";
}

TEST(OdtToDocxEmbeddedTextObject, SurroundingTextStays)
{
    const std::wstring document = part(L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_NE(document.find(L"Before the object."), std::wstring::npos);
}

// An object that names its own document (href ".") must not convert itself forever.
TEST(OdtToDocxEmbeddedTextObject, ObjectNamingItsOwnDocumentStops)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/embedded_text_object_self.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring document = read_part(dir, L"word/document.xml");
    EXPECT_TRUE(is_well_formed(document));
    EXPECT_NE(document.find(L"Before the object."), std::wstring::npos);
}
