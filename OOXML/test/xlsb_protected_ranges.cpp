/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xlsb: the cell ranges of <protectedRanges>. A range with a password hash is written as a
// BrtRangeProtectionIso record with its spin count, hash, salt and algorithm name, followed by a
// BrtRangeProtection record with the same range and title; a range without a hash or without an
// algorithm name only as the second record. Writing a range with a password used to stop the conversion.
// xlsb -> Editor.bin -> xlsx (how the editor opens an xlsb file and saves it as xlsx): the password
// data comes back unchanged.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
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

	const unsigned kBrtRangeProtection = 536;
	const unsigned kBrtRangeProtectionIso = 680;

	// Bytes 0..63 and 16..31.
	const std::string kHash =
		"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==";
	const std::string kSalt = "EBESExQVFhcYGRobHB0eHw==";

	std::string Bytes(unsigned first, unsigned count)
	{
		std::string bytes;
		for (unsigned i = 0; i < count; ++i)
			bytes += static_cast<char>(first + i);
		return bytes;
	}

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

	struct Record
	{
		unsigned type;
		std::string payload;
	};

	// The records of these two types, in file order; false if the stream is cut short.
	bool RangeProtectionRecords(const std::string& data, std::vector<Record>& records)
	{
		size_t pos = 0;
		while (pos < data.size())
		{
			unsigned recordType = 0, size = 0;
			if (!ReadVarUInt(data, pos, 2, recordType) || !ReadVarUInt(data, pos, 4, size) ||
				size > data.size() - pos)
				return false;
			if (recordType == kBrtRangeProtectionIso || recordType == kBrtRangeProtection)
				records.push_back({recordType, data.substr(pos, size)});
			pos += size;
		}
		return true;
	}

	// Reads the fields of a record payload in order; any read past its end makes ok false.
	struct Reader
	{
		explicit Reader(const std::string& payload) : data(payload) {}

		const std::string& data;
		size_t pos = 0;
		bool ok = true;

		uint32_t UInt(size_t bytes)
		{
			if (!ok || bytes > data.size() - pos)
			{
				ok = false;
				return 0;
			}
			uint32_t value = 0;
			for (size_t i = 0; i < bytes; ++i)
				value |= static_cast<uint32_t>(static_cast<unsigned char>(data[pos + i])) << (8 * i);
			pos += bytes;
			return value;
		}
		// LPByteBuf: 4-byte length, then the bytes.
		std::string ByteBuf()
		{
			const uint32_t length = UInt(4);
			if (!ok || length > data.size() - pos)
			{
				ok = false;
				return "";
			}
			std::string bytes = data.substr(pos, length);
			pos += length;
			return bytes;
		}
		// XLWideString: 4-byte character count (0xFFFFFFFF for none), then UTF-16LE; ASCII only here.
		std::string WideString()
		{
			const uint32_t count = UInt(4);
			if (!ok || count == 0xFFFFFFFF)
				return "";
			if (count > (data.size() - pos) / 2)
			{
				ok = false;
				return "";
			}
			std::string text;
			for (uint32_t i = 0; i < count; ++i)
				text += data[pos + 2 * i];
			pos += 2 * count;
			return text;
		}
	};

	struct PasswordData
	{
		std::string hash, salt, algorithm;
	};

	PasswordData ReadPasswordData(Reader& reader)
	{
		PasswordData data;
		data.hash = reader.ByteBuf();
		data.salt = reader.ByteBuf();
		data.algorithm = reader.WideString();
		return data;
	}

	// The fixture with sheet protection and the given <protectedRange> elements on its second sheet.
	bool ProtectedXlsx(const std::string& ranges, std::wstring& xlsx)
	{
		const std::string replacement = "</sheetData><sheetProtection sheet=\"1\" objects=\"1\" scenarios=\"1\"/>"
										 "<protectedRanges>" + ranges + "</protectedRanges><pageMargins";
		return EditXlsxPart(kFmla, L"xl/worksheets/sheet2.xml", "</sheetData><pageMargins", replacement, xlsx);
	}

	// A range with a SHA-512 password and a range without one.
	const std::string kTwoRanges =
		"<protectedRange algorithmName=\"SHA-512\" hashValue=\"" + kHash + "\" saltValue=\"" + kSalt +
		"\" spinCount=\"100000\" sqref=\"A1:B2\" name=\"Range1\"/>"
		"<protectedRange sqref=\"D4\" name=\"Range2\"/>";

	// Converts the fixture with these ranges to xlsb; returns its path and xl/worksheets/sheet2.bin.
	bool ProtectedXlsb(const std::string& ranges, std::wstring& xlsb, std::string& sheetBin)
	{
		std::wstring xlsx;
		if (!ProtectedXlsx(ranges, xlsx))
			return false;
		const std::wstring work = GetWorkDir();
		xlsb = work + FILE_SEPARATOR_STR + L"result.xlsb";
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
		sheetBin = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR
						   L"sheet2.bin");
		return !sheetBin.empty();
	}

	// The start tag of the first `element` in `xml` that has the attribute name="`name`", or "".
	std::string StartTag(const std::string& xml, const std::string& element, const std::string& name)
	{
		size_t begin = 0;
		while ((begin = xml.find("<" + element + " ", begin)) != std::string::npos)
		{
			const size_t end = xml.find('>', begin);
			if (end == std::string::npos)
				return "";
			const std::string tag = xml.substr(begin, end + 1 - begin);
			if (XmlAttribute(tag, "name") == name)
				return tag;
			begin = end;
		}
		return "";
	}
}

