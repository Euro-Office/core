// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: pivot tables become table:data-pilot-table elements with their
// source range, place and fields. The fixture has a sheet "Data" (A1:D9: Year,
// Region, Product, Revenue; text in shared strings, as Excel writes it) and a
// sheet "Pivot Sheet" with two pivot tables on
// one pivot cache, plus one on a cache in another workbook:
//   RevenueByRegion  location A5:E9, page field Year (2026 selected), row
//                    field Region, column field Product (item Plums hidden),
//                    data field "Sum of Revenue"; the field items are not in
//                    shared item order, so items are looked up through x
//   UnitsByProduct   location A14:C18, rowGrandTotals="0", page field Region
//                    with no item selected, row field Product, the data
//                    layout field on columns, data fields "Count of Revenue"
//                    (count) and "Total Revenue" (sum)
//   ExternalSource   its cache's worksheetSource has an r:id (external link)
// The second fixture, xlsx_pivot_tables_review.xlsx, has the same "Data" sheet
// (plus an Excel table SalesTable on A1:D10 with one totals row) and one pivot
// table per case on its "Pivot Sheet":
//   ShowAs           data fields "Pct of total" (percentOfTotal), "Pct of
//                    North" (percent of Region item North), "Diff to previous"
//                    (difference to the previous Region item), "Running"
//                    (runTotal over Region)
//   Duplicates       row field Region and column field Product (Plums hidden)
//                    each listed twice
//   BlankHidden      row field Product with its blank item hidden
//   DatePage         Year as date items, page field Year with 2026 selected
//   DateRows         Year as date items on rows, no filter
//   BoolHidden       Year as boolean items on rows, FALSE hidden
//   CalcUsed         data field on the calculated field Double (Revenue*2)
//   CalcUnused       same cache, Double not used
//   Grouped          row field Year with grouped items (fieldGroup)
//   TableSource      its cache's worksheetSource is the table name SalesTable
//   ErrorHidden      row field Product with an error item (#N/A) hidden
//   Filtered         a label filter (Region equals North)
//   Sorted           row field Region sorted descending
//   NoSubtotals      row fields Region and Product, Region defaultSubtotal="0"
//   CustomSubtotal   row fields Region and Product, Region sumSubtotal="1"
//   ShowAsExt        data field "% of parent row total" (x14:dataField
//                    pivotShowAs in the data field's extension list)
//   SortManual       as NoSubtotals, but Region has sortType="manual" and
//                    defaultSubtotal="1" (the defaults, written out)

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	// The value of attribute name="..." inside tag (with &apos; decoded), or an
	// empty string.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + key.size();
		const size_t end = tag.find(L'"', start);
		std::wstring value = end == std::wstring::npos ? L"" : tag.substr(start, end - start);
		for (size_t at = value.find(L"&apos;"); at != std::wstring::npos; at = value.find(L"&apos;", at + 1))
			value.replace(at, 6, L"'");
		return value;
	}

	const std::wstring& content()
	{
		static const std::wstring result = []
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/xlsx_pivot_tables.xlsx");
			return dir.empty() ? std::wstring() : read_part(dir, L"content.xml");
		}();
		return result;
	}

	const std::wstring& review_content()
	{
		static const std::wstring result = []
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/xlsx_pivot_tables_review.xlsx");
			return dir.empty() ? std::wstring() : read_part(dir, L"content.xml");
		}();
		return result;
	}

	// The whole <table:data-pilot-table> element named name in xml, or an empty string.
	std::wstring pilot_table(const std::wstring& name, const std::wstring& xml = content())
	{
		size_t pos = 0;
		while ((pos = xml.find(L"<table:data-pilot-table ", pos)) != std::wstring::npos)
		{
			const size_t tag_end = xml.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				return L"";
			size_t end = tag_end + 1;
			if (xml[tag_end - 1] != L'/')
			{
				const size_t close = xml.find(L"</table:data-pilot-table>", pos);
				end = close == std::wstring::npos ? std::wstring::npos : close + 25;
			}
			if (end == std::wstring::npos)
				return L"";
			const std::wstring element = xml.substr(pos, end - pos);
			if (attribute(xml.substr(pos, tag_end - pos), L"table:name") == name)
				return element;
			pos = end;
		}
		return L"";
	}

	struct Field
	{
		std::wstring tag;	  // the start tag
		std::wstring element; // the whole element
	};

	// The table:data-pilot-field elements of a pilot table, in document order.
	std::vector<Field> fields(const std::wstring& table)
	{
		std::vector<Field> result;
		size_t pos = 0;
		while ((pos = table.find(L"<table:data-pilot-field ", pos)) != std::wstring::npos)
		{
			const size_t tag_end = table.find(L'>', pos);
			if (tag_end == std::wstring::npos)
				break;
			size_t end = tag_end + 1;
			if (table[tag_end - 1] != L'/')
			{
				const size_t close = table.find(L"</table:data-pilot-field>", pos);
				end = close == std::wstring::npos ? tag_end + 1 : close + 25;
			}
			result.push_back({table.substr(pos, tag_end - pos), table.substr(pos, end - pos)});
			pos = end;
		}
		return result;
	}

	// The fields of table with the given source field name and orientation.
	std::vector<Field> fields_of(const std::wstring& table, const std::wstring& source, const std::wstring& orientation)
	{
		std::vector<Field> result;
		for (const Field& field : fields(table))
		{
			if (attribute(field.tag, L"table:source-field-name") == source &&
				attribute(field.tag, L"table:orientation") == orientation)
				result.push_back(field);
		}
		return result;
	}

	std::wstring source_range(const std::wstring& table)
	{
		const size_t pos = table.find(L"<table:source-cell-range ");
		if (pos == std::wstring::npos)
			return L"";
		return attribute(table.substr(pos, table.find(L'>', pos) - pos), L"table:cell-range-address");
	}
}

