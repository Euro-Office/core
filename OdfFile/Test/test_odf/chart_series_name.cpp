// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODF chart -> DrawingML chart part: the cached series names (c:tx) and
// category names (c:cat) are text taken from the chart's data table; titles,
// formulas and the pivot source name come from the chart and the sheet names.
// They have to be escaped, or a name with "&" or "<" makes the whole chart
// part unreadable.

#include "gtest/gtest.h"

#include "common.h"

#include "OdfFile/Reader/Converter/oox_chart_series.h"

#include <sstream>
#include <string>
#include <vector>

TEST(OdfToOoxmlChart, SeriesAndCategoryNameCachesAreEscaped)
{
	// Embedded bar chart whose first series is named "R&D <total>" and whose
	// first category is named "North & <East>" in the chart's local table.
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/chart_series_name_ampersand.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring xml = read_part(dir, L"word/charts/chart1.xml");
	ASSERT_FALSE(xml.empty()) << "word/charts/chart1.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"<c:v>R&amp;D &lt;total&gt;</c:v>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<c:v>North &amp; &lt;East&gt;</c:v>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<c:v>Costs</c:v>"), std::wstring::npos);
}

// In a spreadsheet, a chart that takes its data from its own local table
// loses the links to that table, so its categories are written as literal
// strings (c:cat/c:strLit) instead of a c:strRef cache.
TEST(OdfToOoxmlChart, LiteralCategoryNamesAreEscaped)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/chart_local_table_category_ampersand.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring xml = read_part(dir, L"xl/charts/chart1.xml");
	ASSERT_FALSE(xml.empty()) << "xl/charts/chart1.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"<c:strLit>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<c:v>North &amp; &lt;East&gt;</c:v>"), std::wstring::npos);
}

// A series name without a formula is written as c:tx/c:v. No small document
// leads there, so the series is filled in directly.
TEST(OdfToOoxmlChart, LiteralSeriesNameIsEscaped)
{
	cpdoccore::oox::oox_bar_series oSeries;
	oSeries.label_.present = true;
	oSeries.label_.str_cache.push_back(L"R&D <total>");

	std::wstringstream oStream;
	oSeries.oox_serialize(oStream);
	const std::wstring xml = oStream.str();

	EXPECT_TRUE(is_well_formed(xml)) << xml;
	EXPECT_NE(xml.find(L"<c:tx><c:v>R&amp;D &lt;total&gt;</c:v></c:tx>"), std::wstring::npos) << xml;
}

// A spreadsheet chart on the sheet "R&D" with the title "Sales & <costs>" and
// the category axis title "Region & area".
TEST(OdfToOoxmlChart, TitlesAreEscaped)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/chart_sheet_name_ampersand.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring xml = read_part(dir, L"xl/charts/chart1.xml");
	ASSERT_FALSE(xml.empty()) << "xl/charts/chart1.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"<a:t>Sales &amp; &lt;costs&gt;</a:t>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<a:t>Region &amp; area</a:t>"), std::wstring::npos);
	EXPECT_EQ(xml.find(L"&amp;amp;"), std::wstring::npos);
}

// The same chart: its ranges refer to the sheet "R&D", so the sheet name is
// part of every c:f formula.
TEST(OdfToOoxmlChart, FormulasWithSheetNameAreEscaped)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/chart_sheet_name_ampersand.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

	const std::wstring xml = read_part(dir, L"xl/charts/chart1.xml");
	ASSERT_FALSE(xml.empty()) << "xl/charts/chart1.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"<c:f>&apos;R&amp;D&apos;!$A$2:$A$3</c:f>"), std::wstring::npos); // c:strRef
	EXPECT_NE(xml.find(L"<c:f>&apos;R&amp;D&apos;!$B$2:$B$3</c:f>"), std::wstring::npos); // c:numRef
	EXPECT_EQ(xml.find(L"&amp;amp;"), std::wstring::npos);
}

// An embedded chart whose loext:data-pilot-source is "R&D <pivot>": the name
// is written to c:pivotSource/c:name.
TEST(OdfToOoxmlChart, PivotSourceNameIsEscaped)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/chart_pivot_source_ampersand.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring xml = read_part(dir, L"word/charts/chart1.xml");
	ASSERT_FALSE(xml.empty()) << "word/charts/chart1.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"<c:name>R&amp;D &lt;pivot&gt;</c:name>"), std::wstring::npos);
}
