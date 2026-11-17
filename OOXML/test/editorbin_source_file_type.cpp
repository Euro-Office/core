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

		// The params the document server sends when it opens a file: m_nFormatTo only, no m_nFormatFrom,
		// so x2t detects the source type and stores it back into the params
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

	// Converts the example file to Editor.bin given the file names only (no m_nFormatFrom, no
	// m_nFormatTo: the direction comes from the extensions) and returns its bytes. m_bIsNoBase64
	// defaults to true, so the file is not base64.
	std::vector<char> ConvertToEditorBinByFileNames(const std::wstring& name, std::string& error)
	{
		std::vector<char> content;
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR + name;
		std::wstring bin = GetWorkDir() + FILE_SEPARATOR_STR L"Editor.bin";
		if (!ConvertTo(source, bin, GetWorkDir(), error))
			return content;
		std::ifstream file(std::filesystem::path(bin), std::ios::binary);
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return content;
	}

	bool ReadBytes(const std::wstring& path, std::vector<char>& content)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		if (!file)
			return false;
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return true;
	}

	// One x2t run with a temp dir of its own: x2t does not clean a temp dir it is given, so a shared
	// one would let a later step zip the parts an earlier step unpacked there.
	bool Convert(const std::wstring& from, const std::wstring& to, int formatFrom, int formatTo, std::string& error)
	{
		int code = ConvertFile(CreateParamsFile(from, to, GetWorkDir(), formatFrom, formatTo));
		if (code != 0 || !NSFile::CFileBinary::Exists(to))
		{
			error = "x2t returned " + std::to_string(code) + " converting " + U_TO_UTF8(from) + " to " + U_TO_UTF8(to);
			return false;
		}
		return true;
	}

	// Saves the workbook of plain.ods to `ext` twice, each time from an Editor.bin as the editor sends
	// it back: once from a binary file with the SourceFileType record (ODS -> Editor.bin), once from
	// one without it (ODS -> XLSX -> Editor.bin). Both results are unpacked into withDir / withoutDir.
	bool SaveWithAndWithoutRecord(int formatTo, const std::wstring& ext, std::wstring& withDir,
								  std::wstring& withoutDir, std::string& error)
	{
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"ods2bin" FILE_SEPARATOR_STR L"plain.ods";
		if (!NSFile::CFileBinary::Exists(source))
		{
			error = "missing input " + U_TO_UTF8(source);
			return false;
		}
		std::wstring workDir = GetWorkDir();
		std::wstring binWith = workDir + FILE_SEPARATOR_STR L"with.bin";
		std::wstring xlsx = workDir + FILE_SEPARATOR_STR L"plain.xlsx";
		std::wstring binWithout = workDir + FILE_SEPARATOR_STR L"without.bin";
		std::wstring savedWith = workDir + FILE_SEPARATOR_STR L"with" + ext;
		std::wstring savedWithout = workDir + FILE_SEPARATOR_STR L"without" + ext;

		if (!Convert(source, binWith, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, error) ||
			!Convert(source, xlsx, 0, AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX, error) ||
			!Convert(xlsx, binWithout, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, error))
			return false;

		std::vector<char> bytesWith, bytesWithout;
		if (!ReadBytes(binWith, bytesWith) || !ReadBytes(binWithout, bytesWithout))
		{
			error = "cannot read the Editor.bin files";
			return false;
		}
		std::vector<char> record = SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS);
		if (!Contains(bytesWith, record) || Contains(bytesWithout, record))
		{
			error = "only the Editor.bin from ODS should carry the record";
			return false;
		}

		if (!Convert(binWith, savedWith, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, formatTo, error) ||
			!Convert(binWithout, savedWithout, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, formatTo, error))
			return false;

		withDir = workDir + FILE_SEPARATOR_STR L"with";
		withoutDir = workDir + FILE_SEPARATOR_STR L"without";
		return Unpack(savedWith, withDir, error) && Unpack(savedWithout, withoutDir, error);
	}

	void ExpectSamePart(const std::wstring& withDir, const std::wstring& withoutDir, const std::wstring& part)
	{
		std::vector<char> with, without;
		ASSERT_TRUE(ReadBytes(withDir + FILE_SEPARATOR_STR + part, with)) << "missing: " << U_TO_UTF8(part);
		ASSERT_TRUE(ReadBytes(withoutDir + FILE_SEPARATOR_STR + part, without)) << "missing: " << U_TO_UTF8(part);
		EXPECT_FALSE(with.empty()) << U_TO_UTF8(part);
		EXPECT_EQ(with, without) << U_TO_UTF8(part) << " differs when the Editor.bin carries the record";
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

// x2t given only the file names (x2t in.ods out.bin) detects the type in odf2oot_bin
TEST(EditorBinSourceFileType, OdsByFileNamesRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBinByFileNames(L"ods2bin" FILE_SEPARATOR_STR L"plain.ods", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS)));
}

TEST(EditorBinSourceFileType, OdsTemplateByFileNamesRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBinByFileNames(L"ods2bin" FILE_SEPARATOR_STR L"plain.ots", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_OTS)));
}

// The save direction: x2t's own reader skips the record, so saving a workbook whose Editor.bin
// carries it gives the same file as saving one without it. The Editor.bin here is x2t's own, where
// the record is the last workbook item; x2t has no code for the record and skips it by its length
// like any unknown item.
TEST(EditorBinSourceFileType, SaveToXlsxSkipsTheRecord)
{
	std::wstring withDir, withoutDir;
	std::string error;
	ASSERT_TRUE(SaveWithAndWithoutRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX, L".xlsx", withDir, withoutDir, error))
		<< error;
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"workbook.xml");
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet1.xml");
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"styles.xml");
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"drawings" FILE_SEPARATOR_STR L"drawing1.xml");
}

TEST(EditorBinSourceFileType, SaveToOdsSkipsTheRecord)
{
	std::wstring withDir, withoutDir;
	std::string error;
	ASSERT_TRUE(SaveWithAndWithoutRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS, L".ods", withDir, withoutDir, error))
		<< error;
	ExpectSamePart(withDir, withoutDir, L"content.xml");
	ExpectSamePart(withDir, withoutDir, L"styles.xml");
}
