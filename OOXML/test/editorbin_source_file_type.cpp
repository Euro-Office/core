/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// spreadsheet -> Editor.bin: a workbook converted from an OpenDocument
// spreadsheet carries its source file type (workbook record SourceFileType),
// so the editor can apply the OpenDocument print rules. Other sources do not
// get the record.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace
{
	// c_oSerWorkbookTypes::SourceFileType, item length 4, then the file type as a little-endian LONG
	std::vector<char> SourceFileTypeRecord(int fileType)
	{
		std::vector<char> record = {100, 4, 0, 0, 0};
		for (int i = 0; i < 4; ++i)
			record.push_back(static_cast<char>((fileType >> (8 * i)) & 0xFF));
		return record;
	}

	bool Contains(const std::vector<char>& data, const std::vector<char>& part)
	{
		return std::search(data.begin(), data.end(), part.begin(), part.end()) != data.end();
	}

	// Converts the example file to Editor.bin (no base64) and returns its bytes.
	std::vector<char> ConvertToEditorBin(const std::wstring& name, std::string& error)
	{
		std::vector<char> content;
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR + name;
		if (!NSFile::CFileBinary::Exists(source))
		{
			error = "missing input " + U_TO_UTF8(source);
			return content;
		}
		std::wstring stem = std::filesystem::path(name).stem().wstring();
		std::wstring workDir = GetWorkDir() + FILE_SEPARATOR_STR + L"source_file_type_" + stem;
		std::wstring tempDir = workDir + FILE_SEPARATOR_STR + L"temp";
		std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
		NSDirectory::CreateDirectory(workDir);
		NSDirectory::CreateDirectory(tempDir);

		// No format from: x2t detects it, as for a file given by its name only
		int code = ConvertFile(CreateParamsFile(source, bin, tempDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET));
		if (code != 0)
		{
			error = U_TO_UTF8(name) + " -> Editor.bin: x2t returned " + std::to_string(code);
			return content;
		}
		std::ifstream file(std::filesystem::path(bin), std::ios::binary);
		if (!file)
		{
			error = "no Editor.bin for " + U_TO_UTF8(name);
			return content;
		}
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return content;
	}
}

TEST(EditorBinSourceFileType, OdsWorkbookRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"ods2bin" FILE_SEPARATOR_STR L"plain.ods", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS)));
}

TEST(EditorBinSourceFileType, FlatOdsWorkbookRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"ods2bin" FILE_SEPARATOR_STR L"plain.fods", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS_FLAT)));
}

TEST(EditorBinSourceFileType, OdsTemplateWorkbookRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"ods2bin" FILE_SEPARATOR_STR L"plain.ots", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_OTS)));
}

TEST(EditorBinSourceFileType, XlsxWorkbookHasNoSourceRecord)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"xlsx2xlsb" FILE_SEPARATOR_STR L"simple1.xlsx", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_FALSE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX)));
	EXPECT_FALSE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS)));
}
