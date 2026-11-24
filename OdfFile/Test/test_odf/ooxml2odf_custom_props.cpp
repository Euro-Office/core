// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT custom document properties (docProps/custom.xml -> meta:user-defined in meta.xml).
// XLSX -> ODS and PPTX -> ODP go through the same converter code.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	std::wstring user_defined(const std::wstring& name, const std::wstring& type, const std::wstring& value)
	{
		return L"<meta:user-defined meta:name=\"" + name + L"\" meta:value-type=\"" + type + L"\">" + value
			+ L"</meta:user-defined>";
	}
}

// Every value keeps its text and gets the ODF value type that matches its OOXML variant type.
TEST(DocxToOdtCustomProps, TypedValuesAreKept)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/custom_props_typed.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring meta = read_part(dir, L"meta.xml");
	ASSERT_FALSE(meta.empty()) << "meta.xml is missing";
	EXPECT_TRUE(is_well_formed(meta)) << meta;

	EXPECT_NE(meta.find(user_defined(L"Text", L"string", L"hello")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Count", L"float", L"42")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Price", L"float", L"3.25")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Tenth", L"float", L"0.1")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Rate", L"float", L"1.5")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Approved", L"boolean", L"true")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Archived", L"boolean", L"false")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Due", L"date", L"2026-11-19T10:30:00Z")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Loss", L"float", L"-2.5")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Huge", L"float", L"1e+20")), std::wstring::npos) << meta;
	// Whole numbers and numbers of ordinary size are written without an exponent, as LibreOffice does.
	EXPECT_NE(meta.find(user_defined(L"Year", L"float", L"2000")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Visitors", L"float", L"45600")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Tiny", L"float", L"0.0000001")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Quadrillion", L"float", L"1000000000000000")), std::wstring::npos) << meta;
	// A file time that is no date stays text.
	EXPECT_NE(meta.find(user_defined(L"Later", L"string", L"soon")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"Never", L"string", L"2026-13-45T00:00:00Z")), std::wstring::npos) << meta;
	// A day the month does not have, or text after the date, is no date either.
	EXPECT_NE(meta.find(user_defined(L"Feb31", L"string", L"2026-02-31T00:00:00Z")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"JunkTime", L"string", L"2026-01-05Tjunk")), std::wstring::npos) << meta;
	EXPECT_NE(meta.find(user_defined(L"LeapDay", L"date", L"2024-02-29T08:00:00Z")), std::wstring::npos) << meta;
}

// The property name is an attribute value: markup in it must stay text, or meta.xml is no XML.
TEST(DocxToOdtCustomProps, MarkupInNameIsEscaped)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/custom_props_typed.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring meta = read_part(dir, L"meta.xml");
	ASSERT_FALSE(meta.empty()) << "meta.xml is missing";
	EXPECT_TRUE(is_well_formed(meta)) << meta;
	EXPECT_NE(meta.find(user_defined(L"R&amp;D &lt;x&gt; &quot;y&quot;", L"string", L"team")), std::wstring::npos) << meta;
}
