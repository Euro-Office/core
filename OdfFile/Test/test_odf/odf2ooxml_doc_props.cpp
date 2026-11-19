// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX, ODP -> PPTX and ODT -> DOCX document properties from meta.xml
// (docProps/core.xml and docProps/custom.xml).

#include "gtest/gtest.h"

#include "common.h"

#include "OdfFile/Reader/Format/styles_lite_container.h"

#include <map>
#include <string>

namespace
{
	// Output folder of a converted fixture, converted once per fixture.
	std::wstring converted_dir(const std::wstring& fixture)
	{
		static std::map<std::wstring, std::wstring> dirs;
		std::map<std::wstring, std::wstring>::iterator it = dirs.find(fixture);
		if (it == dirs.end())
			it = dirs.insert(std::make_pair(fixture, convert_odf_to_ooxml_dir(fixture))).first;
		return it->second;
	}

	bool contains(const std::wstring& xml, const std::wstring& text)
	{
		return xml.find(text) != std::wstring::npos;
	}

	size_t count(const std::wstring& xml, const std::wstring& text)
	{
		size_t n = 0;
		for (size_t pos = xml.find(text); pos != std::wstring::npos; pos = xml.find(text, pos + text.size()))
			++n;
		return n;
	}

	// The fixtures' meta.xml has a title with an ampersand, a subject, a description, a creation
	// date, the user-defined property Project = Apollo, two keywords, meta:initial-creator
	// Ada Author and dc:creator Bob Editor.
	void expect_core(const std::wstring& fixture)
	{
		const std::wstring dir = converted_dir(fixture);
		ASSERT_FALSE(dir.empty()) << "conversion failed";

		const std::wstring core = read_part(dir, L"docProps/core.xml");
		ASSERT_FALSE(core.empty()) << "docProps/core.xml is missing";
		EXPECT_TRUE(is_well_formed(core));
		EXPECT_TRUE(contains(core, L"<dc:title>Budget &amp; plan</dc:title>")) << core;
		EXPECT_TRUE(contains(core, L"<dc:subject>Quarterly figures</dc:subject>")) << core;
		EXPECT_TRUE(contains(core, L"<dc:description>Numbers for the review</dc:description>")) << core;
		// The fixture's dates have no time zone: core.xml marks them as the wall-clock time.
		EXPECT_TRUE(contains(core, L">2026-01-02T03:04:05Z</dcterms:created>")) << core;
		EXPECT_TRUE(contains(core, L">2026-01-03T04:05:06Z</dcterms:modified>")) << core;
		// OOXML dc:creator is the author who created the file, cp:lastModifiedBy the last one who
		// saved it; several ODF keywords become one cp:keywords string.
		EXPECT_TRUE(contains(core, L"<dc:creator>Ada Author</dc:creator>")) << core;
		EXPECT_TRUE(contains(core, L"<cp:lastModifiedBy>Bob Editor</cp:lastModifiedBy>")) << core;
		EXPECT_TRUE(contains(core, L"<cp:keywords>finance budget</cp:keywords>")) << core;

		const std::wstring rels = read_part(dir, L"_rels/.rels");
		EXPECT_EQ(1, count(rels, L"/relationships/metadata/core-properties\"")) << rels;
		EXPECT_TRUE(contains(rels,
			L"Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\""))
			<< rels;
	}

	void expect_custom(const std::wstring& fixture)
	{
		const std::wstring dir = converted_dir(fixture);
		ASSERT_FALSE(dir.empty()) << "conversion failed";

		const std::wstring custom = read_part(dir, L"docProps/custom.xml");
		ASSERT_FALSE(custom.empty()) << "docProps/custom.xml is missing";
		EXPECT_TRUE(is_well_formed(custom));
		EXPECT_TRUE(contains(custom, L"name=\"Project\"")) << custom;
		EXPECT_TRUE(contains(custom, L"<vt:lpwstr>Apollo</vt:lpwstr>")) << custom;

		EXPECT_TRUE(contains(read_part(dir, L"[Content_Types].xml"), L"PartName=\"/docProps/custom.xml\""));
		EXPECT_TRUE(contains(read_part(dir, L"_rels/.rels"), L"Target=\"docProps/custom.xml\""));
	}
}

TEST(OdsToXlsxDocProps, CoreProperties)
{
	expect_core(L"ExampleFiles/doc_props.ods");
}

TEST(OdsToXlsxDocProps, CustomProperties)
{
	expect_custom(L"ExampleFiles/doc_props.ods");
}

// Without meta.xml there is nothing to keep: no custom.xml, core.xml stays valid.
TEST(OdsToXlsxDocProps, NoMeta)
{
	const std::wstring dir = converted_dir(L"ExampleFiles/line_height_normal.ods");
	ASSERT_FALSE(dir.empty()) << "conversion failed";
	EXPECT_TRUE(is_well_formed(read_part(dir, L"docProps/core.xml")));
	EXPECT_TRUE(read_part(dir, L"docProps/custom.xml").empty());
	EXPECT_FALSE(contains(read_part(dir, L"_rels/.rels"), L"custom-properties"));
	// Nobody is named as the last author: not even the application.
	EXPECT_FALSE(contains(read_part(dir, L"docProps/core.xml"), L"cp:lastModifiedBy"));
}

// A file with dc:creator but no meta:initial-creator: the name is the last author, and the author too,
// so that it survives the next save.
TEST(OdsToXlsxDocProps, LastAuthorOnly)
{
	const std::wstring dir = converted_dir(L"ExampleFiles/doc_props_editor_only.ods");
	ASSERT_FALSE(dir.empty()) << "conversion failed";

	const std::wstring core = read_part(dir, L"docProps/core.xml");
	EXPECT_TRUE(is_well_formed(core)) << core;
	EXPECT_TRUE(contains(core, L"<cp:lastModifiedBy>Bob Editor</cp:lastModifiedBy>")) << core;
	EXPECT_TRUE(contains(core, L"<dc:creator>Bob Editor</dc:creator>")) << core;
}

