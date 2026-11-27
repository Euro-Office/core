// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of the filter conditions of a database range: one filterColumn per
// filtered field, the OOXML operator for each ODF operator, value lists, top/bottom N and
// colour filters.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, every sheet A1:G5 with a header row:
	// 1 Values: field 1 "=" East with the value list East, North and an empty value (filter-or)
	// 2 AndNum: filter-and of field 2 >= 10, field 2 < 50 and field 0 != x
	// 3 Text:   filter-or of field 0 contains "app" and field 0 does not begin with "x"
	// 4 Single: field 1 != East, no filter-and/filter-or
	// 5 Top:    field 2 bottom 10 percent
	// 6 Offset: B1:C5 with filter buttons only and E1:E5 with field 0 < 5; both share the sheet's autoFilter
	// 7 Color:  field 0 filtered by the text colour #ff0000
	// 8 OrFields:    filter-or of field 0 = apple and field 1 = East
	// 9 Mixed:       filter-or of (field 0 = apple and field 2 > 10) and field 0 = pear
	// 10 BadField:   filter-and of field -1 = a, field 5 = x (the range has 3 columns) and field 1 != East
	// 11 ThreeCustom: filter-and of field 2 > 1, < 50, != 12 and field 0 = apple
	// 12 TopBad:     filter-and of field 2 top "abc" values and field 1 top 900 values
	// 13 Rows:       A1:A3 with filter buttons only and B3:B5 with field 0 = West; both share the sheet's autoFilter
	// 14 Orient:     field 1 = East on a range with table:orientation="column"
	// 15 Escape:     filter-and of field 0 in the value list "a & b", "x<y" and field 1 contains "R&D"
	// 16 TextMore:   filter-and of field 0 begins with "a", does not end with "e", field 1 ends with "st", does not contain "x"
	// Sheets 1-5, 7, 8-12 and 14-16 become table parts (xl/tables/table1-14.xml), sheets 6 and 13 a sheet autoFilter.
	const std::wstring& xlsx_dir()
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_autofilter.ods");
		return dir;
	}

	// The <autoFilter> element of an XLSX part, "(none)" if there is none.
	std::wstring auto_filter(const std::wstring& part)
	{
		if (xlsx_dir().empty())
			return L"(conversion failed)";
		const std::wstring xml = read_part(xlsx_dir(), part);
		const size_t begin = xml.find(L"<autoFilter");
		if (begin == std::wstring::npos)
			return L"(none)";
		const size_t end = xml.find(L"</autoFilter>", begin);
		if (end == std::wstring::npos)
			return xml.substr(begin, xml.find(L">", begin) + 1 - begin);
		return xml.substr(begin, end + 13 - begin);
	}
}

TEST(OdsToXlsx, AutoFilterValueList)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table1.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><filters blank=\"1\"><filter val=\"East\"/>"
		L"<filter val=\"North\"/></filters></filterColumn></autoFilter>");
}

// Each field gets its own column; "!=" stays "not equal"; AND of two conditions on one field.
TEST(OdsToXlsx, AutoFilterAndOfFields)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table2.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><customFilters><customFilter operator=\"notEqual\" val=\"x\"/>"
		L"</customFilters></filterColumn><filterColumn colId=\"2\"><customFilters and=\"1\">"
		L"<customFilter operator=\"greaterThanOrEqual\" val=\"10\"/><customFilter operator=\"lessThan\" val=\"50\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}

TEST(OdsToXlsx, AutoFilterTextOperators)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table3.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><customFilters><customFilter operator=\"equal\" val=\"*app*\"/>"
		L"<customFilter operator=\"notEqual\" val=\"x*\"/></customFilters></filterColumn></autoFilter>");
}

TEST(OdsToXlsx, AutoFilterSingleCondition)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table4.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><customFilters><customFilter operator=\"notEqual\" val=\"East\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}

TEST(OdsToXlsx, AutoFilterBottomPercent)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table5.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"2\"><top10 top=\"0\" percent=\"1\" val=\"10\"/></filterColumn>"
		L"</autoFilter>");
}

