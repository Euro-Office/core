/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xlsb with array constants in cell formulas. The XLSB writer parses
// the formula text again (PtgExtraArray::fromString); an element it did not
// recognise (a name such as "nan", or a blank before a value) never moved the
// parser on, so the conversion did not end.
//
// The xlsx -> xlsb leg runs in a death-test child with a time limit, so that a
// conversion that does not end fails this test instead of stopping the suite.
// The result is then read back with x2t (xlsb -> xlsx) and the formulas are
// compared.

#include "common.h"

#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

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

TEST(XlsbArrayConstant, ElementsThatAreNotOneConstantBecomeNameErrors)
{
#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
	// C5 is a name in an array, D5 has blanks around the values, E5 has a name in a 2x2 array.
	// F5 has a tab before a value, G5 two values in one element, H5 an empty element, I5 a number
	// followed by a name.
	// All arrays are square: reading an XLSB file back swaps the row and column count of other
	// shapes, which is a separate problem.
	std::wstring edited;
	ASSERT_TRUE(EditXlsxPart(kFmla, L"xl/worksheets/sheet1.xml", "<c r=\"C5\"><f>SUM\\(B1:D1\\)</f><v>9</v></c>",
							 "<c r=\"C5\"><f>{nan}</f></c><c r=\"D5\"><f>{ 1 , 2 ; 3 , 4 }</f></c>"
							 "<c r=\"E5\"><f>{1,abc;3,4}</f></c><c r=\"F5\"><f>{5,\t6;7,8}</f></c>"
							 "<c r=\"G5\"><f>{1 2,3;4,5}</f></c><c r=\"H5\"><f>{9,;8,7}</f></c>"
							 "<c r=\"I5\"><f>{1abc,2;3,6}</f></c>",
							 edited));

	const std::wstring work = GetWorkDir();
	const std::wstring xlsb = work + FILE_SEPARATOR_STR + L"result.xlsb";
	ASSERT_EXIT(
		{
			alarm(30);
			std::string error;
			_exit(ConvertTo(edited, xlsb, work, error) ? 0 : 1);
		},
		::testing::ExitedWithCode(0), "");

	const std::wstring xlsx = work + FILE_SEPARATOR_STR + L"back.xlsx";
	const std::wstring unpacked = work + FILE_SEPARATOR_STR + L"back";
	std::string error;
	ASSERT_TRUE(ConvertTo(xlsb, xlsx, work, error)) << error;
	ASSERT_TRUE(Unpack(xlsx, unpacked, error)) << error;

	const std::string sheet = ReadAll(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"worksheets" +
									  FILE_SEPARATOR_STR + L"sheet1.xml");
	EXPECT_NE(std::string::npos, sheet.find("<f>{#NAME?}</f>")) << sheet;
	EXPECT_NE(std::string::npos, sheet.find("<f>{1,2;3,4}</f>")) << sheet;
	EXPECT_NE(std::string::npos, sheet.find("<f>{1,#NAME?;3,4}</f>")) << sheet;
	EXPECT_NE(std::string::npos, sheet.find("<f>{5,6;7,8}</f>")) << sheet;
	EXPECT_NE(std::string::npos, sheet.find("<f>{#NAME?,3;4,5}</f>")) << sheet;
	EXPECT_NE(std::string::npos, sheet.find("<f>{9,#NAME?;8,7}</f>")) << sheet;
	EXPECT_NE(std::string::npos, sheet.find("<f>{#NAME?,2;3,6}</f>")) << sheet;
	// The formula before them is unchanged.
	EXPECT_NE(std::string::npos, sheet.find("<f>A1</f>")) << sheet;
#else
	GTEST_SKIP() << "needs death tests";
#endif
}
