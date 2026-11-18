/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// CSVReader maps the code page from the client to an index of the encodings
// table, and converts UTF-8/16/32 by hand. The code page reaches the reader
// through the x2t parameters, so these tests convert a CSV with x2t.

#include "common.h"

#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <string>

namespace
{
	std::wstring WriteCsv(const std::string& bytes, const std::wstring& name)
	{
		std::wstring path = GetWorkDir() + FILE_SEPARATOR_STR + name;
		NSFile::CFileBinary oFile;
		oFile.CreateFile(path);
		oFile.WriteFile(bytes.data(), (DWORD)bytes.size());
		oFile.CloseFile();
		return path;
	}

	// UTF-16LE by hand, so a lone surrogate can be written.
	std::string Utf16Le(const std::wstring& text)
	{
		std::string bytes("\xFF\xFE", 2); // BOM
		for (size_t i = 0; i < text.size(); ++i)
		{
			bytes.push_back((char)(text[i] & 0xFF));
			bytes.push_back((char)((text[i] >> 8) & 0xFF));
		}
		return bytes;
	}

	// Rows x2t writes for a CSV imported with that code page; a sheet that
	// could not be converted has none.
	size_t ImportedRows(const std::wstring& csvPath, int codePage, std::string& error)
	{
		std::wstring workDir = GetWorkDir();
		std::wstring xlsx = workDir + FILE_SEPARATOR_STR + L"csv2xlsx.xlsx";
		if (!ConvertTo(csvPath, xlsx, workDir, error, codePage))
			return 0;

		std::wstring sheetDir = workDir + FILE_SEPARATOR_STR + L"csv2xlsx";
		if (!Unpack(xlsx, sheetDir, error))
			return 0;

		std::wstring sheetPath = sheetDir + FILE_SEPARATOR_STR + L"xl" + FILE_SEPARATOR_STR +
								 L"worksheets" + FILE_SEPARATOR_STR + L"sheet1.xml";
		std::string sheet;
		if (!NSFile::CFileBinary::ReadAllTextUtf8A(sheetPath, sheet))
		{
			error = "cannot read " + U_TO_UTF8(sheetPath);
			return 0;
		}

		size_t rows = 0;
		for (size_t found = sheet.find("<row"); found != std::string::npos; found = sheet.find("<row", found + 1))
			++rows;
		return rows;
	}
}

// 1000 is converted by ansi_2_unicode and has no entry in the encodings table,
// so it must not index it.
TEST(CsvImport, AnsiCodePageDoesNotIndexTheEncodingsTable)
{
	std::wstring csv = WriteCsv("a,b\r\n1,2\r\n", L"ansi.csv");
	std::string error;
	EXPECT_EQ((size_t)2, ImportedRows(csv, 1000, error)) << error;
}

// A lone surrogate makes the strict UTF-16 conversion fail; the ICU fallback
// has to convert the file instead of importing an empty sheet.
TEST(CsvImport, Utf16FallbackKeepsTheSheet)
{
	std::wstring text(L"a,b\r\n");
	text += L'\xD800'; // lone high surrogate
	text += L",1\r\n";

	std::wstring csv = WriteCsv(Utf16Le(text), L"utf16.csv");
	std::string error;
	EXPECT_EQ((size_t)2, ImportedRows(csv, 48 /*UTF-16LE*/, error)) << error;
}
