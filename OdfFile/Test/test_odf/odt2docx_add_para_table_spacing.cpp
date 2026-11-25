// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of the AddParaTableSpacing document setting:
// w:doNotUseHTMLParagraphAutoSpacing in word/settings.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <ostream>
#include <string>

namespace
{
// AddParaTableSpacing true (the paragraph spacing below and the spacing above
// of the next paragraph add up) is Word's w:doNotUseHTMLParagraphAutoSpacing.
// A document without the setting uses the same rule, so only "false" leaves
// the flag out.

struct SpacingCase
{
    const wchar_t* fixture;
    bool           flag;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const SpacingCase& c, std::ostream* os)
{
    *os << c.name;
}

class OdtToDocxAddParaTableSpacing : public ::testing::TestWithParam<SpacingCase>
{
};

TEST_P(OdtToDocxAddParaTableSpacing, WritesCompatFlag)
{
    const SpacingCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring settings = read_part(dir, L"word/settings.xml");
    ASSERT_FALSE(settings.empty()) << "word/settings.xml is missing";

    const size_t compat = settings.find(L"<w:compat>");
    ASSERT_NE(compat, std::wstring::npos) << "no w:compat";
    const size_t compatEnd = settings.find(L"</w:compat>", compat);
    ASSERT_NE(compatEnd, std::wstring::npos) << "w:compat is not closed";
    const std::wstring element = settings.substr(compat, compatEnd - compat);

    EXPECT_EQ(element.find(L"<w:doNotUseHTMLParagraphAutoSpacing/>") != std::wstring::npos, c.flag);
}

const SpacingCase kSpacingCases[] = {
    { L"add_para_table_spacing_true.odt", true, "SettingTrue" },
    { L"add_para_table_spacing_false.odt", false, "SettingFalse" },
    { L"add_para_table_spacing_missing.odt", true, "NoSetting" },
    { L"add_para_table_spacing_other_setting.odt", true, "OtherSettingOnly" },
    { L"add_para_table_spacing_invalid_value.odt", false, "InvalidValue" },
};

std::string SpacingCaseName(const ::testing::TestParamInfo<SpacingCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(AddParaTableSpacing, OdtToDocxAddParaTableSpacing, ::testing::ValuesIn(kSpacingCases),
                         SpacingCaseName);
} // namespace
