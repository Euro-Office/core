// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT conversion of w:contextualSpacing (no spacing between paragraphs
// of the same style): style:contextual-spacing must follow its w:val.

#include "gtest/gtest.h"

#include "common.h"

#include <ostream>
#include <string>

namespace
{
struct ContextualCase
{
    const wchar_t* fixture;
    bool           on;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const ContextualCase& c, std::ostream* os)
{
    *os << c.name;
}

class DocxToOdtContextualSpacing : public ::testing::TestWithParam<ContextualCase>
{
};

// Each fixture has one paragraph style with w:contextualSpacing, so the
// converted styles carry exactly one value.
TEST_P(DocxToOdtContextualSpacing, FollowsValue)
{
    const ContextualCase& c = GetParam();

    const std::wstring dir = convert_ooxml_to_odf_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

    const std::wstring styles = read_part(dir, L"styles.xml") + read_part(dir, L"content.xml");
    ASSERT_FALSE(styles.empty()) << "styles.xml and content.xml are missing";

    const bool hasTrue = styles.find(L"style:contextual-spacing=\"true\"") != std::wstring::npos;
    const bool hasFalse = styles.find(L"style:contextual-spacing=\"false\"") != std::wstring::npos;
    EXPECT_EQ(hasTrue, c.on);
    EXPECT_EQ(hasFalse, !c.on);
}

const ContextualCase kContextualCases[] = {
    { L"contextual_spacing_on.docx", true, "NoValue" },
    { L"contextual_spacing_val_1.docx", true, "ValueOne" },
    { L"contextual_spacing_val_0.docx", false, "ValueZero" },
    { L"contextual_spacing_val_false.docx", false, "ValueFalse" },
};

std::string ContextualCaseName(const ::testing::TestParamInfo<ContextualCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(ContextualSpacing, DocxToOdtContextualSpacing, ::testing::ValuesIn(kContextualCases),
                         ContextualCaseName);
} // namespace
