/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// CParameterType is the parameterType attribute of xl/connections.xml.
// The file format value is "prompt"; this reader used to write "promt".

#include "../../OOXML/Common/SimpleTypes_Spreadsheet.h"

#include <gtest/gtest.h>

TEST(ParameterType, PromptIsWrittenAsPrompt)
{
	SimpleTypes::Spreadsheet::CParameterType oType;
	oType.FromString(L"prompt");
	EXPECT_EQ(std::wstring(L"prompt"), oType.ToString());
}

// Files written before the fix carry the misspelling, so it still reads as a
// prompt parameter.
TEST(ParameterType, OldMisspellingStillReadsAsPrompt)
{
	SimpleTypes::Spreadsheet::CParameterType oType;
	EXPECT_EQ(SimpleTypes::Spreadsheet::parameterPrompt, oType.FromString(L"promt"));
	EXPECT_EQ(SimpleTypes::Spreadsheet::parameterPrompt, oType.FromString(L"prompt"));
}

TEST(ParameterType, ValueAndCellAreWrittenAsRead)
{
	SimpleTypes::Spreadsheet::CParameterType oType;
	oType.FromString(L"value");
	EXPECT_EQ(std::wstring(L"value"), oType.ToString());
	oType.FromString(L"cell");
	EXPECT_EQ(std::wstring(L"cell"), oType.ToString());
}
