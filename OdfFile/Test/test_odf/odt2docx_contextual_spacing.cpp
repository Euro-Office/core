// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of style:contextual-spacing (no spacing between
// paragraphs of the same style): w:contextualSpacing in the paragraph style.

#include "gtest/gtest.h"

#include "common.h"

#include <ostream>
#include <string>

namespace
{
enum class Expected
{
    On,
    Off,
    Absent,
};

struct ContextualCase
{
    const wchar_t* fixture;
    Expected       expected;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const ContextualCase& c, std::ostream* os)
{
    *os << c.name;
}

class OdtToDocxContextualSpacing : public ::testing::TestWithParam<ContextualCase>
{
};

// Each fixture has one paragraph style "Ctx" with or without the attribute.
// "false" is written out too, so that it overrides a parent style.
TEST_P(OdtToDocxContextualSpacing, WritesElement)
{
    const ContextualCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring styles = read_part(dir, L"word/styles.xml");
    ASSERT_FALSE(styles.empty()) << "word/styles.xml is missing";
    EXPECT_TRUE(is_well_formed(styles));
    const std::wstring xml = styles + read_part(dir, L"word/document.xml");

    const bool on = xml.find(L"<w:contextualSpacing/>") != std::wstring::npos;
    const bool off = xml.find(L"<w:contextualSpacing w:val=\"0\"/>") != std::wstring::npos;
    const bool any = xml.find(L"<w:contextualSpacing") != std::wstring::npos;
    EXPECT_EQ(on, c.expected == Expected::On);
    EXPECT_EQ(off, c.expected == Expected::Off);
    EXPECT_EQ(any, c.expected != Expected::Absent);
}

const ContextualCase kContextualCases[] = {
    { L"contextual_spacing_true.odt", Expected::On, "True" },
    { L"contextual_spacing_false.odt", Expected::Off, "False" },
    { L"contextual_spacing_missing.odt", Expected::Absent, "Missing" },
    { L"contextual_spacing_loext_true.odt", Expected::On, "LoextTrue" },
    { L"contextual_spacing_loext_false.odt", Expected::Off, "LoextFalse" },
};

std::string ContextualCaseName(const ::testing::TestParamInfo<ContextualCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(ContextualSpacing, OdtToDocxContextualSpacing, ::testing::ValuesIn(kContextualCases),
                         ContextualCaseName);
} // namespace
