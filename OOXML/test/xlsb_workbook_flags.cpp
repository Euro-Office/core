/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xlsb: the flags of the calculation properties (BrtCalcProp) and the
// workbook properties (BrtWbProp) in xl/workbook.bin. A flag whose attribute
// is missing in the xlsx file must get the xlsx default; it used to be left
// unset, so the written bit depended on what was in memory before.
// xlsb -> xlsx: the thread count of BrtCalcProp becomes concurrentManualCount
// only when fUserSetThreadCount says the user chose it.
//
// To make an unset flag show up every time, the conversion runs with every
// new allocation filled with zeros (operator new below): an unset flag then
// reads as 0, while the defaults checked here (A1 references, ink annotations
// shown, pictures compressed) are 1.

#include "common.h"

#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <new>
#include <string>

namespace
{
	bool g_zeroNewMemory = false;

	struct ZeroNewMemory
	{
		ZeroNewMemory() { g_zeroNewMemory = true; }
		~ZeroNewMemory() { g_zeroNewMemory = false; }
	};
}

#ifndef _WIN32
// Replaces the global operator new of this test program. The converter library
// takes it from here too (it is linked dynamically). The array and nothrow
// forms call this one. operator delete is replaced as well, so that memory
// from malloc() is always released with free().
void* operator new(std::size_t size)
{
	void* memory = std::malloc(size ? size : 1);
	if (!memory)
		throw std::bad_alloc();
	if (g_zeroNewMemory)
		std::memset(memory, 0, size);
	return memory;
}

void operator delete(void* memory) noexcept
{
	std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
	std::free(memory);
}
#endif

namespace
{
	const std::wstring kFmla =
		L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"fmla.xlsx";

	// Saved by Excel with the default thread setting: cUserThreadCount 1, fUserSetThreadCount 0.
	const std::wstring kSimple1 =
		L"ExampleFiles" FILE_SEPARATOR_STR L"xlsb2xlsx" FILE_SEPARATOR_STR L"simple1.xlsb";

	const unsigned kBrtWbProp = 153;
	const unsigned kBrtCalcProp = 157;

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

	// The payload of the first record of this type, or false.
	bool FindRecord(const std::string& data, unsigned type, std::string& payload)
	{
		size_t pos = 0;
		while (pos < data.size())
		{
			unsigned recordType = 0, size = 0;
			if (!ReadVarUInt(data, pos, 2, recordType) || !ReadVarUInt(data, pos, 4, size) ||
				size > data.size() - pos)
				return false;
			if (recordType == type)
			{
				payload = data.substr(pos, size);
				return true;
			}
			pos += size;
		}
		return false;
	}

	uint32_t UInt(const std::string& data, size_t pos, size_t bytes)
	{
		uint32_t value = 0;
		for (size_t i = 0; i < bytes; ++i)
			value |= static_cast<uint32_t>(static_cast<unsigned char>(data[pos + i])) << (8 * i);
		return value;
	}

	bool Bit(uint32_t flags, int bit)
	{
		return (flags >> bit) & 1;
	}

	// Converts the fixture, with this <calcPr> if one is given, to xlsb and returns xl/workbook.bin
	// (and the path of the xlsb file in `xlsbPath`, if given).
	bool WorkbookBin(const std::string& calcPr, std::string& workbookBin, std::wstring* xlsbPath = nullptr)
	{
		std::wstring edited = kFmla;
		if (!calcPr.empty() &&
			!EditXlsxPart(kFmla, L"xl/workbook.xml", "<calcPr calcId=\"191029\"/>", calcPr, edited))
			return false;

		const std::wstring work = GetWorkDir();
		const std::wstring xlsb = work + FILE_SEPARATOR_STR + L"result.xlsb";
		std::string error;
		bool converted = false;
		{
			ZeroNewMemory zero;
			converted = ConvertTo(edited, xlsb, work, error);
		}
		if (!converted)
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
		workbookBin = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"workbook.bin");
		if (xlsbPath)
			*xlsbPath = xlsb;
		return !workbookBin.empty();
	}

	// Converts an xlsb file to xlsx and returns the <calcPr .../> element of xl/workbook.xml.
	bool CalcPrOfXlsx(const std::wstring& xlsb, std::string& calcPr)
	{
		const std::wstring work = GetWorkDir();
		const std::wstring xlsx = work + FILE_SEPARATOR_STR + L"result.xlsx";
		std::string error;
		if (!ConvertTo(xlsb, xlsx, work, error))
		{
			ADD_FAILURE() << error;
			return false;
		}
		const std::wstring unpacked = work + FILE_SEPARATOR_STR + L"unpacked";
		if (!Unpack(xlsx, unpacked, error))
		{
			ADD_FAILURE() << error;
			return false;
		}
		const std::string workbookXml =
			ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"workbook.xml");
		const size_t begin = workbookXml.find("<calcPr");
		if (begin == std::string::npos)
			return false;
		const size_t end = workbookXml.find('>', begin);
		if (end == std::string::npos)
			return false;
		calcPr = workbookXml.substr(begin, end + 1 - begin);
		return true;
	}
}

