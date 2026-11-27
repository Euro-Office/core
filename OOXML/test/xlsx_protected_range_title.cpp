/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> Editor.bin -> xlsx: the title of an edit range of a protected sheet (<protectedRange name>) is written as an
// escaped XML attribute. A title with & or < used to make the sheet XML unreadable.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/File.h"
#include "../../DesktopEditor/xml/include/xmlutils.h"

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
}

TEST(XlsxProtectedRangeTitle, EscapesTitle)
{
	const std::string replacement =
		"</sheetData><sheetProtection sheet=\"1\" objects=\"1\" scenarios=\"1\"/>"
		"<protectedRanges><protectedRange sqref=\"A1:B2\" name=\"Q&amp;A &lt;1&gt;\"/></protectedRanges><pageMargins";
	std::wstring xlsx;
	ASSERT_TRUE(EditXlsxPart(kFmla, L"xl/worksheets/sheet2.xml", "</sheetData><pageMargins", replacement, xlsx));

	// One work dir per conversion, so the second cannot pick up parts left by the first.
	const std::wstring openDir = GetWorkDir();
	const std::wstring saveDir = GetWorkDir();
	const std::wstring bin = openDir + FILE_SEPARATOR_STR + L"Editor.bin";
	const std::wstring back = saveDir + FILE_SEPARATOR_STR + L"result.xlsx";
	ASSERT_EQ(0, ConvertFile(CreateParamsFile(xlsx, bin, openDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET)));
	ASSERT_EQ(0, ConvertFile(CreateParamsFile(bin, back, saveDir, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
											  AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX)));
	const std::wstring unpacked = saveDir + FILE_SEPARATOR_STR + L"unpacked";
	std::string error;
	ASSERT_TRUE(Unpack(back, unpacked, error)) << error;
	const std::wstring sheet =
		unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet2.xml";

	EXPECT_NE(std::string::npos, ReadAll(sheet).find("name=\"Q&amp;A &lt;1&gt;\""));
	// FromXmlFile keeps the nodes read before a parse error and still returns true, so check the parsed title and an
	// element after the edit ranges instead.
	XmlUtils::CXmlNode root;
	ASSERT_TRUE(root.FromXmlFile(sheet));
	XmlUtils::CXmlNode range = root.ReadNodeNoNS(L"protectedRanges").ReadNodeNoNS(L"protectedRange");
	EXPECT_EQ(L"Q&A <1>", range.GetAttribute(L"name"));
	EXPECT_TRUE(root.ReadNodeNoNS(L"pageMargins").IsValid()) << "sheet2.xml is not read past the edit ranges";
}
