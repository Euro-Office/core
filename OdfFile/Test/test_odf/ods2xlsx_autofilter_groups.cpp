// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of filters that nest table:filter-or inside table:filter-and. XLSX ANDs
// its filterColumns, so an OR of conditions on one field fits into that field's filterColumn.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, every sheet A1:C5 (Product, Region, Units) with a header row
	// and a named database range with filter buttons, so each becomes a table part:
	// 1 AndOr:          filter-and of field 0 = x and filter-or (field 2 > 10, field 2 < 0)
	// 2 AndOrValues:    filter-and of field 0 = apple and filter-or (field 1 = East, field 1 = North)
	// 3 OrTwoFields:    filter-and of field 0 = x and filter-or (field 1 = East, field 2 > 10)
	// 4 AndOrSameField: filter-and of field 2 > 0, filter-or (field 2 > 10, field 2 < 5) and field 0 = apple
	// 5 OrUnmapped:     filter-or of field 1 = East and field 1 matches the regular expression "N.*"
	// 6 OrColor:        filter-or of field 0 in text colour #ff0000 and field 0 = pear
	// 7 AndUnmapped:    filter-and of field 1 = East and field 1 matches "N.*"
	const std::wstring& xlsx_dir()
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_autofilter_groups.ods");
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

// "column A, and C above 10 or below 0": the OR stays inside column C's filterColumn.
TEST(OdsToXlsx, AutoFilterOrInsideAnd)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table1.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><filters><filter val=\"x\"/></filters></filterColumn>"
		L"<filterColumn colId=\"2\"><customFilters><customFilter operator=\"greaterThan\" val=\"10\"/>"
		L"<customFilter operator=\"lessThan\" val=\"0\"/></customFilters></filterColumn></autoFilter>");
}

TEST(OdsToXlsx, AutoFilterValueListInsideAnd)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table2.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><filters><filter val=\"apple\"/></filters></filterColumn>"
		L"<filterColumn colId=\"1\"><filters><filter val=\"East\"/><filter val=\"North\"/></filters></filterColumn>"
		L"</autoFilter>");
}

// An OR across two fields has no XLSX form: no filterColumn at all, as before.
TEST(OdsToXlsx, AutoFilterOrOfTwoFieldsInsideAnd)
{
	const std::wstring filter = auto_filter(L"xl/tables/table3.xml");
	EXPECT_EQ(filter.find(L"<autoFilter ref=\"A1:C5\""), 0u) << filter;
	EXPECT_EQ(filter.find(L"filterColumn"), std::wstring::npos) << filter;
}

// Field 2 needs "> 0 and (> 10 or < 5)": no filterColumn for it, field 0 keeps its own.
TEST(OdsToXlsx, AutoFilterFieldWithAndAndOrIsLeftOut)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table4.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"0\"><filters><filter val=\"apple\"/></filters></filterColumn>"
		L"</autoFilter>");
}

// "East or matches N.*": XLSX has no regular expressions. Writing only "East" would hide the North rows, so
// the field gets no filterColumn.
TEST(OdsToXlsx, AutoFilterOrWithUnmappedConditionIsLeftOut)
{
	const std::wstring filter = auto_filter(L"xl/tables/table5.xml");
	EXPECT_EQ(filter.find(L"<autoFilter ref=\"A1:C5\""), 0u) << filter;
	EXPECT_EQ(filter.find(L"filterColumn"), std::wstring::npos) << filter;
}

// "text colour red or pear": a colorFilter alone would hide the pear rows.
TEST(OdsToXlsx, AutoFilterOrWithColorIsLeftOut)
{
	const std::wstring filter = auto_filter(L"xl/tables/table6.xml");
	EXPECT_EQ(filter.find(L"<autoFilter ref=\"A1:C5\""), 0u) << filter;
	EXPECT_EQ(filter.find(L"filterColumn"), std::wstring::npos) << filter;
}

// "East and matches N.*": the condition XLSX can express stays; the filter shows more rows, never fewer.
TEST(OdsToXlsx, AutoFilterAndKeepsMappedCondition)
{
	EXPECT_EQ(auto_filter(L"xl/tables/table7.xml"),
		L"<autoFilter ref=\"A1:C5\"><filterColumn colId=\"1\"><customFilters><customFilter operator=\"equal\" val=\"East\"/>"
		L"</customFilters></filterColumn></autoFilter>");
}
