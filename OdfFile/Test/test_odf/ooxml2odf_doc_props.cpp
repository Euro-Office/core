// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT, XLSX -> ODS and PPTX -> ODP document properties from docProps/core.xml
// (meta.xml).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	bool contains(const std::wstring& xml, const std::wstring& text)
	{
		return xml.find(text) != std::wstring::npos;
	}

	// The fixtures' core.xml: dc:creator Ada Author (who created the file), cp:lastModifiedBy
	// Bob Editor (who saved it last), created and modified dates, a title with an ampersand,
	// a subject, a description and the keywords "finance budget".
	void expect_meta(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		ASSERT_FALSE(dir.empty()) << "conversion failed";

		const std::wstring meta = read_part(dir, L"meta.xml");
		ASSERT_FALSE(meta.empty()) << "meta.xml is missing";
		EXPECT_TRUE(is_well_formed(meta)) << meta;
		EXPECT_TRUE(contains(meta, L"<meta:initial-creator>Ada Author</meta:initial-creator>")) << meta;
		EXPECT_TRUE(contains(meta, L"<dc:creator>Bob Editor</dc:creator>")) << meta;
		EXPECT_TRUE(contains(meta, L"<meta:creation-date>2026-01-02T03:04:05Z</meta:creation-date>")) << meta;
		EXPECT_TRUE(contains(meta, L"<dc:date>2026-01-03T04:05:06Z</dc:date>")) << meta;
		EXPECT_TRUE(contains(meta, L"<dc:title>Budget &amp; plan</dc:title>")) << meta;
		EXPECT_TRUE(contains(meta, L"<dc:subject>Quarterly figures</dc:subject>")) << meta;
		EXPECT_TRUE(contains(meta, L"<dc:description>Numbers for the review</dc:description>")) << meta;
		// cp:keywords is split into one meta:keyword per keyword.
		EXPECT_TRUE(contains(meta, L"<meta:keyword>finance</meta:keyword>")) << meta;
		EXPECT_TRUE(contains(meta, L"<meta:keyword>budget</meta:keyword>")) << meta;
		EXPECT_FALSE(contains(meta, L"finance budget")) << meta;
	}
}

TEST(DocxToOdtDocProps, CoreProperties)
{
	expect_meta(L"ExampleFiles/doc_props_core.docx");
}

TEST(XlsxToOdsDocProps, CoreProperties)
{
	expect_meta(L"ExampleFiles/doc_props_core.xlsx");
}

TEST(PptxToOdpDocProps, CoreProperties)
{
	expect_meta(L"ExampleFiles/doc_props_core.pptx");
}

// The last author, the dates and the language come from the file: markup in them must stay text.
TEST(DocxToOdtDocProps, MarkupInValuesIsEscaped)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/doc_props_core_markup.docx");
	ASSERT_FALSE(dir.empty()) << "conversion failed";

	const std::wstring meta = read_part(dir, L"meta.xml");
	EXPECT_TRUE(is_well_formed(meta)) << meta;
	EXPECT_TRUE(contains(meta, L"<dc:creator>A &amp; B</dc:creator>")) << meta;
	EXPECT_TRUE(contains(meta, L"<dc:date>&lt;/dc:date&gt;&lt;dc:title&gt;x</dc:date>")) << meta;
	EXPECT_FALSE(contains(meta, L"<dc:title>x")) << meta;
	EXPECT_TRUE(contains(meta, L"<dc:language>&lt;l&gt;</dc:language>")) << meta;
	EXPECT_TRUE(contains(meta, L"<meta:creation-date>&lt;c&gt;</meta:creation-date>")) << meta;
	EXPECT_FALSE(contains(meta, L"<meta:initial-creator>")) << meta;
}