// The field number counts from the database range (E), colId from the sheet's autoFilter range (B).
TEST(OdsToXlsx, AutoFilterColumnOffset)
{
	const std::wstring sheet = read_part(xlsx_dir(), L"xl/worksheets/sheet6.xml");
	EXPECT_TRUE(is_well_formed(sheet));
	EXPECT_EQ(auto_filter(L"xl/worksheets/sheet6.xml"),
		L"<autoFilter ref=\"B1:E5\"><filterColumn colId=\"3\"><customFilters><customFilter operator=\"lessThan\" val=\"5\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}

TEST(OdsToXlsx, AutoFilterTextColor)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table6.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><colorFilter dxfId=\"0\" cellColor=\"0\"/></filterColumn>"
		L"</autoFilter>");
}
// filterColumns are ANDed: an OR across columns, also one with an AND inside it, has no OOXML form, so no condition is
// written (LibreOffice does not export such a filter either).
TEST(OdsToXlsx, AutoFilterOrAcrossFields)
{
	const std::wstring filter = auto_filter(L"xl/tables/table7.xml");
	EXPECT_EQ(filter.find(L"<autoFilter ref=\"A1:C5\""), 0u) << filter;
	EXPECT_EQ(filter.find(L"filterColumn"), std::wstring::npos) << filter;
}
TEST(OdsToXlsx, AutoFilterMixedAndOr)
{
	const std::wstring filter = auto_filter(L"xl/tables/table8.xml");
	EXPECT_EQ(filter.find(L"<autoFilter ref=\"A1:C5\""), 0u) << filter;
	EXPECT_EQ(filter.find(L"filterColumn"), std::wstring::npos) << filter;
}
// Field numbers outside the range are skipped instead of becoming colId -1 or 5.
TEST(OdsToXlsx, AutoFilterFieldOutsideRange)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table9.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><customFilters><customFilter operator=\"notEqual\" val=\"East\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}
// customFilters holds two conditions: a field with three is not filtered rather than filtered differently.
TEST(OdsToXlsx, AutoFilterThreeConditions)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table10.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><filters><filter val=\"apple\"/></filters></filterColumn>"
		L"</autoFilter>");
}
// The N of top N must be a number; it is limited to 500.
TEST(OdsToXlsx, AutoFilterTopValuesNumber)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table11.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><top10 val=\"500\"/></filterColumn></autoFilter>");
}
// The sheet's autoFilter covers rows 1-5, the filtered range only rows 3-5: its condition would hide other rows.
TEST(OdsToXlsx, AutoFilterOtherRows)
{
	EXPECT_EQ(auto_filter(L"xl/worksheets/sheet13.xml"), L"<autoFilter ref=\"A1:B5\"/>");
}
// LibreOffice writes table:orientation="column" from the range's sort settings; the filter still filters rows.
TEST(OdsToXlsx, AutoFilterColumnOrientation)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table12.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><filters><filter val=\"East\"/></filters></filterColumn>"
		L"</autoFilter>");
}
// The XML writer of this converter leaves '&' alone, so filter values must be encoded before they are written.
TEST(OdsToXlsx, AutoFilterValuesEscaped)
{
	const std::wstring table = read_part(xlsx_dir(), L"xl/tables/table13.xml");
	EXPECT_TRUE(is_well_formed(table));
	EXPECT_EQ(auto_filter(L"xl/tables/table13.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><filters><filter val=\"a &amp; b\"/><filter val=\"x&lt;y\"/>"
		L"</filters></filterColumn><filterColumn colId=\"1\"><customFilters><customFilter operator=\"equal\" val=\"*R&amp;D*\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}

// The ODF names of the text operators (begins-with, does-not-end-with, ends-with, does-not-contain).
TEST(OdsToXlsx, AutoFilterTextOperatorNames)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table14.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><customFilters and=\"1\"><customFilter operator=\"equal\" val=\"a*\"/>"
		L"<customFilter operator=\"notEqual\" val=\"*e\"/></customFilters></filterColumn><filterColumn colId=\"1\">"
		L"<customFilters and=\"1\"><customFilter operator=\"equal\" val=\"*st\"/><customFilter operator=\"notEqual\" val=\"*x*\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}
