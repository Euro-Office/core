// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: the conditions of an autoFilter become the table:filter of the
// database range. One fixture sheet per case.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	const std::wstring& autofilter_content()
	{
		static const std::wstring xml = []()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/xlsx_autofilter_conditions.xlsx");
			return dir.empty() ? std::wstring() : read_part(dir, L"content.xml");
		}();
		return xml;
	}

	// The table:database-range element whose range is on the given sheet.
	std::wstring database_range(const std::wstring& sheet)
	{
		const std::wstring& xml = autofilter_content();
		const size_t attr = xml.find(L"table:target-range-address=\"" + sheet + L".");
		if (attr == std::wstring::npos)
			return L"";
		const size_t start = xml.rfind(L"<table:database-range ", attr);
		if (start == std::wstring::npos)
			return L"";
		size_t end = xml.find(L"<table:database-range ", attr);
		const size_t close = xml.find(L"</table:database-ranges>", attr);
		if (end == std::wstring::npos || (close != std::wstring::npos && close < end))
			end = close;
		return end == std::wstring::npos ? xml.substr(start) : xml.substr(start, end - start);
	}

	class XlsxToOdsAutoFilter : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_FALSE(autofilter_content().empty()) << "XLSX -> ODS conversion failed";
			ASSERT_TRUE(is_well_formed(autofilter_content()));
		}
	};
}

// A value list: one "=" condition with a set item per value.
TEST_F(XlsxToOdsAutoFilter, ValueList)
{
	const std::wstring range = database_range(L"Values");
	EXPECT_NE(range.find(L"<table:filter><table:filter-condition table:field-number=\"1\" table:value=\"East\" "
						 L"table:operator=\"=\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:filter-set-item table:value=\"East\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:filter-set-item table:value=\"North\""), std::wstring::npos) << range;
}

// "(Blanks)" next to a value is an empty set item.
TEST_F(XlsxToOdsAutoFilter, ValueAndBlank)
{
	const std::wstring range = database_range(L"Blank");
	EXPECT_NE(range.find(L"table:field-number=\"0\" table:value=\"x\" table:operator=\"=\""), std::wstring::npos)
		<< range;
	EXPECT_NE(range.find(L"<table:filter-set-item table:value=\"x\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:filter-set-item table:value=\"\""), std::wstring::npos) << range;
}

// Only "(Blanks)": the empty operator.
TEST_F(XlsxToOdsAutoFilter, OnlyBlank)
{
	const std::wstring range = database_range(L"OnlyBlank");
	EXPECT_NE(range.find(L"table:field-number=\"2\" table:value=\"\" table:operator=\"empty\""), std::wstring::npos)
		<< range;
	EXPECT_EQ(range.find(L"table:filter-set-item"), std::wstring::npos) << range;
}

// Two custom conditions joined by and, compared as numbers.
TEST_F(XlsxToOdsAutoFilter, CustomAndNumbers)
{
	const std::wstring range = database_range(L"AndNumbers");
	EXPECT_NE(range.find(L"<table:filter><table:filter-and>"), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:field-number=\"2\" table:value=\"10\" table:operator=\"&gt;\" "
						 L"table:data-type=\"number\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:field-number=\"2\" table:value=\"50\" table:operator=\"&lt;=\" "
						 L"table:data-type=\"number\""), std::wstring::npos) << range;
}

// "*an*" or not "b*": contains and does not begin with, joined by or.
TEST_F(XlsxToOdsAutoFilter, CustomOrWildcards)
{
	const std::wstring range = database_range(L"OrText");
	EXPECT_NE(range.find(L"<table:filter><table:filter-or>"), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:value=\"an\" table:operator=\"contains\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:value=\"b\" table:operator=\"does-not-begin-with\""), std::wstring::npos) << range;
}

// top10 with top="0" percent="1".
TEST_F(XlsxToOdsAutoFilter, BottomPercent)
{
	const std::wstring range = database_range(L"Bottom");
	EXPECT_NE(range.find(L"table:field-number=\"1\" table:value=\"10\" table:operator=\"bottom percent\" "
						 L"table:data-type=\"number\""), std::wstring::npos) << range;
}

