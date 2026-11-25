/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xlsb -> xlsx for a pivot data field with showDataAs="runTotal".
// On this path x2t rebuilds the pivot table from the object model, so the
// attribute comes from CShowDataAs::ToString(); xlsx -> xlsx copies the part
// verbatim and would not catch a serializer bug.

#include "common.h"

#include "../../DesktopEditor/common/Directory.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
	class PivotShowDataAs : public ::testing::Test
	{
	public:
		static void SetUpTestSuite()
		{
			std::wstring workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"pivot_show_data_as" FILE_SEPARATOR_STR L"runTotal.xlsx";
			std::wstring xlsb = workDir + FILE_SEPARATOR_STR + L"runTotal.xlsb";
			std::wstring result = workDir + FILE_SEPARATOR_STR + L"result.xlsx";

			// One temp dir per step, as in editorbin_pivots.cpp: both steps use
			// <temp>/xlsx_unpacked (or xlsb_unpacked) and a shared dir would let
			// the second step write over the parts the first one unpacked.
			std::wstring toXlsbTempDir = workDir + FILE_SEPARATOR_STR + L"to_xlsb";
			std::wstring toXlsxTempDir = workDir + FILE_SEPARATOR_STR + L"to_xlsx";
			NSDirectory::CreateDirectory(toXlsbTempDir);
			NSDirectory::CreateDirectory(toXlsxTempDir);

			if (!ConvertTo(source, xlsb, toXlsbTempDir, error) ||
				!ConvertTo(xlsb, result, toXlsxTempDir, error))
				return;

			std::wstring unpacked = workDir + FILE_SEPARATOR_STR + L"result";
			if (!Unpack(result, unpacked, error))
				return;

			// The rebuilt package keeps one pivot table; read every part in case
			// the name changes.
			std::filesystem::path dir(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"pivotTables");
			for (const auto& entry : std::filesystem::directory_iterator(dir))
			{
				std::ifstream file(entry.path(), std::ios::binary);
				pivotTable.append(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			}
			if (pivotTable.empty())
				error = "no pivot table in the converted file";
		}

		void SetUp() override
		{
			ASSERT_TRUE(error.empty()) << error;
		}

		static std::string pivotTable;

	private:
		static std::string error;
	};

	std::string PivotShowDataAs::pivotTable;
	std::string PivotShowDataAs::error;
}

TEST_F(PivotShowDataAs, XlsbRoundTripKeepsRunTotal)
{
	EXPECT_NE(std::string::npos, pivotTable.find("showDataAs=\"runTotal\"")) << pivotTable;
	EXPECT_EQ(std::string::npos, pivotTable.find("showDataAs=\"percentOfTotal\"")) << pivotTable;
}
