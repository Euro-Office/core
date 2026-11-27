/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsb -> xlsx: the cell ranges of a protected sheet (<protectedRanges>, "Allow edit ranges") are written
// back to the worksheet, after <sheetProtection>. The direct conversion used to drop all of them.

#include "common.h"

#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace
{
	const std::wstring kFmla =
		L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"fmla.xlsx";

	std::string ReadAll(const std::wstring& path)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
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

TEST(XlsbProtectedRangesToXlsx, KeepsRangesOfProtectedSheet)
{
	const std::string replacement = "</sheetData><sheetProtection sheet=\"1\" objects=\"1\" scenarios=\"1\"/>"
									 "<protectedRanges><protectedRange sqref=\"A1:B2\" name=\"Range1\"/>"
									 "<protectedRange sqref=\"D4:E5\" name=\"Range2\"/></protectedRanges><pageMargins";
	std::wstring xlsx;
	ASSERT_TRUE(EditXlsxPart(kFmla, L"xl/worksheets/sheet2.xml", "</sheetData><pageMargins", replacement, xlsx));

	// One work dir per conversion, so the second cannot pick up parts left by the first.
	const std::wstring toXlsb = GetWorkDir();
	const std::wstring toXlsx = GetWorkDir();
	const std::wstring xlsb = toXlsb + FILE_SEPARATOR_STR + L"result.xlsb";
	const std::wstring back = toXlsx + FILE_SEPARATOR_STR + L"result.xlsx";
	std::string error;
	ASSERT_TRUE(ConvertTo(xlsx, xlsb, toXlsb, error)) << error;
	ASSERT_TRUE(ConvertTo(xlsb, back, toXlsx, error)) << error;
	const std::wstring unpacked = toXlsx + FILE_SEPARATOR_STR + L"unpacked";
	ASSERT_TRUE(Unpack(back, unpacked, error)) << error;
	const std::string sheetXml = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR
										 L"worksheets" FILE_SEPARATOR_STR L"sheet2.xml");
	ASSERT_FALSE(sheetXml.empty());

	const std::string range1 = StartTag(sheetXml, "protectedRange", "Range1");
	const std::string range2 = StartTag(sheetXml, "protectedRange", "Range2");
	ASSERT_FALSE(range1.empty());
	ASSERT_FALSE(range2.empty());
	EXPECT_EQ("A1:B2", XmlAttribute(range1, "sqref"));
	EXPECT_EQ("D4:E5", XmlAttribute(range2, "sqref"));

	// Schema order: sheetProtection, protectedRanges, ..., pageMargins.
	const size_t protection = sheetXml.find("<sheetProtection");
	const size_t ranges = sheetXml.find("<protectedRanges>");
	const size_t margins = sheetXml.find("<pageMargins");
	ASSERT_NE(std::string::npos, protection);
	ASSERT_NE(std::string::npos, ranges);
	ASSERT_NE(std::string::npos, margins);
	EXPECT_LT(protection, ranges);
	EXPECT_LT(ranges, margins);
}