// Columns are joined by and; a column with an or keeps it in a nested
// table:filter-or. Field numbers count from the first column of the range (B).
TEST_F(XlsxToOdsAutoFilter, AndOfColumnWithOr)
{
	const std::wstring range = database_range(L"Mixed");
	EXPECT_NE(range.find(L"<table:filter><table:filter-and><table:filter-condition table:field-number=\"0\" "
						 L"table:value=\"a\" table:operator=\"=\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:filter-or><table:filter-condition table:field-number=\"2\" table:value=\"1\" "
						 L"table:operator=\"&gt;\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:field-number=\"2\" table:value=\"-1.5\" table:operator=\"&lt;\" "
						 L"table:data-type=\"number\""), std::wstring::npos) << range;
}

// A wildcard inside the value has no ODF operator here: no condition, the
// range itself is kept.
TEST_F(XlsxToOdsAutoFilter, InnerWildcardWritesNoCondition)
{
	const std::wstring range = database_range(L"Wildcard");
	ASSERT_FALSE(range.empty());
	EXPECT_EQ(range.find(L"table:filter"), std::wstring::npos) << range;
}

TEST_F(XlsxToOdsAutoFilter, ValuesEscaped)
{
	const std::wstring range = database_range(L"Escape");
	EXPECT_NE(range.find(L"<table:filter-set-item table:value=\"R&amp;D\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"<table:filter-set-item table:value=\"x&lt;y\""), std::wstring::npos) << range;
}

// The autoFilter of a table part.
TEST_F(XlsxToOdsAutoFilter, TablePart)
{
	const std::wstring range = database_range(L"Table");
	EXPECT_NE(range.find(L"table:name=\"Orders\""), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:field-number=\"1\" table:value=\"East\" table:operator=\"=\""), std::wstring::npos)
		<< range;
}

// Excel's "non-blanks" is not equal " "; the value is trimmed, as LibreOffice
// does, which gives the not empty operator.
TEST_F(XlsxToOdsAutoFilter, NotBlank)
{
	const std::wstring range = database_range(L"NotBlank");
	EXPECT_NE(range.find(L"table:field-number=\"0\" table:value=\"\" table:operator=\"!empty\""), std::wstring::npos)
		<< range;
}

// "*a" is ends with; top10 without attributes is the top N values.
TEST_F(XlsxToOdsAutoFilter, EndsWithAndTopValues)
{
	const std::wstring range = database_range(L"TopEnds");
	EXPECT_NE(range.find(L"<table:filter><table:filter-and>"), std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:field-number=\"0\" table:value=\"a\" table:operator=\"ends-with\""),
			  std::wstring::npos) << range;
	EXPECT_NE(range.find(L"table:field-number=\"2\" table:value=\"3\" table:operator=\"top values\" "
						 L"table:data-type=\"number\""), std::wstring::npos) << range;
}

// Colour and date group filters have no condition here, and "*" matches every
// row: no table:filter at all.
TEST_F(XlsxToOdsAutoFilter, SkippedKindsWriteNoCondition)
{
	const std::wstring range = database_range(L"Skipped");
	ASSERT_FALSE(range.empty());
	EXPECT_EQ(range.find(L"table:filter"), std::wstring::npos) << range;
}

// An empty <filter/> is not a value (LibreOffice skips it too).
TEST_F(XlsxToOdsAutoFilter, EmptyFilterValueIgnored)
{
	const std::wstring range = database_range(L"EmptyValue");
	EXPECT_NE(range.find(L"table:field-number=\"1\" table:value=\"East\" table:operator=\"=\""), std::wstring::npos)
		<< range;
	EXPECT_EQ(range.find(L"table:filter-set-item"), std::wstring::npos) << range;
}

// A top10 value that is not a finite number writes no condition.
TEST_F(XlsxToOdsAutoFilter, TopValueNotANumber)
{
	const std::wstring range = database_range(L"TopNaN");
	ASSERT_FALSE(range.empty());
	EXPECT_EQ(range.find(L"table:filter"), std::wstring::npos) << range;
}
