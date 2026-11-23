// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT conversion of w:doNotUseHTMLParagraphAutoSpacing: the
// AddParaTableSpacing setting in settings.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <ostream>
#include <string>

namespace
{
// An ODF document without AddParaTableSpacing adds the spacing below a
// paragraph and the spacing above the next one, which is not Word's default.
// So the setting must be written in both cases.

struct SpacingCase
{
    const wchar_t* fixture;
    const wchar_t* value;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const SpacingCase& c, std::ostream* os)
{
    *os << c.name;
}

class DocxToOdtAddParaTableSpacing : public ::testing::TestWithParam<SpacingCase>
{
};

TEST_P(DocxToOdtAddParaTableSpacing, WritesSetting)
{
    const SpacingCase& c = GetParam();

    const std::wstring dir = convert_ooxml_to_odf_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

    const std::wstring settings = read_part(dir, L"settings.xml");
    ASSERT_FALSE(settings.empty()) << "settings.xml is missing";

    const std::wstring name = L"config:name=\"AddParaTableSpacing\"";
    const size_t at = settings.find(name);
    ASSERT_NE(at, std::wstring::npos) << "no AddParaTableSpacing";
    EXPECT_EQ(settings.find(name, at + name.size()), std::wstring::npos) << "AddParaTableSpacing twice";

    const size_t start = settings.find(L'>', at);
    ASSERT_NE(start, std::wstring::npos);
    const size_t end = settings.find(L'<', start);
    ASSERT_NE(end, std::wstring::npos);
    EXPECT_EQ(settings.substr(start + 1, end - start - 1), c.value);
}

const SpacingCase kSpacingCases[] = {
    { L"paragraph_spacing_compat_flag.docx", L"true", "CompatFlag" },
    { L"paragraph_spacing_no_compat_flag.docx", L"false", "NoCompatFlag" },
    { L"paragraph_spacing_no_settings_part.docx", L"false", "NoSettingsPart" },
};

std::string SpacingCaseName(const ::testing::TestParamInfo<SpacingCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(AddParaTableSpacing, DocxToOdtAddParaTableSpacing, ::testing::ValuesIn(kSpacingCases),
                         SpacingCaseName);
} // namespace
