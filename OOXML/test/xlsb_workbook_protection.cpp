/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xlsb: the password data of <workbookProtection> (BrtBookProtectionIso) and <fileSharing>
// (BrtFileSharingIso) in xl/workbook.bin. The hash and salt must be written as their bytes, with the
// algorithm name; they used to be written as a length with no bytes behind it and an empty name, so
// the protection could no longer be removed with the password.
// xlsb -> xlsx: the same values come back unchanged.

#include "common.h"

#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace
{
	const std::wstring kFmla =
		L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"fmla.xlsx";

	const unsigned kBrtFileSharingIso = 676;
	const unsigned kBrtBookProtectionIso = 677;

	// Bytes 0..63 and 100..115.
	const std::string kBookHash =
		"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==";
	const std::string kBookSalt = "ZGVmZ2hpamtsbW5vcHFycw==";
	// Bytes 128..191 and 200..215.
	const std::string kSharingHash =
		"gIGCg4SFhoeIiYqLjI2Oj5CRkpOUlZaXmJmam5ydnp+goaKjpKWmp6ipqqusra6vsLGys7S1tre4ubq7vL2+vw==";
	const std::string kSharingSalt = "yMnKy8zNzs/Q0dLT1NXW1w==";

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

	// The fixture with a password-protected workbook structure and a password for write access.
	bool ProtectedXlsx(std::wstring& xlsx)
	{
		const std::string replacement =
			"<fileSharing userName=\"Author\" algorithmName=\"SHA-512\" hashValue=\"" + kSharingHash +
			"\" saltValue=\"" + kSharingSalt + "\" spinCount=\"100000\"/>"
			"<workbookPr defaultThemeVersion=\"166925\"/>"
			"<workbookProtection workbookAlgorithmName=\"SHA-512\" workbookHashValue=\"" + kBookHash +
			"\" workbookSaltValue=\"" + kBookSalt + "\" workbookSpinCount=\"100000\" lockStructure=\"1\"/>";
		return EditXlsxPart(kFmla, L"xl/workbook.xml", "<workbookPr defaultThemeVersion=\"166925\"/>",
							replacement, xlsx);
	}

	// Converts the protected fixture to xlsb; returns its path and xl/workbook.bin.
	bool ProtectedXlsb(std::wstring& xlsb, std::string& workbookBin)
	{
		std::wstring xlsx;
		if (!ProtectedXlsx(xlsx))
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
		workbookBin = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"workbook.bin");
		return !workbookBin.empty();
	}

	// The start tag of the first `element` in `xml`, or "".
	std::string StartTag(const std::string& xml, const std::string& element)
	{
		const size_t begin = xml.find("<" + element + " ");
		if (begin == std::string::npos)
			return "";
		const size_t end = xml.find('>', begin);
		return end == std::string::npos ? "" : xml.substr(begin, end + 1 - begin);
	}
}

TEST(XlsbWorkbookProtection, WritesBookPasswordData)
{
	std::wstring xlsb;
	std::string workbookBin, payload;
	ASSERT_TRUE(ProtectedXlsb(xlsb, workbookBin));
	ASSERT_TRUE(FindRecord(workbookBin, kBrtBookProtectionIso, payload));

	Reader reader(payload);
	EXPECT_EQ(100000u, reader.UInt(4)); // dwBookSpinCount
	reader.UInt(4);                     // dwRevSpinCount
	EXPECT_EQ(1u, reader.UInt(2) & 1);  // fLockStructure
	const PasswordData book = ReadPasswordData(reader);
	const PasswordData revisions = ReadPasswordData(reader);
	ASSERT_TRUE(reader.ok) << "a length in BrtBookProtectionIso runs past the record";
	EXPECT_EQ(payload.size(), reader.pos) << "BrtBookProtectionIso has bytes after the revision password data";

	EXPECT_EQ(Bytes(0, 64), book.hash);
	EXPECT_EQ(Bytes(100, 16), book.salt);
	EXPECT_EQ("SHA-512", book.algorithm);
	EXPECT_TRUE(revisions.hash.empty());
	EXPECT_TRUE(revisions.salt.empty());
}

TEST(XlsbWorkbookProtection, WritesFileSharingPasswordData)
{
	std::wstring xlsb;
	std::string workbookBin, payload;
	ASSERT_TRUE(ProtectedXlsb(xlsb, workbookBin));
	ASSERT_TRUE(FindRecord(workbookBin, kBrtFileSharingIso, payload));

	Reader reader(payload);
	EXPECT_EQ(100000u, reader.UInt(4)); // dwSpinCount
	reader.UInt(2);                     // fReadOnlyRec
	EXPECT_EQ("Author", reader.WideString());
	const PasswordData sharing = ReadPasswordData(reader);
	ASSERT_TRUE(reader.ok) << "a length in BrtFileSharingIso runs past the record";
	EXPECT_EQ(payload.size(), reader.pos) << "BrtFileSharingIso has bytes after the password data";

	EXPECT_EQ(Bytes(128, 64), sharing.hash);
	EXPECT_EQ(Bytes(200, 16), sharing.salt);
	EXPECT_EQ("SHA-512", sharing.algorithm);
}

TEST(XlsbWorkbookProtection, RoundTripKeepsPasswordData)
{
	std::wstring xlsb;
	std::string workbookBin;
	ASSERT_TRUE(ProtectedXlsb(xlsb, workbookBin));

	const std::wstring work = GetWorkDir();
	const std::wstring xlsx = work + FILE_SEPARATOR_STR + L"result.xlsx";
	std::string error;
	ASSERT_TRUE(ConvertTo(xlsb, xlsx, work, error)) << error;
	const std::wstring unpacked = work + FILE_SEPARATOR_STR + L"unpacked";
	ASSERT_TRUE(Unpack(xlsx, unpacked, error)) << error;
	const std::string workbookXml =
		ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"workbook.xml");

	const std::string protection = StartTag(workbookXml, "workbookProtection");
	ASSERT_FALSE(protection.empty());
	EXPECT_EQ("SHA-512", XmlAttribute(protection, "workbookAlgorithmName"));
	EXPECT_EQ(kBookHash, XmlAttribute(protection, "workbookHashValue"));
	EXPECT_EQ(kBookSalt, XmlAttribute(protection, "workbookSaltValue"));
	EXPECT_EQ("100000", XmlAttribute(protection, "workbookSpinCount"));
	EXPECT_EQ("1", XmlAttribute(protection, "lockStructure"));

	const std::string sharing = StartTag(workbookXml, "fileSharing");
	ASSERT_FALSE(sharing.empty());
	EXPECT_EQ("SHA-512", XmlAttribute(sharing, "algorithmName"));
	EXPECT_EQ(kSharingHash, XmlAttribute(sharing, "hashValue"));
	EXPECT_EQ(kSharingSalt, XmlAttribute(sharing, "saltValue"));
}