TEST(XlsbWorkbookFlags, MissingAttributesGetTheXlsxDefaults)
{
#ifndef _WIN32
	// The fixture's <workbookPr> has only defaultThemeVersion; <calcPr> has only calcId.
	std::string workbookBin;
	ASSERT_TRUE(WorkbookBin(std::string(), workbookBin));

	std::string calcProp;
	ASSERT_TRUE(FindRecord(workbookBin, kBrtCalcProp, calcProp));
	ASSERT_EQ(26u, calcProp.size());
	const uint32_t calcFlags = UInt(calcProp, 24, 2);
	EXPECT_TRUE(Bit(calcFlags, 1)) << "fRefA1: references are A1 unless refMode says R1C1";
	EXPECT_FALSE(Bit(calcFlags, 7)) << "fUserSetThreadCount: no concurrentManualCount";

	std::string wbProp;
	ASSERT_TRUE(FindRecord(workbookBin, kBrtWbProp, wbProp));
	ASSERT_LE(8u, wbProp.size());
	const uint32_t wbFlags = UInt(wbProp, 0, 4);
	EXPECT_TRUE(Bit(wbFlags, 5)) << "fShowInkAnnotation: showInkAnnotation defaults to true";
	EXPECT_TRUE(Bit(wbFlags, 16)) << "fAutoCompressPictures: autoCompressPictures defaults to true";
#else
	GTEST_SKIP() << "replaces operator new, which a DLL does not take from the program on Windows";
#endif
}

TEST(XlsbWorkbookFlags, CalcPrAttributesAreWritten)
{
#ifndef _WIN32
	std::string workbookBin;
	ASSERT_TRUE(
		WorkbookBin("<calcPr calcId=\"191029\" refMode=\"R1C1\" concurrentManualCount=\"4\"/>", workbookBin));

	std::string calcProp;
	ASSERT_TRUE(FindRecord(workbookBin, kBrtCalcProp, calcProp));
	ASSERT_EQ(26u, calcProp.size());
	EXPECT_EQ(4u, UInt(calcProp, 20, 4)) << "cUserThreadCount";
	const uint32_t calcFlags = UInt(calcProp, 24, 2);
	EXPECT_FALSE(Bit(calcFlags, 1)) << "fRefA1: refMode is R1C1";
	EXPECT_TRUE(Bit(calcFlags, 7)) << "fUserSetThreadCount: concurrentManualCount is set";
#else
	GTEST_SKIP() << "replaces operator new, which a DLL does not take from the program on Windows";
#endif
}

TEST(XlsbWorkbookFlags, ThreadCountIsReadOnlyWhenUserSet)
{
	std::string calcPr;
	ASSERT_TRUE(CalcPrOfXlsx(kSimple1, calcPr));
	EXPECT_EQ(std::string::npos, calcPr.find("concurrentManualCount"))
		<< "fUserSetThreadCount is 0, so the thread count is not the user's: " << calcPr;
}

TEST(XlsbWorkbookFlags, UserSetThreadCountSurvivesARoundTrip)
{
	std::string workbookBin;
	std::wstring xlsb;
	ASSERT_TRUE(WorkbookBin("<calcPr calcId=\"191029\" concurrentManualCount=\"4\"/>", workbookBin, &xlsb));

	std::string calcPr;
	ASSERT_TRUE(CalcPrOfXlsx(xlsb, calcPr));
	EXPECT_NE(std::string::npos, calcPr.find("concurrentManualCount=\"4\"")) << calcPr;
}