// Every pivot table of a sheet is written, not only the first, and the
// content is well-formed.
TEST(XlsxToOds, PivotTablesAllWritten)
{
	ASSERT_TRUE(is_well_formed(content()));
	EXPECT_FALSE(pilot_table(L"RevenueByRegion").empty());
	EXPECT_FALSE(pilot_table(L"UnitsByProduct").empty());
}

// No attribute of a pilot table or its fields is written without a value
// (an unset value used to come out as "--").
TEST(XlsxToOds, PivotTableAttributesHaveValues)
{
	const std::wstring& xml = content();
	const size_t start = xml.find(L"<table:data-pilot-tables");
	ASSERT_NE(start, std::wstring::npos);
	const std::wstring tables = xml.substr(start, xml.find(L"</table:data-pilot-tables>", start) - start);
	EXPECT_EQ(tables.find(L"\"--\""), std::wstring::npos) << tables;
}

// The source is the cache's sheet range; the target range starts at the
// location, moved up over the page field and one blank row (ODF counts page
// fields into the table, Excel places them above it).
TEST(XlsxToOds, PivotTableSourceAndTarget)
{
	const std::wstring first = pilot_table(L"RevenueByRegion");
	ASSERT_FALSE(first.empty());
	EXPECT_EQ(source_range(first), L"Data.A1:Data.D9");
	EXPECT_EQ(attribute(first, L"table:target-range-address"), L"'Pivot Sheet'.A3:'Pivot Sheet'.E9");

	const std::wstring second = pilot_table(L"UnitsByProduct");
	ASSERT_FALSE(second.empty());
	EXPECT_EQ(source_range(second), L"Data.A1:Data.D9");
	EXPECT_EQ(attribute(second, L"table:target-range-address"), L"'Pivot Sheet'.A12:'Pivot Sheet'.C18");
}

// Row, column and page fields keep their orientation, the selected page and
// the hidden item; the data field keeps its function and name.
TEST(XlsxToOds, PivotTableFields)
{
	const std::wstring table = pilot_table(L"RevenueByRegion");
	ASSERT_FALSE(table.empty());
	EXPECT_EQ(fields_of(table, L"Region", L"row").size(), 1u) << table;

	const std::vector<Field> product = fields_of(table, L"Product", L"column");
	ASSERT_EQ(product.size(), 1u) << table;
	EXPECT_NE(product[0].element.find(L"<table:data-pilot-member table:name=\"Plums\" table:display=\"false\""),
			  std::wstring::npos)
		<< product[0].element;
	EXPECT_EQ(product[0].element.find(L"table:name=\"Apples\""), std::wstring::npos) << product[0].element;

	const std::vector<Field> year = fields_of(table, L"Year", L"page");
	ASSERT_EQ(year.size(), 1u) << table;
	EXPECT_EQ(attribute(year[0].tag, L"table:selected-page"), L"2026");

	const std::vector<Field> revenue = fields_of(table, L"Revenue", L"data");
	ASSERT_EQ(revenue.size(), 1u) << table;
	EXPECT_EQ(attribute(revenue[0].tag, L"table:function"), L"sum");
	EXPECT_EQ(attribute(revenue[0].tag, L"tableooo:display-name"), L"Sum of Revenue");
}

