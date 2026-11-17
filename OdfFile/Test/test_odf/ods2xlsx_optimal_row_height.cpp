// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of rows with style:use-optimal-row-height="true". LibreOffice fits such a row to its
// content when it loads the file; the stored style:row-height may have been measured with other fonts. XLSX keeps
// the height but leaves out customHeight, so the spreadsheet application fits the row as well. LibreOffice adds a
// cell's top and bottom padding to the fitted height and XLSX has no cell padding, so a row with a padded cell keeps
// its custom height.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// Sheet 1 "Rows", one case per row, every row 1 cm high:
	// 1 optimal, plain cell
	// 2 optimal, cell padding 0.071 cm a side
	// 3 fixed height (use-optimal-row-height="false")
	// 4 optimal, padding 0.035 cm a side (LibreOffice's default, 1 pt)
	// 5 optimal, cell style inherits fo:padding="0.1cm" from its parent style
	// 6 optimal, cell style sets padding-top and padding-bottom to 0 over that parent
	// 7 optimal, padded row default cell style, cell without a style
	// 8 optimal, text cell followed by an empty cell with the padded style
	// 9 optimal, padding 0.05 cm a side (2.8 pt in all, above the default)
	// Sheet 2 "Columns": columns B..D have the padded default cell style; row 1 is optimal with cell A filled and
	// three empty cells without a style.
	// Sheet 3 "Alternating": the default cell styles of columns A..E alternate between two styles without padding,
	// column F's is padded. Row 1 (optimal): text in A, empty cells without a style in B..F; row 2 (optimal): the
	// same up to D.
	std::wstring sheet_xml(int sheet)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_optimal_row_height.ods");
		if (dir.empty())
			return std::wstring();
		return read_part(dir, L"xl/worksheets/sheet" + std::to_wstring(sheet) + L".xml");
	}

	// The start tag of row r, empty if none.
	std::wstring row_tag(const std::wstring& xml, int r)
	{
		const size_t pos = xml.find(L"<row r=\"" + std::to_wstring(r) + L"\"");
		if (pos == std::wstring::npos)
			return std::wstring();
		const size_t end = xml.find(L">", pos);
		return end == std::wstring::npos ? std::wstring() : xml.substr(pos, end - pos + 1);
	}

	// Value of an attribute in a start tag, or "(none)" if it is absent.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"(none)";
		const size_t begin = pos + key.size();
		const size_t end = tag.find(L"\"", begin);
		return end == std::wstring::npos ? L"(none)" : tag.substr(begin, end - begin);
	}

	void expect_row(int sheet, int r, const std::wstring& customHeight)
	{
		const std::wstring xml = sheet_xml(sheet);
		ASSERT_FALSE(xml.empty()) << "ODS -> XLSX conversion failed";
		EXPECT_TRUE(is_well_formed(xml));

		const std::wstring tag = row_tag(xml, r);
		ASSERT_FALSE(tag.empty()) << "no row " << r;
		EXPECT_EQ(attribute(tag, L"customHeight"), customHeight) << std::string(tag.begin(), tag.end());
		// 1 cm in x2t's points (length.cpp cm_to_pt), written with three decimals
		EXPECT_EQ(attribute(tag, L"ht"), L"28.345") << "the stored height is kept";
	}
}

TEST(OdsToXlsxOptimalRowHeight, OptimalRowIsFittedByTheApplication)
{
	expect_row(1, 1, L"(none)");
}

TEST(OdsToXlsxOptimalRowHeight, PaddedCellKeepsTheHeight)
{
	expect_row(1, 2, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, FixedRowKeepsTheHeight)
{
	expect_row(1, 3, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, DefaultPaddingDoesNotCount)
{
	expect_row(1, 4, L"(none)");
}

TEST(OdsToXlsxOptimalRowHeight, PaddingInheritedFromParentStyle)
{
	expect_row(1, 5, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, OwnSidesOverrideParentPadding)
{
	expect_row(1, 6, L"(none)");
}

TEST(OdsToXlsxOptimalRowHeight, PaddedRowDefaultCellStyle)
{
	expect_row(1, 7, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, PaddedEmptyCellCounts)
{
	expect_row(1, 8, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, PaddingAboveTheDefaultKeepsTheHeight)
{
	expect_row(1, 9, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, PaddedColumnDefaultCellStyle)
{
	expect_row(2, 1, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, PaddedColumnAfterAlternatingColumnStyles)
{
	expect_row(3, 1, L"1");
}

TEST(OdsToXlsxOptimalRowHeight, CellsBeforeThePaddedColumn)
{
	expect_row(3, 2, L"(none)");
}