TEST(XlsbProtectedRanges, WritesRangePasswordData)
{
	std::wstring xlsb;
	std::string sheetBin;
	ASSERT_TRUE(ProtectedXlsb(kTwoRanges, xlsb, sheetBin));
	std::vector<Record> records;
	ASSERT_TRUE(RangeProtectionRecords(sheetBin, records));
	ASSERT_EQ(3u, records.size());
	EXPECT_EQ(kBrtRangeProtectionIso, records[0].type);
	EXPECT_EQ(kBrtRangeProtection, records[1].type);
	EXPECT_EQ(kBrtRangeProtection, records[2].type);

	// BrtRangeProtectionIso: dwSpinCount, sqRfX, title, password data. The BrtRangeProtection after it has
	// protPwd 0 and the same sqRfX and title.
	const std::string& iso = records[0].payload;
	const std::string& plain = records[1].payload;
	ASSERT_GE(plain.size(), 2u);
	Reader reader(iso);
	EXPECT_EQ(100000u, reader.UInt(4)); // dwSpinCount
	ASSERT_GE(iso.size(), 4 + plain.size() - 2);
	EXPECT_EQ(plain.substr(2), iso.substr(4, plain.size() - 2)) << "the range or title differs between the records";
	reader.pos = 4 + plain.size() - 2;
	const PasswordData password = ReadPasswordData(reader);
	ASSERT_TRUE(reader.ok) << "a length in BrtRangeProtectionIso runs past the record";
	EXPECT_EQ(iso.size(), reader.pos) << "BrtRangeProtectionIso has bytes after the password data";
	EXPECT_EQ(Bytes(0, 64), password.hash);
	EXPECT_EQ(Bytes(16, 16), password.salt);
	EXPECT_EQ("SHA-512", password.algorithm);
	EXPECT_EQ(std::string(2, '\0'), plain.substr(0, 2)); // protPwd

	const std::string range2("R\0a\0n\0g\0e\0" "2\0", 12);
	EXPECT_NE(std::string::npos, records[2].payload.find(range2));
}

TEST(XlsbProtectedRanges, WritesRangeWithoutHashAsPlainRecord)
{
	std::wstring xlsb;
	std::string sheetBin;
	ASSERT_TRUE(ProtectedXlsb("<protectedRange spinCount=\"100000\" sqref=\"A1:B2\" name=\"Range1\"/>", xlsb,
							  sheetBin));
	std::vector<Record> records;
	ASSERT_TRUE(RangeProtectionRecords(sheetBin, records));
	ASSERT_EQ(1u, records.size());
	EXPECT_EQ(kBrtRangeProtection, records[0].type);
}