// Two data fields on one source field, the data layout field on columns,
// unused fields hidden, and only the column grand total.
TEST(XlsxToOds, PivotTableDataLayoutAndGrandTotal)
{
	const std::wstring table = pilot_table(L"UnitsByProduct");
	ASSERT_FALSE(table.empty());
	EXPECT_EQ(attribute(table.substr(0, table.find(L'>')), L"table:grand-total"), L"column");

	const std::vector<Field> layout = fields_of(table, L"", L"column");
	ASSERT_EQ(layout.size(), 1u) << table;
	EXPECT_EQ(attribute(layout[0].tag, L"table:is-data-layout-field"), L"true");

	const std::vector<Field> revenue = fields_of(table, L"Revenue", L"data");
	ASSERT_EQ(revenue.size(), 2u) << table;
	EXPECT_EQ(attribute(revenue[0].tag, L"table:function"), L"count");
	EXPECT_EQ(attribute(revenue[0].tag, L"tableooo:display-name"), L"Count of Revenue");
	EXPECT_EQ(attribute(revenue[1].tag, L"table:function"), L"sum");

	EXPECT_EQ(fields_of(table, L"Product", L"row").size(), 1u) << table;
	EXPECT_EQ(fields_of(table, L"Year", L"hidden").size(), 1u) << table;
}

// A page field without a selected item still has table:selected-page (ODF
// requires it), empty and marked to be ignored.
TEST(XlsxToOds, PivotTablePageFieldWithoutSelection)
{
	const std::wstring table = pilot_table(L"UnitsByProduct");
	ASSERT_FALSE(table.empty());
	const std::vector<Field> region = fields_of(table, L"Region", L"page");
	ASSERT_EQ(region.size(), 1u) << table;
	EXPECT_NE(region[0].tag.find(L" table:selected-page=\"\""), std::wstring::npos) << region[0].tag;
	EXPECT_EQ(attribute(region[0].tag, L"loext:ignore-selected-page"), L"true") << region[0].tag;
}

// A selected page is written as LibreOffice writes it: the item in
// table:selected-page, marked to be ignored, and every other item a hidden
// member. A plain selected page left the other pivot table on the same source
// (UnitsByProduct) empty after a refresh in LibreOffice.
TEST(XlsxToOds, PivotTableSelectedPageAsHiddenMembers)
{
	const std::wstring table = pilot_table(L"RevenueByRegion");
	ASSERT_FALSE(table.empty());
	const std::vector<Field> year = fields_of(table, L"Year", L"page");
	ASSERT_EQ(year.size(), 1u) << table;
	EXPECT_EQ(attribute(year[0].tag, L"table:selected-page"), L"2026") << year[0].tag;
	EXPECT_EQ(attribute(year[0].tag, L"loext:ignore-selected-page"), L"true") << year[0].tag;
	EXPECT_NE(year[0].element.find(L"<table:data-pilot-member table:name=\"2025\" table:display=\"false\""),
			  std::wstring::npos)
		<< year[0].element;
	EXPECT_EQ(year[0].element.find(L"table:name=\"2026\""), std::wstring::npos) << year[0].element;
}

// A pivot table over a range in another workbook is not written as a range
// of this document.
TEST(XlsxToOds, PivotTableExternalSourceSkipped)
{
	ASSERT_TRUE(is_well_formed(content()));
	EXPECT_TRUE(pilot_table(L"ExternalSource").empty()) << pilot_table(L"ExternalSource");
}

// A field listed twice on an axis is written once, with its hidden member once.
TEST(XlsxToOds, PivotTableDuplicateAxisFieldWrittenOnce)
{
	ASSERT_TRUE(is_well_formed(review_content()));
	const std::wstring table = pilot_table(L"Duplicates", review_content());
	ASSERT_FALSE(table.empty());
	EXPECT_EQ(fields_of(table, L"Region", L"row").size(), 1u) << table;
	const std::vector<Field> product = fields_of(table, L"Product", L"column");
	ASSERT_EQ(product.size(), 1u) << table;
	EXPECT_NE(product[0].element.find(L"table:name=\"Plums\""), std::wstring::npos) << product[0].element;
	EXPECT_EQ(fields_of(table, L"Region", L"hidden").size() + fields_of(table, L"Product", L"hidden").size(), 0u) << table;
}