// Authors and custom properties come from the file: markup in them must stay text. Dates that are
// no dates are left out.
TEST(OdsToXlsxDocProps, MarkupInValuesIsEscaped)
{
	const std::wstring dir = converted_dir(L"ExampleFiles/doc_props_markup.ods");
	ASSERT_FALSE(dir.empty()) << "conversion failed";

	const std::wstring core = read_part(dir, L"docProps/core.xml");
	EXPECT_TRUE(is_well_formed(core)) << core;
	EXPECT_FALSE(contains(core, L"dcterms:created")) << core;
	EXPECT_FALSE(contains(core, L"dcterms:modified")) << core;
	EXPECT_FALSE(contains(core, L"<dc:creator>x")) << core;
	EXPECT_TRUE(contains(core, L"<dc:creator>&lt;a&gt; &amp; b</dc:creator>")) << core;
	EXPECT_TRUE(contains(core, L"<cp:lastModifiedBy>&lt;/cp:lastModifiedBy&gt;x</cp:lastModifiedBy>")) << core;

	const std::wstring custom = read_part(dir, L"docProps/custom.xml");
	EXPECT_TRUE(is_well_formed(custom)) << custom;
	EXPECT_TRUE(contains(custom, L"name=\"A &amp; B\"")) << custom;
	EXPECT_TRUE(contains(custom, L"<vt:lpwstr>&lt;v&gt;</vt:lpwstr>")) << custom;
}

TEST(OdpToPptxDocProps, CoreProperties)
{
	expect_core(L"ExampleFiles/doc_props.odp");
}

TEST(OdpToPptxDocProps, CustomProperties)
{
	expect_custom(L"ExampleFiles/doc_props.odp");
}

TEST(OdtToDocxDocProps, CoreProperties)
{
	expect_core(L"ExampleFiles/doc_props.odt");
}

TEST(OdtToDocxDocProps, CustomProperties)
{
	expect_custom(L"ExampleFiles/doc_props.odt");
}

namespace
{
	// docProps/core.xml written for a meta:creation-date and a dc:date with the given value.
	std::wstring core_for_dates(const std::wstring& value)
	{
		cpdoccore::odf_reader::doc_props_container props;
		props.creation_date_ = value;
		props.dc_date_ = value;
		return props.dump_core();
	}
}

// ODF dates are xsd:dateTime, mostly without a time zone and with up to nine fraction digits; core.xml
// takes W3CDTF. A value without a zone is the wall-clock time, written with "Z" as LibreOffice does: an
// editor would read it as local time and save it shifted by its UTC offset.
TEST(OdfToOoxmlDocProps, DatesAreW3CDTF)
{
	const struct { const wchar_t* odf; const wchar_t* core; } cases[] = {
		{ L"2024-06-05T16:03:40", L"2024-06-05T16:03:40Z" },
		{ L"2024-06-05T16:03:40.809199394", L"2024-06-05T16:03:40Z" },
		{ L"2024-06-05T16:03:40,5", L"2024-06-05T16:03:40Z" },
		{ L" 2024-06-05T16:03:40\n", L"2024-06-05T16:03:40Z" },
		{ L"2024-06-05T16:03:40Z", L"2024-06-05T16:03:40Z" },
		{ L"2024-06-05T16:03:40.25+02:00", L"2024-06-05T16:03:40+02:00" },
		{ L"2024-06-05T16:03:40-05:30", L"2024-06-05T16:03:40-05:30" },
		{ L"2024-06-05", L"2024-06-05T00:00:00Z" },
		{ L"2024-02-29T12:00:00", L"2024-02-29T12:00:00Z" },
		{ L"2000-02-29T12:00:00+14:00", L"2000-02-29T12:00:00+14:00" },
	};
	for (const auto& c : cases)
	{
		const std::wstring core = core_for_dates(c.odf);
		EXPECT_TRUE(is_well_formed(core)) << core;
		EXPECT_TRUE(contains(core, std::wstring(L"\">") + c.core + L"</dcterms:created>")) << c.odf << L"\n" << core;
		EXPECT_TRUE(contains(core, std::wstring(L"\">") + c.core + L"</dcterms:modified>")) << c.odf << L"\n" << core;
	}
}

// Office reports a core.xml with a year before 1601 or after 9999 as corrupt; values that are no dates
// carry nothing to keep.
TEST(OdfToOoxmlDocProps, InvalidDatesAreLeftOut)
{
	const wchar_t* values[] = {
		L"1600-12-31T23:59:59", L"10000-01-01T00:00:00", L"-0001-01-01T00:00:00", L"0000-01-01",
		L"2024-13-01T00:00:00", L"2024-06-05T25:00:00", L"2024-06-05T16:03", L"2024-06-05T16:03:40.",
		L"2024-06-05T16:03:40+2:00", L"2024-06-05T16:03:40 CET", L"05.06.2024", L"1 & 2", L"",
		L"2024-02-30T00:00:00", L"2023-02-29", L"1900-02-29", L"2024-04-31", L"2024-06-05T16:03:40+14:30",
	};
	for (const wchar_t* value : values)
	{
		const std::wstring core = core_for_dates(value);
		EXPECT_TRUE(is_well_formed(core)) << core;
		EXPECT_FALSE(contains(core, L"dcterms:created")) << value << L"\n" << core;
		EXPECT_FALSE(contains(core, L"dcterms:modified")) << value << L"\n" << core;
	}
}
