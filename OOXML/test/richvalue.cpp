/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// Round-trips an XLSX with rich values through Editor.bin (XLSX -> bin -> XLSX),
// as the editor does on open and save.
//
// Excel stores the cached value of a rich value (e.g. the error behind a #NUM! cell)
// as <fb> in xl/richData/rdrichvalue.xml, before its <v> values. The bin carries it as
// c_oSer_RichValue::Fallback with FallbackValue and FallbackType.

#include "common.h"

#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <string>

namespace
{
	class RichValueRoundTrip : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			std::wstring workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2bin" FILE_SEPARATOR_STR L"richvalue-fallback.xlsx";
			std::wstring bin = workDir + FILE_SEPARATOR_STR L"Editor.bin";
			std::wstring xlsx = workDir + FILE_SEPARATOR_STR L"result.xlsx";
			std::wstring unpacked = workDir + FILE_SEPARATOR_STR L"result";

			// Each step gets its own temp dir, as in CheckboxRoundTrip.
			error.clear();
			if (ConvertTo(source, bin, GetWorkDir(), error) &&
				ConvertTo(bin, xlsx, GetWorkDir(), error) &&
				Unpack(xlsx, unpacked, error))
			{
				NSFile::CFileBinary::ReadAllTextUtf8A(
					unpacked + FILE_SEPARATOR_STR L"xl" FILE_SEPARATOR_STR L"richData" FILE_SEPARATOR_STR L"rdrichvalue.xml",
					richValues);
			}
		}

		static std::string error;
		static std::string richValues;
	};

	std::string RichValueRoundTrip::error;
	std::string RichValueRoundTrip::richValues;
}

TEST_F(RichValueRoundTrip, KeepsTheFallbackBeforeTheValues)
{
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_NE(richValues.find("<rv s=\"0\"><fb t=\"e\">#NUM!</fb><v>6</v><v>0</v></rv>"), std::string::npos) << richValues;
}

TEST_F(RichValueRoundTrip, EscapesTextFallbacks)
{
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_NE(richValues.find("<rv s=\"0\"><fb>a &amp; b</fb><v>7</v><v>0</v></rv>"), std::string::npos) << richValues;
}
