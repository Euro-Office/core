// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS conversions checked on the converted content.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	std::wstring convert_xlsx_content(const std::wstring& fixture)
	{
		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		if (dir.empty())
			return L"";
		return read_part(dir, L"content.xml");
	}

	// The start tag of the table cell whose text is the given value.
	std::wstring cell_with_text(const std::wstring& xml, const std::wstring& text)
	{
		const size_t pos = xml.find(L"<text:p>" + text + L"</text:p>");
		if (pos == std::wstring::npos)
			return L"";
		const size_t cell = xml.rfind(L"<table:table-cell", pos);
		if (cell == std::wstring::npos)
			return L"";
		return xml.substr(cell, xml.find(L'>', cell) + 1 - cell);
	}
}

// t="d" cells hold an ISO 8601 date (ST_CellType); here in a Strict package.
TEST(XlsxToOds, IsoDateCell)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_type_d_date.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"office:value-type=\"date\""), std::wstring::npos);
	EXPECT_NE(xml.find(L"office:date-value=\"2023-03-15\""), std::wstring::npos);
}

// Serial date-times are rounded to whole seconds, not cut: 15:05 and 04:52:56 are
// binary fractions a little below the exact value, and 23:59:59.9999 is midnight
// of the next day.
TEST(XlsxToOds, SerialDateTimeRoundedToWholeSeconds)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_datetime_serial_seconds.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	for (const wchar_t* expected : {L"2023-11-22T15:05:00\"", L"2014-01-01T04:52:56\"", L"2023-11-23\""})
	{
		EXPECT_NE(xml.find(std::wstring(L"office:date-value=\"") + expected), std::wstring::npos) << expected;
	}
	for (const wchar_t* cut : {L"15:04:59", L"04:52:55", L"23:59:59"})
	{
		EXPECT_EQ(xml.find(cut), std::wstring::npos) << cut;
	}
}

// Date-time values, and t="d" values that are not valid dates: these are kept
// as text in string cells (a date cell needs office:date-value).
TEST(XlsxToOds, IsoDateTimeCellAndInvalidDateCells)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_type_d_datetime.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));

	// as written; 10:30 gets the seconds xsd:dateTime needs; the fraction is kept and the
	// trailing Z dropped; a leap day; the serial number 45000, converted as before
	for (const wchar_t* expected : {L"2023-03-15T10:30:00\"", L"2023-03-16T10:30:00\"", L"2023-03-17T10:30:15.5\"",
		L"2024-02-29\"", L"2023-03-15\""})
	{
		EXPECT_NE(xml.find(std::wstring(L"office:date-value=\"") + expected), std::wstring::npos) << expected;
	}

	for (const wchar_t* value :
		 {L"not-a-date", L"2023-02-30", L"2023-13-01", L"2023-03-15T24:00", L"2100-02-29", L"1.2.3"})
	{
		const std::wstring cell = cell_with_text(xml, value);
		EXPECT_NE(cell.find(L"office:value-type=\"string\""), std::wstring::npos) << "value " << value << ": " << cell;
		EXPECT_EQ(xml.find(std::wstring(L"office:date-value=\"") + value), std::wstring::npos) << "value " << value;
	}
}

// A formula cell whose cached t="d" value is not a date keeps that value as its
// text, once.
TEST(XlsxToOds, InvalidDateFormulaCellTextWrittenOnce)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_type_d_datetime.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring text = L"<text:p>formula-not-a-date</text:p>";
	const size_t first = xml.find(text);
	ASSERT_NE(first, std::wstring::npos);
	EXPECT_EQ(xml.find(text, first + text.size()), std::wstring::npos) << "the cell text is written twice";
}

// A serial number with no date part is a time of day. office:time-value is an
// xsd:duration, so 0.5 must be written as PT12H00M00S, not as T12:00:00.
TEST(XlsxToOds, TimeOnlySerialWrittenAsDuration)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_time_only_serials.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_TRUE(is_well_formed(xml));
	EXPECT_NE(xml.find(L"office:time-value=\"PT12H00M00S\""), std::wstring::npos);
	EXPECT_EQ(xml.find(L"office:time-value=\"T"), std::wstring::npos);
}

// A zero serial in a date or time formatted cell keeps its value: both cells get
// office:time-value PT00H00M00S, and no time cell is left without a value.
TEST(XlsxToOds, ZeroSerialKeepsTimeValue)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_time_only_serials.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";

	const std::wstring zero = L"office:time-value=\"PT00H00M00S\"";
	size_t count = 0;
	for (size_t pos = xml.find(zero); pos != std::wstring::npos; pos = xml.find(zero, pos + zero.size()))
		++count;
	EXPECT_EQ(count, 2u);

	for (size_t pos = xml.find(L"office:value-type=\"time\""); pos != std::wstring::npos;
		 pos = xml.find(L"office:value-type=\"time\"", pos + 1))
	{
		const size_t start = xml.rfind(L'<', pos);
		const std::wstring tag = xml.substr(start, xml.find(L'>', pos) + 1 - start);
		EXPECT_NE(tag.find(L"office:time-value="), std::wstring::npos) << tag;
	}
}

// A serial number with a date part is still written as a date and time.
TEST(XlsxToOds, DateTimeSerialStaysDateValue)
{
	const std::wstring xml = convert_xlsx_content(L"ExampleFiles/cell_time_only_serials.xlsx");
	ASSERT_FALSE(xml.empty()) << "XLSX -> ODS conversion failed";
	EXPECT_NE(xml.find(L"office:date-value=\"2023-03-15T12:00:00\""), std::wstring::npos);
}