// "Show values as" becomes table:data-pilot-field-reference on the base field,
// so a refresh computes percentages and differences, not plain sums. A base
// item that names no member is written as an empty member name, which ODF
// requires with member-type named.
TEST(XlsxToOds, PivotTableShowValuesAs)
{
	const std::wstring table = pilot_table(L"ShowAs", review_content());
	ASSERT_FALSE(table.empty());
	const std::vector<Field> revenue = fields_of(table, L"Revenue", L"data");
	ASSERT_EQ(revenue.size(), 5u) << table;
	struct Expected
	{
		const wchar_t* type;
		const wchar_t* field;
		const wchar_t* member_type;
		const wchar_t* member;
		bool has_member;
	};
	const Expected expected[] = {{L"total-percentage", L"Year", L"named", L"2025", true},
								 {L"member-percentage", L"Region", L"named", L"North", true},
								 {L"member-difference", L"Region", L"previous", L"", false},
								 {L"running-total", L"Region", L"named", L"East", true},
								 {L"index", L"Year", L"named", L"", true}};
	for (size_t i = 0; i < 5; i++)
	{
		EXPECT_EQ(attribute(revenue[i].tag, L"table:function"), L"sum") << revenue[i].tag;
		const size_t pos = revenue[i].element.find(L"<table:data-pilot-field-reference ");
		ASSERT_NE(pos, std::wstring::npos) << revenue[i].element;
		const std::wstring reference = revenue[i].element.substr(pos, revenue[i].element.find(L'>', pos) - pos);
		EXPECT_EQ(attribute(reference, L"table:type"), expected[i].type) << reference;
		EXPECT_EQ(attribute(reference, L"table:field-name"), expected[i].field) << reference;
		EXPECT_EQ(attribute(reference, L"table:member-type"), expected[i].member_type) << reference;
		EXPECT_EQ(attribute(reference, L"table:member-name"), expected[i].member) << reference;
		EXPECT_EQ(reference.find(L" table:member-name=\"") != std::wstring::npos, expected[i].has_member) << reference;
	}
}

// A hidden blank item is the member with the empty name, as LibreOffice names
// it.
TEST(XlsxToOds, PivotTableHiddenBlankItem)
{
	const std::wstring table = pilot_table(L"BlankHidden", review_content());
	ASSERT_FALSE(table.empty());
	const std::vector<Field> product = fields_of(table, L"Product", L"row");
	ASSERT_EQ(product.size(), 1u) << table;
	EXPECT_NE(product[0].element.find(L"<table:data-pilot-member table:name=\"\" table:display=\"false\""),
			  std::wstring::npos)
		<< product[0].element;
}

// A hidden error item is the member named by the error text, as LibreOffice
// names it.
TEST(XlsxToOds, PivotTableHiddenErrorItem)
{
	const std::wstring table = pilot_table(L"ErrorHidden", review_content());
	ASSERT_FALSE(table.empty());
	const std::vector<Field> product = fields_of(table, L"Product", L"row");
	ASSERT_EQ(product.size(), 1u) << table;
	EXPECT_NE(product[0].element.find(L"<table:data-pilot-member table:name=\"#N/A\" table:display=\"false\""),
			  std::wstring::npos)
		<< product[0].element;
}

// LibreOffice names date and boolean members by the number format of the
// source cells, which the pivot cache does not keep. A pivot table that
// filters on such an item is not written (its cells keep their values), so a
// refresh cannot drop the filter; date items without a filter are written.
TEST(XlsxToOds, PivotTableFilterOnUnnamedItemsNotWritten)
{
	ASSERT_TRUE(is_well_formed(review_content()));
	EXPECT_TRUE(pilot_table(L"DatePage", review_content()).empty()) << pilot_table(L"DatePage", review_content());
	EXPECT_TRUE(pilot_table(L"BoolHidden", review_content()).empty()) << pilot_table(L"BoolHidden", review_content());

	const std::wstring dates = pilot_table(L"DateRows", review_content());
	ASSERT_FALSE(dates.empty());
	EXPECT_EQ(fields_of(dates, L"Year", L"row").size(), 1u) << dates;
}

// A calculated field or grouped items have no source column to refresh from:
// a pivot table that uses one is not written; an unused calculated field is
// left out of the fields.
TEST(XlsxToOds, PivotTableDerivedFields)
{
	ASSERT_TRUE(is_well_formed(review_content()));
	EXPECT_TRUE(pilot_table(L"CalcUsed", review_content()).empty()) << pilot_table(L"CalcUsed", review_content());
	EXPECT_TRUE(pilot_table(L"Grouped", review_content()).empty()) << pilot_table(L"Grouped", review_content());

	const std::wstring table = pilot_table(L"CalcUnused", review_content());
	ASSERT_FALSE(table.empty());
	EXPECT_EQ(fields_of(table, L"Revenue", L"data").size(), 1u) << table;
	EXPECT_EQ(table.find(L"table:source-field-name=\"Double\""), std::wstring::npos) << table;
}

