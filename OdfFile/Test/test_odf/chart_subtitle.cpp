// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODF charts -> OOXML chart parts.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
    // xl/charts/chart1.xml of the converted fixture; fails the test if it is missing or broken.
    std::wstring convert_first_xlsx_chart(const std::wstring& fixture)
    {
        const std::wstring dir = convert_odf_to_ooxml_dir(fixture);
        EXPECT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";
        if (dir.empty())
            return L"";

        const std::wstring xml = read_part(dir, L"xl/charts/chart1.xml");
        EXPECT_FALSE(xml.empty()) << "xl/charts/chart1.xml is missing";
        EXPECT_TRUE(is_well_formed(xml));
        return xml;
    }
}

// chart:subtitle may be empty: its text:p is optional.
TEST(OdfToOoxmlCharts, EmptySubtitle)
{
    const std::wstring xml = convert_first_xlsx_chart(L"ExampleFiles/chart_empty_subtitle.ods");
    EXPECT_NE(xml.find(L"<a:t>Main</a:t>"), std::wstring::npos) << xml;
}

// A subtitle position may give svg:x without svg:y.
TEST(OdfToOoxmlCharts, SubtitleWithoutY)
{
    const std::wstring xml = convert_first_xlsx_chart(L"ExampleFiles/chart_subtitle_without_y.ods");
    EXPECT_NE(xml.find(L"<a:t>Main</a:t>"), std::wstring::npos) << xml;
    EXPECT_NE(xml.find(L"<a:t>Sub</a:t>"), std::wstring::npos) << xml;
}
