// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: the database range of a worksheet sort or autoFilter on a sheet whose name is not
// a plain word. An ODF cell address puts such a name in quotes and doubles the quotes in it.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// One sheet per case, data in A1:C6 with a header row. Plain, My-Sheet, Q1.2024, It's and Two Words
	// have a worksheet sortState A2:C6; Q2.2024 and Bob's also have an autoFilter A1:C6. Then "It's v1.3"
	// with only an autoFilter, "It's v1.2" with only a sortState, and Tbl-Sheet with the table part Sales.
	const std::wstring& content()
	{
		static const std::wstring xml = []()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/xlsx_sort_sheet_names.xlsx");
			return dir.empty() ? std::wstring() : read_part(dir, L"content.xml");
		}();
		return xml;
	}

	// The table:database-range element with this target address, "(none)" if there is none.
	std::wstring range_at(const std::wstring& address)
	{
		const std::wstring& xml = content();
		const size_t attr = xml.find(L"table:target-range-address=\"" + address + L"\"");
		if (attr == std::wstring::npos)
			return L"(none)";
		const size_t start = xml.rfind(L"<table:database-range ", attr);
		const size_t tag_end = xml.find(L'>', attr);
		if (start == std::wstring::npos || tag_end == std::wstring::npos)
			return L"(none)";
		if (xml[tag_end - 1] == L'/') // an empty element
			return xml.substr(start, tag_end + 1 - start);
		const size_t end = xml.find(L"</table:database-range>", tag_end);
		if (end == std::wstring::npos)
			return L"(none)";
		return xml.substr(start, end - start);
	}

	size_t count_of(const std::wstring& text, const std::wstring& what)
	{
		size_t count = 0;
		for (size_t pos = text.find(what); pos != std::wstring::npos; pos = text.find(what, pos + what.size()))
			count++;
		return count;
	}

	class XlsxToOdsSortSheetNames : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_FALSE(content().empty()) << "XLSX -> ODS conversion failed";
			ASSERT_TRUE(is_well_formed(content()));
		}
	};
}

TEST_F(XlsxToOdsSortSheetNames, PlainNameStaysUnquoted)
{
	EXPECT_NE(range_at(L"Plain.A2:Plain.C6").find(L"<table:sort>"), std::wstring::npos) << content();
}

TEST_F(XlsxToOdsSortSheetNames, NamesWithDashDotOrSpaceAreQuoted)
{
	EXPECT_NE(range_at(L"&apos;My-Sheet&apos;.A2:&apos;My-Sheet&apos;.C6").find(L"<table:sort>"), std::wstring::npos)
		<< content();
	EXPECT_NE(range_at(L"&apos;Q1.2024&apos;.A2:&apos;Q1.2024&apos;.C6").find(L"<table:sort>"), std::wstring::npos)
		<< content();
	EXPECT_NE(range_at(L"&apos;Two Words&apos;.A2:&apos;Two Words&apos;.C6").find(L"<table:sort>"), std::wstring::npos)
		<< content();
}

TEST_F(XlsxToOdsSortSheetNames, QuoteInNameIsDoubled)
{
	EXPECT_NE(range_at(L"&apos;It&apos;&apos;s&apos;.A2:&apos;It&apos;&apos;s&apos;.C6").find(L"<table:sort>"),
		std::wstring::npos) << content();
}

// The autoFilter and the sort one row below its header are one database range, also on these sheets.
TEST_F(XlsxToOdsSortSheetNames, AutoFilterAndSortShareOneRange)
{
	const std::wstring dot = range_at(L"&apos;Q2.2024&apos;.A1:&apos;Q2.2024&apos;.C6");
	EXPECT_NE(dot.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << dot;
	EXPECT_NE(dot.find(L"<table:sort>"), std::wstring::npos) << dot;

	const std::wstring quote = range_at(L"&apos;Bob&apos;&apos;s&apos;.A1:&apos;Bob&apos;&apos;s&apos;.C6");
	EXPECT_NE(quote.find(L"table:display-filter-buttons=\"true\""), std::wstring::npos) << quote;
	EXPECT_NE(quote.find(L"<table:sort>"), std::wstring::npos) << quote;

	EXPECT_EQ(count_of(content(), L"<table:database-range "), 10u) << content();
}

// The anonymous range of "It's v1.3" is not the range of "It's v1.2": both quoted names start with
// 'It''s v1. and a doubled quote must not be taken for the end of the name.
TEST_F(XlsxToOdsSortSheetNames, SortNextToRangeOfSimilarQuotedName)
{
	const std::wstring sorted = range_at(L"&apos;It&apos;&apos;s v1.2&apos;.A2:&apos;It&apos;&apos;s v1.2&apos;.C6");
	EXPECT_NE(sorted.find(L"<table:sort>"), std::wstring::npos) << content();
	EXPECT_NE(range_at(L"&apos;It&apos;&apos;s v1.3&apos;.A1:&apos;It&apos;&apos;s v1.3&apos;.C6"), L"(none)") << content();
}

TEST_F(XlsxToOdsSortSheetNames, TablePartAddressIsQuoted)
{
	const std::wstring table = range_at(L"&apos;Tbl-Sheet&apos;.A1:&apos;Tbl-Sheet&apos;.C6");
	EXPECT_NE(table.find(L"table:name=\"Sales\""), std::wstring::npos) << content();
}
