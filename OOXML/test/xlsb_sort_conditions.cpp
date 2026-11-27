/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xlsb: the <sortCondition> elements of an autoFilter's <sortState>. Each one becomes a BrtBeginSortCond
// record: flags, the range, an 8-byte value and an empty list name. The value holds the dxfId of a cell or font
// color sort and is zero otherwise; it used to carry whatever was on the stack. A color sort without a dxfId used
// to stop the conversion.

#include "common.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
	const std::wstring kFmla =
		L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"fmla.xlsx";

	const unsigned kBrtBeginSortCond = 532;

	// flags (2 bytes), range (16), value (8), list name (4 bytes: none).
	const size_t kValueOffset = 18;
	const size_t kRecordSize = 30;

	std::string ReadAll(const std::wstring& path)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	// Record type and size are variable-length: 7 bits per byte, the high bit
	// says that another byte follows.
	bool ReadVarUInt(const std::string& data, size_t& pos, size_t maxBytes, unsigned& value)
	{
		value = 0;
		for (size_t i = 0; i < maxBytes; ++i)
		{
			if (pos >= data.size())
				return false;
			unsigned char byte = static_cast<unsigned char>(data[pos++]);
			value |= static_cast<unsigned>(byte & 0x7F) << (7 * i);
			if (!(byte & 0x80))
				return true;
		}
		return false;
	}

	// The payloads of the BrtBeginSortCond records, in file order; false if the stream is cut short.
	bool SortConditionRecords(const std::string& data, std::vector<std::string>& records)
	{
		size_t pos = 0;
		while (pos < data.size())
		{
			unsigned recordType = 0, size = 0;
			if (!ReadVarUInt(data, pos, 2, recordType) || !ReadVarUInt(data, pos, 4, size) ||
				size > data.size() - pos)
				return false;
			if (recordType == kBrtBeginSortCond)
				records.push_back(data.substr(pos, size));
			pos += size;
		}
		return true;
	}

	uint32_t UInt(const std::string& data, size_t pos, size_t bytes)
	{
		uint32_t value = 0;
		for (size_t i = 0; i < bytes; ++i)
			value |= static_cast<uint32_t>(static_cast<unsigned char>(data[pos + i])) << (8 * i);
		return value;
	}

	// fSortDes is bit 0 of the flags, sortOn bits 1-4.
	unsigned SortOn(const std::string& record)
	{
		return (UInt(record, 0, 2) >> 1) & 0xF;
	}

	// Converts the fixture with an autoFilter whose sortState has these conditions on its second sheet to xlsb
	// and returns the BrtBeginSortCond records of xl/worksheets/sheet2.bin.
	bool SortConditions(const std::string& conditions, std::vector<std::string>& records)
	{
		const std::string replacement = "</sheetData><autoFilter ref=\"A1:A4\"><sortState ref=\"A2:A4\">" +
										conditions + "</sortState></autoFilter><pageMargins";
		std::wstring xlsx;
		if (!EditXlsxPart(kFmla, L"xl/worksheets/sheet2.xml", "</sheetData><pageMargins", replacement, xlsx))
			return false;
		const std::wstring work = GetWorkDir();
		const std::wstring xlsb = work + FILE_SEPARATOR_STR + L"result.xlsb";
		std::string error;
		if (!ConvertTo(xlsx, xlsb, work, error))
		{
			ADD_FAILURE() << error;
			return false;
		}
		const std::wstring unpacked = work + FILE_SEPARATOR_STR + L"unpacked";
		if (!Unpack(xlsb, unpacked, error))
		{
			ADD_FAILURE() << error;
			return false;
		}
		const std::string sheetBin = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR
											 L"worksheets" FILE_SEPARATOR_STR L"sheet2.bin");
		return SortConditionRecords(sheetBin, records);
	}
}

TEST(XlsbSortConditions, WritesZeroValueWithoutColorSort)
{
	std::vector<std::string> records;
	ASSERT_TRUE(SortConditions("<sortCondition ref=\"A2:A4\"/>"
							   "<sortCondition descending=\"1\" ref=\"A2:A4\"/>"
							   "<sortCondition sortBy=\"icon\" ref=\"A2:A4\" iconSet=\"3Arrows\" iconId=\"1\"/>",
							   records));
	ASSERT_EQ(3u, records.size());
	const unsigned sortOn[] = {0, 0, 3};
	for (size_t i = 0; i < records.size(); ++i)
	{
		ASSERT_EQ(kRecordSize, records[i].size()) << "condition " << i;
		EXPECT_EQ(sortOn[i], SortOn(records[i])) << "condition " << i;
		EXPECT_EQ(std::string(8, '\0'), records[i].substr(kValueOffset, 8)) << "condition " << i;
	}
	EXPECT_EQ(1u, UInt(records[1], 0, 2) & 1) << "fSortDes of the descending condition";
}

TEST(XlsbSortConditions, WritesDxfIdOfColorSort)
{
	std::vector<std::string> records;
	ASSERT_TRUE(SortConditions("<sortCondition sortBy=\"cellColor\" ref=\"A2:A4\" dxfId=\"3\"/>"
							   "<sortCondition sortBy=\"fontColor\" ref=\"A2:A4\" dxfId=\"4\"/>",
							   records));
	ASSERT_EQ(2u, records.size());
	for (size_t i = 0; i < records.size(); ++i)
	{
		ASSERT_EQ(kRecordSize, records[i].size()) << "condition " << i;
		EXPECT_EQ(1 + i, SortOn(records[i])) << "condition " << i;
		EXPECT_EQ(3 + i, UInt(records[i], kValueOffset, 4)) << "dxfId of condition " << i;
		EXPECT_EQ(0u, UInt(records[i], kValueOffset + 4, 4)) << "reserved of condition " << i;
	}
}

TEST(XlsbSortConditions, WritesColorSortWithoutDxfId)
{
	std::vector<std::string> records;
	ASSERT_TRUE(SortConditions("<sortCondition sortBy=\"cellColor\" ref=\"A2:A4\"/>"
							   "<sortCondition sortBy=\"fontColor\" ref=\"A2:A4\"/>",
							   records));
	ASSERT_EQ(2u, records.size());
	for (size_t i = 0; i < records.size(); ++i)
	{
		ASSERT_EQ(kRecordSize, records[i].size()) << "condition " << i;
		EXPECT_EQ(1 + i, SortOn(records[i])) << "condition " << i;
		EXPECT_EQ(std::string(8, '\0'), records[i].substr(kValueOffset, 8)) << "condition " << i;
	}
}
