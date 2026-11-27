/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsb -> Editor.bin -> xlsx: a protected sheet with one edit range that has a password and one that has none keeps
// both. In XLSB a range with a password is a BrtRangeProtectionIso followed by a BrtRangeProtection with the same
// cell reference and title; a range without a password is a BrtRangeProtection only. The reader used to keep only the
// BrtRangeProtectionIso records as soon as there was one, so the range without a password was lost.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/File.h"

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

	std::string ReadAll(const std::wstring& path)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	std::string Uint32(uint32_t value)
	{
		std::string bytes;
		for (int i = 0; i < 4; ++i)
			bytes += static_cast<char>((value >> (8 * i)) & 0xFF);
		return bytes;
	}

	// Length-prefixed byte run (IsoPasswordData rgbHash / rgbSalt): first, first + 1, ...
	std::string ByteRun(uint32_t count, unsigned char first)
	{
		std::string bytes = Uint32(count);
		for (uint32_t i = 0; i < count; ++i)
			bytes += static_cast<char>(first + i);
		return bytes;
	}

	// XLWideString: character count + UTF-16LE (ASCII only here)
	std::string WideString(const std::string& text)
	{
		std::string bytes = Uint32(static_cast<uint32_t>(text.size()));
		for (char c : text)
		{
			bytes += c;
			bytes += '\0';
		}
		return bytes;
	}

	// Inserts a BrtRangeProtectionIso (SHA-512, spin count 100000, hash 0x00..0x3F, salt 0x10..0x1F) in front of the
	// BrtRangeProtection of the range titled `title`, with that record's cell reference and title. That record then
	// is the BrtRangeProtection that follows every BrtRangeProtectionIso. Returns `sheet` unchanged if the record is
	// not there.
	std::string AddPassword(const std::string& sheet, const std::string& title)
	{
		// BrtRangeProtection with one cell rectangle: type 536 (0x98 0x04), size 42 = protpwd (2) + UncheckedSqRfX
		// (4 + 16) + rgchTitle + SDRel (4)
		const std::string header("\x98\x04\x2a", 3);
		const size_t titlePos = sheet.find(WideString(title));
		if (titlePos == std::string::npos || titlePos < header.size() + 2 + 20)
			return sheet;
		const size_t start = titlePos - 20 - 2 - header.size();
		if (sheet.compare(start, header.size(), header) != 0)
			return sheet;
		const std::string refAndTitle = sheet.substr(start + header.size() + 2, 20 + WideString(title).size() + 4);
		const std::string body = Uint32(100000) + refAndTitle + ByteRun(64, 0x00) + ByteRun(16, 0x10) +
								 WideString("SHA-512");
		// type 680 (0xA8 0x05), size as a 7-bit varint
		std::string iso("\xa8\x05", 2);
		iso += static_cast<char>(0x80 | (body.size() & 0x7F));
		iso += static_cast<char>(body.size() >> 7);
		return sheet.substr(0, start) + iso + body + sheet.substr(start);
	}

	// The start tags of all <protectedRange> elements in `xml`, in order.
	std::vector<std::string> ProtectedRanges(const std::string& xml)
	{
		std::vector<std::string> tags;
		size_t begin = 0;
		while ((begin = xml.find("<protectedRange ", begin)) != std::string::npos)
		{
			const size_t end = xml.find('>', begin);
			if (end == std::string::npos)
				break;
			tags.push_back(xml.substr(begin, end + 1 - begin));
			begin = end;
		}
		return tags;
	}

	// Protected sheet with Range1 (A1:B2) and Range2 (D4:E5) -> XLSB, a password for the ranges titled in `passwords`,
	// -> Editor.bin -> XLSX; returns the <protectedRange> start tags of the result.
	std::vector<std::string> RangesAfterRoundTrip(const std::vector<std::string>& passwords)
	{
		std::vector<std::string> ranges;
		const std::string replacement =
			"</sheetData><sheetProtection sheet=\"1\" objects=\"1\" scenarios=\"1\"/>"
			"<protectedRanges><protectedRange sqref=\"A1:B2\" name=\"Range1\"/>"
			"<protectedRange sqref=\"D4:E5\" name=\"Range2\"/></protectedRanges><pageMargins";
		std::wstring xlsx;
		if (!EditXlsxPart(kFmla, L"xl/worksheets/sheet2.xml", "</sheetData><pageMargins", replacement, xlsx))
			return ranges;

		// One work dir per conversion, so a later one cannot pick up parts left by an earlier one.
		const std::wstring toXlsb = GetWorkDir();
		const std::wstring xlsb = toXlsb + FILE_SEPARATOR_STR + L"result.xlsb";
		std::string error;
		if (!ConvertTo(xlsx, xlsb, toXlsb, error))
		{
			ADD_FAILURE() << error;
			return ranges;
		}
		std::wstring edited;
		if (!TransformXlsxPart(xlsb, L"xl/worksheets/sheet2.bin", [&](const std::string& sheet)
			{
				std::string result = sheet;
				for (const std::string& title : passwords)
				{
					const std::string withPassword = AddPassword(result, title);
					EXPECT_NE(result, withPassword) << "no BrtRangeProtection for " << title;
					result = withPassword;
				}
				return result;
			}, edited))
			return ranges;
		const std::wstring mixed = toXlsb + FILE_SEPARATOR_STR + L"mixed.xlsb";
		if (!NSFile::CFileBinary::Copy(edited, mixed))
		{
			ADD_FAILURE() << "cannot copy the edited XLSB";
			return ranges;
		}

		const std::wstring openDir = GetWorkDir();
		const std::wstring saveDir = GetWorkDir();
		const std::wstring bin = openDir + FILE_SEPARATOR_STR + L"Editor.bin";
		const std::wstring back = saveDir + FILE_SEPARATOR_STR + L"result.xlsx";
		int code = ConvertFile(CreateParamsFile(mixed, bin, openDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET));
		if (code != 0)
		{
			ADD_FAILURE() << "xlsb -> Editor.bin: x2t returned " << code;
			return ranges;
		}
		code = ConvertFile(CreateParamsFile(bin, back, saveDir, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
											AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX));
		if (code != 0)
		{
			ADD_FAILURE() << "Editor.bin -> xlsx: x2t returned " << code;
			return ranges;
		}
		const std::wstring unpacked = saveDir + FILE_SEPARATOR_STR + L"unpacked";
		if (!Unpack(back, unpacked, error))
		{
			ADD_FAILURE() << error;
			return ranges;
		}
		return ProtectedRanges(ReadAll(
			unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet2.xml"));
	}

	const std::string kHash =
		"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==";
}

TEST(XlsbMixedProtectedRanges, KeepsRangesWithAndWithoutPassword)
{
	const std::vector<std::string> ranges = RangesAfterRoundTrip({"Range1"});

	// Both ranges, Range1 once (not again from the BrtRangeProtection after its BrtRangeProtectionIso) and with its
	// password.
	ASSERT_EQ(2u, ranges.size());
	EXPECT_EQ("Range1", XmlAttribute(ranges[0], "name"));
	EXPECT_EQ("A1:B2", XmlAttribute(ranges[0], "sqref"));
	EXPECT_EQ("SHA-512", XmlAttribute(ranges[0], "algorithmName"));
	EXPECT_EQ(kHash, XmlAttribute(ranges[0], "hashValue"));
	EXPECT_EQ("EBESExQVFhcYGRobHB0eHw==", XmlAttribute(ranges[0], "saltValue"));
	EXPECT_EQ("Range2", XmlAttribute(ranges[1], "name"));
	EXPECT_EQ("D4:E5", XmlAttribute(ranges[1], "sqref"));
	EXPECT_EQ("", XmlAttribute(ranges[1], "hashValue"));
}

TEST(XlsbMixedProtectedRanges, KeepsOrderOfRangesWithPassword)
{
	const std::vector<std::string> ranges = RangesAfterRoundTrip({"Range1", "Range2"});

	ASSERT_EQ(2u, ranges.size());
	EXPECT_EQ("Range1", XmlAttribute(ranges[0], "name"));
	EXPECT_EQ("A1:B2", XmlAttribute(ranges[0], "sqref"));
	EXPECT_EQ(kHash, XmlAttribute(ranges[0], "hashValue"));
	EXPECT_EQ("Range2", XmlAttribute(ranges[1], "name"));
	EXPECT_EQ("D4:E5", XmlAttribute(ranges[1], "sqref"));
	EXPECT_EQ(kHash, XmlAttribute(ranges[1], "hashValue"));
}

TEST(XlsbMixedProtectedRanges, KeepsPasswordOfSecondRange)
{
	const std::vector<std::string> ranges = RangesAfterRoundTrip({"Range2"});

	// The password belongs to Range2 only; Range1 keeps its place in front of it.
	ASSERT_EQ(2u, ranges.size());
	EXPECT_EQ("Range1", XmlAttribute(ranges[0], "name"));
	EXPECT_EQ("A1:B2", XmlAttribute(ranges[0], "sqref"));
	EXPECT_EQ("", XmlAttribute(ranges[0], "hashValue"));
	EXPECT_EQ("Range2", XmlAttribute(ranges[1], "name"));
	EXPECT_EQ("D4:E5", XmlAttribute(ranges[1], "sqref"));
	EXPECT_EQ(kHash, XmlAttribute(ranges[1], "hashValue"));
	EXPECT_EQ("100000", XmlAttribute(ranges[1], "spinCount"));
}
