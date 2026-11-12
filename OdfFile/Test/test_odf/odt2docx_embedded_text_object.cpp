// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX of a text document embedded in a text document (draw:object of an ODF text
// object, with a replacement image): it is shown as its replacement image and embedded as a Word
// document, like an embedded spreadsheet. Before, the frame was dropped and nothing was drawn.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
std::wstring part(const std::wstring& name)
{
	static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/embedded_text_object.odt");
	return dir.empty() ? std::wstring() : read_part(dir, name);
}
} // namespace

TEST(OdtToDocxEmbeddedTextObject, EmbeddedAsWordDocument)
{
	const std::wstring document = part(L"word/document.xml");
	ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
	EXPECT_TRUE(is_well_formed(document));
	EXPECT_NE(document.find(L"Word.Document.12"), std::wstring::npos) << document;
}

TEST(OdtToDocxEmbeddedTextObject, ReplacementImageAndDocumentAreLinked)
{
	const std::wstring rels = part(L"word/_rels/document.xml.rels");
	ASSERT_FALSE(rels.empty()) << "ODT -> DOCX conversion failed";
	EXPECT_NE(rels.find(L"media/"), std::wstring::npos) << rels;
	EXPECT_NE(rels.find(L"embeddings/"), std::wstring::npos) << rels;
}

TEST(OdtToDocxEmbeddedTextObject, SurroundingTextStays)
{
	const std::wstring document = part(L"word/document.xml");
	ASSERT_FALSE(document.empty()) << "ODT -> DOCX conversion failed";
	EXPECT_NE(document.find(L"Before the object."), std::wstring::npos);
}