TEST(XlsbProtectedRanges, WritesEachPasswordRangeAsPair)
{
	const std::string range3 = "<protectedRange algorithmName=\"SHA-512\" hashValue=\"" + kHash + "\" saltValue=\"" +
							   kSalt + "\" spinCount=\"100000\" sqref=\"C3\" name=\"Range3\"/>";
	std::wstring xlsb;
	std::string sheetBin;
	ASSERT_TRUE(ProtectedXlsb(kTwoRanges + range3, xlsb, sheetBin));
	std::vector<Record> records;
	ASSERT_TRUE(RangeProtectionRecords(sheetBin, records));
	ASSERT_EQ(5u, records.size());
	// Range1 (password), Range2 (none), Range3 (password): each BrtRangeProtectionIso is directly followed by
	// the BrtRangeProtection of the same range.
	const unsigned types[] = {kBrtRangeProtectionIso, kBrtRangeProtection, kBrtRangeProtection,
							  kBrtRangeProtectionIso, kBrtRangeProtection};
	const char* names[] = {"Range1", "Range1", "Range2", "Range3", "Range3"};
	for (size_t i = 0; i < records.size(); ++i)
	{
		EXPECT_EQ(types[i], records[i].type) << "record " << i;
		std::string name;
		for (const char* c = names[i]; *c; ++c)
			name += std::string(1, *c) + '\0';
		EXPECT_NE(std::string::npos, records[i].payload.find(name)) << "record " << i << " is not " << names[i];
	}
}

TEST(XlsbProtectedRanges, WritesHashWithoutAlgorithmAsPlainRecord)
{
	// BrtRangeProtectionIso must name the algorithm of a hash.
	std::wstring xlsb;
	std::string sheetBin;
	ASSERT_TRUE(ProtectedXlsb("<protectedRange hashValue=\"" + kHash + "\" saltValue=\"" + kSalt +
								  "\" spinCount=\"100000\" sqref=\"A1:B2\" name=\"Range1\"/>",
							  xlsb, sheetBin));
	std::vector<Record> records;
	ASSERT_TRUE(RangeProtectionRecords(sheetBin, records));
	ASSERT_EQ(1u, records.size());
	EXPECT_EQ(kBrtRangeProtection, records[0].type);
}

TEST(XlsbProtectedRanges, RoundTripKeepsRangePasswordData)
{
	std::wstring xlsb;
	std::string sheetBin;
	ASSERT_TRUE(ProtectedXlsb(kTwoRanges, xlsb, sheetBin));

	// Formats as the document server passes them: from a .bin target alone x2t cannot tell that an xlsb
	// opens as a spreadsheet. Own temp dirs, so the save step cannot pick up parts unpacked by the open step.
	const std::wstring work = GetWorkDir();
	const std::wstring openTemp = work + FILE_SEPARATOR_STR + L"open";
	const std::wstring saveTemp = work + FILE_SEPARATOR_STR + L"save";
	NSDirectory::CreateDirectory(openTemp);
	NSDirectory::CreateDirectory(saveTemp);
	const std::wstring bin = work + FILE_SEPARATOR_STR + L"Editor.bin";
	const std::wstring xlsx = work + FILE_SEPARATOR_STR + L"result.xlsx";
	ASSERT_EQ(0, ConvertFile(CreateParamsFile(xlsb, bin, openTemp, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET)));
	ASSERT_EQ(0, ConvertFile(CreateParamsFile(bin, xlsx, saveTemp, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
											  AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX)));
	std::string error;
	const std::wstring unpacked = work + FILE_SEPARATOR_STR + L"roundtrip";
	ASSERT_TRUE(Unpack(xlsx, unpacked, error)) << error;
	const std::string sheetXml = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR
										 L"worksheets" FILE_SEPARATOR_STR L"sheet2.xml");

	const std::string range = StartTag(sheetXml, "protectedRange", "Range1");
	ASSERT_FALSE(range.empty());
	EXPECT_EQ("A1:B2", XmlAttribute(range, "sqref"));
	EXPECT_EQ("SHA-512", XmlAttribute(range, "algorithmName"));
	EXPECT_EQ(kHash, XmlAttribute(range, "hashValue"));
	EXPECT_EQ(kSalt, XmlAttribute(range, "saltValue"));
	EXPECT_EQ("100000", XmlAttribute(range, "spinCount"));
}