// A pivot table over an Excel table reads the table's range without its
// totals row; the fixture names the table in other case ("salestable"), as
// Excel table names are case-insensitive.
TEST(XlsxToOds, PivotTableSourceIsATable)
{
	const std::wstring table = pilot_table(L"TableSource", review_content());
	ASSERT_FALSE(table.empty());
	EXPECT_EQ(source_range(table), L"Data.A1:Data.D9");
	EXPECT_EQ(fields_of(table, L"Region", L"row").size(), 1u) << table;
}

// What the converter does not write yet would be dropped by a refresh: label,
// value and date filters, the sort order of a field, custom subtotal
// functions, and the "show values as" kinds that only the extension list
// holds. Such a pivot table is not written (its cells keep their values); the
// defaults written out do not count.
TEST(XlsxToOds, PivotTableUnwrittenOptionsSkipTheTable)
{
	ASSERT_TRUE(is_well_formed(review_content()));
	for (const wchar_t* name : {L"Filtered", L"Sorted", L"CustomSubtotal", L"ShowAsExt"})
		EXPECT_TRUE(pilot_table(name, review_content()).empty()) << name << ": " << pilot_table(name, review_content());

	const std::wstring table = pilot_table(L"SortManual", review_content());
	ASSERT_FALSE(table.empty());
	EXPECT_EQ(fields_of(table, L"Region", L"row").size(), 1u) << table;
	EXPECT_EQ(fields_of(table, L"Product", L"row").size(), 1u) << table;
}

// Excel's automatic subtotals of a row or column field become
// table:function="auto" (LibreOffice shows no subtotals without
// table:data-pilot-subtotals); defaultSubtotal="0" writes none. Page, hidden
// and data fields get none.
TEST(XlsxToOds, PivotTableAutomaticSubtotals)
{
	const std::wstring automatic = L"<table:data-pilot-subtotals><table:data-pilot-subtotal table:function=\"auto\"/></table:data-pilot-subtotals>";

	const std::wstring manual = pilot_table(L"SortManual", review_content());
	ASSERT_FALSE(manual.empty());
	for (const wchar_t* name : {L"Region", L"Product"})
	{
		const std::vector<Field> field = fields_of(manual, name, L"row");
		ASSERT_EQ(field.size(), 1u) << manual;
		EXPECT_NE(field[0].element.find(automatic), std::wstring::npos) << field[0].element;
	}
	const std::vector<Field> sum = fields_of(manual, L"Revenue", L"data");
	ASSERT_EQ(sum.size(), 1u) << manual;
	EXPECT_EQ(sum[0].element.find(L"data-pilot-subtotal"), std::wstring::npos) << sum[0].element;

	const std::wstring none = pilot_table(L"NoSubtotals", review_content());
	ASSERT_FALSE(none.empty());
	const std::vector<Field> region = fields_of(none, L"Region", L"row");
	ASSERT_EQ(region.size(), 1u) << none;
	EXPECT_EQ(region[0].element.find(L"data-pilot-subtotal"), std::wstring::npos) << region[0].element;
	const std::vector<Field> product = fields_of(none, L"Product", L"row");
	ASSERT_EQ(product.size(), 1u) << none;
	EXPECT_NE(product[0].element.find(automatic), std::wstring::npos) << product[0].element;

	// Subtotals and hidden members share one table:data-pilot-level, subtotals first.
	const std::wstring table = pilot_table(L"RevenueByRegion");
	ASSERT_FALSE(table.empty());
	const std::vector<Field> column = fields_of(table, L"Product", L"column");
	ASSERT_EQ(column.size(), 1u) << table;
	const size_t level = column[0].element.find(L"<table:data-pilot-level");
	ASSERT_NE(level, std::wstring::npos) << column[0].element;
	EXPECT_EQ(column[0].element.find(L"<table:data-pilot-level", level + 1), std::wstring::npos) << column[0].element;
	EXPECT_LT(column[0].element.find(automatic), column[0].element.find(L"<table:data-pilot-members>")) << column[0].element;
	const std::vector<Field> year = fields_of(table, L"Year", L"page");
	ASSERT_EQ(year.size(), 1u) << table;
	EXPECT_EQ(year[0].element.find(L"data-pilot-subtotal"), std::wstring::npos) << year[0].element;
}
