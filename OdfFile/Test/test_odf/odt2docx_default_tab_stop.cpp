// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of the default tab distance: w:defaultTabStop in
// word/settings.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <ostream>
#include <string>

namespace
{
// The default tab distance (style:tab-stop-distance of the default paragraph
// style) becomes w:defaultTabStop, whose w:val is ST_TwipsMeasure: an unsigned
// integer number of twips, not a decimal.

struct TabStopCase
{
    const wchar_t* fixture;
    const wchar_t* twips;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const TabStopCase& c, std::ostream* os)
{
    *os << c.name;
}

class OdtToDocxDefaultTabStop : public ::testing::TestWithParam<TabStopCase>
{
};

TEST_P(OdtToDocxDefaultTabStop, WritesWholeTwips)
{
    const TabStopCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring settings = read_part(dir, L"word/settings.xml");
    ASSERT_FALSE(settings.empty()) << "word/settings.xml is missing";

    const size_t at = settings.find(L"<w:defaultTabStop ");
    ASSERT_NE(at, std::wstring::npos) << "no w:defaultTabStop";
    const std::wstring element = settings.substr(at, settings.find(L">", at) + 1 - at);
    EXPECT_EQ(element, L"<w:defaultTabStop w:val=\"" + std::wstring(c.twips) + L"\"/>");
}

const TabStopCase kTabStopCases[] = {
    { L"default_tab_stop_2cm.odt", L"1134", "TwoCentimetres" },
    { L"default_tab_stop_1251cm.odt", L"709", "LibreOfficeDefault" },
};

std::string TabStopCaseName(const ::testing::TestParamInfo<TabStopCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(DefaultTabStop, OdtToDocxDefaultTabStop, ::testing::ValuesIn(kTabStopCases), TabStopCaseName);
} // namespace
