/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> Editor.bin -> xlsx. x2t keeps the pivot parts as raw bytes on this
// path, so they must come back byte for byte; rebuilding them from the object
// model drops content it does not keep.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
	class XlsxEditorBinPivots : public ::testing::Test
	{
	public:
		static void SetUpTestSuite()
		{
			workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"simple2.xlsx";
			std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
			std::wstring result = workDir + FILE_SEPARATOR_STR + L"result.xlsx";

			// One x2t temp dir per step. x2t does not clean a temp dir it is given, and both steps use
			// <temp>/xlsx_unpacked: with a shared dir, step 2 would zip the source parts that step 1
			// unpacked there, so a pivot part dropped by the save step would still be in result.xlsx.
			std::wstring openTempDir = workDir + FILE_SEPARATOR_STR + L"open";
			std::wstring saveTempDir = workDir + FILE_SEPARATOR_STR + L"save";
			NSDirectory::CreateDirectory(openTempDir);
			NSDirectory::CreateDirectory(saveTempDir);

			if (!NSFile::CFileBinary::Exists(source))
			{
				error = "missing input " + U_TO_UTF8(source);
				return;
			}
			int code = ConvertFile(CreateParamsFile(source, bin, openTempDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET));
			if (code != 0)
			{
				error = "xlsx -> Editor.bin: x2t returned " + std::to_string(code);
				return;
			}
			code = ConvertFile(CreateParamsFile(bin, result, saveTempDir, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
												AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX));
			if (code != 0)
			{
				error = "Editor.bin -> xlsx: x2t returned " + std::to_string(code);
				return;
			}
			resultDir = workDir + FILE_SEPARATOR_STR + L"result";
			sourceDir = workDir + FILE_SEPARATOR_STR + L"source";
			if (!Unpack(result, resultDir, error))
				return;
			Unpack(source, sourceDir, error);
		}

		void SetUp() override
		{
			ASSERT_TRUE(error.empty()) << error;
		}

		// Compares one part of the round-tripped file with the source.
		static void ExpectSamePart(const std::wstring& part)
		{
			std::vector<char> actual, expected;
			ASSERT_TRUE(ReadPart(resultDir, part, actual)) << "missing in result: " << U_TO_UTF8(part);
			ASSERT_TRUE(ReadPart(sourceDir, part, expected)) << "missing in source: " << U_TO_UTF8(part);
			EXPECT_EQ(actual, expected) << U_TO_UTF8(part) << " changed in the round trip";
		}

	private:
		static bool ReadPart(const std::wstring& dir, const std::wstring& part, std::vector<char>& content)
		{
			std::ifstream file(std::filesystem::path(dir + FILE_SEPARATOR_STR + part), std::ios::binary);
			if (!file)
				return false;
			content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			return true;
		}

		static std::wstring workDir;
		static std::wstring resultDir;
		static std::wstring sourceDir;
		static std::string error;
	};

	std::wstring XlsxEditorBinPivots::workDir;
	std::wstring XlsxEditorBinPivots::resultDir;
	std::wstring XlsxEditorBinPivots::sourceDir;
	std::string XlsxEditorBinPivots::error;
}

TEST_F(XlsxEditorBinPivots, PivotTable)
{
	ExpectSamePart(L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml");
}

TEST_F(XlsxEditorBinPivots, PivotCacheDefinition)
{
	ExpectSamePart(L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheDefinition1.xml");
}

TEST_F(XlsxEditorBinPivots, PivotCacheRecords)
{
	ExpectSamePart(L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheRecords1.xml");
}
