// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX of a text document embedded in a text document (draw:object of an ODF text
// object, with a replacement image): it is shown as its replacement image and embedded as a Word
// document, like an embedded spreadsheet. Before, the frame was dropped and nothing was drawn.

#include "gtest/gtest.h"

#include "common.h"

#include "DesktopEditor/common/Directory.h"
#include "DesktopEditor/common/File.h"
#include "OfficeUtils/src/OfficeUtils.h"
#include "Reader/Converter/mediaitems.h"

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

size_t occurrences(const std::wstring& xml, const std::wstring& what)
{
    size_t n = 0;
    for (size_t pos = xml.find(what); pos != std::wstring::npos; pos = xml.find(what, pos + 1))
        ++n;
    return n;
}

// An embedded package of a conversion (a .docx in word/embeddings), unpacked next to it, or ""
// if it is missing.
std::wstring unpack(const std::wstring& file)
{
    if (!NSFile::CFileBinary::Exists(file))
        return L"";
    const std::wstring dir = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetFolderPath(file));
    COfficeUtils utils(NULL);
    return !dir.empty() && S_OK == utils.ExtractToDirectory(file, dir, NULL, 0) ? dir : std::wstring();
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

// Objects that name their own document (".", "./", "Object 1/..") are not opened, also from inside
// an object ("../Object 1" in Object 1): only the real object is embedded. Before, each such object
// converted the whole document again, objects included.
TEST(OdtToDocxEmbeddedTextObject, ObjectsNamingTheirOwnDocumentAreNotConverted)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/embedded_text_object_self_many.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    EXPECT_TRUE(is_well_formed(document));
    EXPECT_NE(document.find(L"Before the object."), std::wstring::npos);

    size_t embedded = 0;
    for (size_t pos = document.find(L"Word.Document.12"); pos != std::wstring::npos;
         pos = document.find(L"Word.Document.12", pos + 1))
        ++embedded;
    EXPECT_EQ(embedded, 1u) << "only the real object is embedded";
    EXPECT_FALSE(read_part(dir, L"word/embeddings/msObject1.docx").empty()) << "the real object is missing";
    EXPECT_TRUE(read_part(dir, L"word/embeddings/msObject2.docx").empty());
}

// An href counts as inside the package folder only when it resolves to a path below that folder.
// From inside "Object 1", the sibling "../Object 10" also starts with ".../Object 1", and ".." is
// the folder above: neither is inside "Object 1".
TEST(OdtToDocxEmbeddedTextObject, SiblingFolderIsNotInsideTheObject)
{
    const std::wstring root = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());
    ASSERT_FALSE(root.empty());
    const std::wstring object1 = root + FILE_SEPARATOR_STR + L"Object 1";
    ASSERT_TRUE(NSDirectory::CreateDirectory(object1));
    ASSERT_TRUE(NSDirectory::CreateDirectory(object1 + FILE_SEPARATOR_STR + L"Pictures"));
    ASSERT_TRUE(NSDirectory::CreateDirectory(root + FILE_SEPARATOR_STR + L"Object 10"));

    cpdoccore::oox::mediaitems items(object1);
    EXPECT_TRUE(items.is_internal_path(L"Pictures", object1));
    EXPECT_TRUE(items.is_internal_path(L"./Pictures/", object1));
    EXPECT_FALSE(items.is_internal_path(L"../Object 10", object1));
    EXPECT_FALSE(items.is_internal_path(L"..", object1));
    EXPECT_FALSE(items.is_internal_path(L"Pictures/../..", object1));
    EXPECT_FALSE(items.is_internal_path(L"Missing", object1));

    NSDirectory::DeleteDirectory(root);
}

// K objects name the same subfolder on three levels: the document has K naming "Object 1", which
// has K naming "Object 2", which has K naming "Object 3". Each document is converted once and all
// objects naming it use that file; the third level is not opened and shows only its replacement
// image. Before, every object on the first two levels was converted again (K + K * K conversions).
TEST(OdtToDocxEmbeddedTextObject, RepeatedObjectsAreConvertedOnce)
{
    const size_t K = 20;
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/embedded_text_object_repeated.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    EXPECT_TRUE(is_well_formed(document));
    EXPECT_EQ(occurrences(document, L"Word.Document.12"), K);
    EXPECT_TRUE(read_part(dir, L"word/embeddings/msObject2.docx").empty()) << "the objects share one document";

    const std::wstring first = unpack(dir + FILE_SEPARATOR_STR + L"word/embeddings/msObject1.docx");
    ASSERT_FALSE(first.empty()) << "the first object is missing";
    const std::wstring firstDocument = read_part(first, L"word/document.xml");
    EXPECT_NE(firstDocument.find(L"Text in the first object."), std::wstring::npos);
    EXPECT_EQ(occurrences(firstDocument, L"Word.Document.12"), K) << "every object inside it is embedded";

    const std::wstring second = unpack(first + FILE_SEPARATOR_STR + L"word/embeddings/msObject1.docx");
    ASSERT_FALSE(second.empty()) << "the second object is missing";
    const std::wstring secondDocument = read_part(second, L"word/document.xml");
    EXPECT_NE(secondDocument.find(L"Text in the second object."), std::wstring::npos);
    EXPECT_EQ(occurrences(secondDocument, L"Word.Document.12"), 0u) << "the third level shows only images";
}

// One object holds 40 objects naming 40 different documents. Objects inside embedded documents are
// converted while the budget of 32 conversions per conversion lasts; the others show only their
// replacement image. Before, there was no limit.
TEST(OdtToDocxEmbeddedTextObject, ObjectsInsideObjectsStopAtTheBudget)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/embedded_text_object_nested_many.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
    EXPECT_EQ(occurrences(read_part(dir, L"word/document.xml"), L"Word.Document.12"), 1u);

    const std::wstring first = unpack(dir + FILE_SEPARATOR_STR + L"word/embeddings/msObject1.docx");
    ASSERT_FALSE(first.empty()) << "the object is missing";
    const std::wstring document = read_part(first, L"word/document.xml");
    EXPECT_TRUE(is_well_formed(document));
    EXPECT_NE(document.find(L"Text in the first object."), std::wstring::npos);
    EXPECT_EQ(occurrences(document, L"Word.Document.12"), 32u);
}
