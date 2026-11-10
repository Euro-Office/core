// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Paragraph properties of shape text written by the ODF -> DrawingML converter
// (style_paragraph_properties_pptx.cpp).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// fo:line-height="normal" is single spacing.
TEST(OdpToPptxText, LineHeightNormalIsSingleSpacing)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/line_height_normal.odp");
    ASSERT_FALSE(dir.empty()) << "ODP -> PPTX conversion failed";

    const std::wstring xml = read_part(dir, L"ppt/slides/slide1.xml");
    ASSERT_FALSE(xml.empty()) << "ppt/slides/slide1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"<a:lnSpc><a:spcPct val=\"100000\"/></a:lnSpc>"), std::wstring::npos);
    EXPECT_NE(xml.find(L"Normal line height"), std::wstring::npos);
}

// The same paragraph style on a shape in a spreadsheet goes through the
// XLSX drawing writer.
TEST(OdsToXlsxText, LineHeightNormalIsSingleSpacing)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/line_height_normal.ods");
    ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";

    const std::wstring xml = read_part(dir, L"xl/drawings/drawing1.xml");
    ASSERT_FALSE(xml.empty()) << "xl/drawings/drawing1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"<a:lnSpc><a:spcPct val=\"100000\"/></a:lnSpc>"), std::wstring::npos);
    EXPECT_NE(xml.find(L"Normal line height"), std::wstring::npos);
}

// A value that is neither normal, a percentage nor a length (here "auto")
// writes no a:lnSpc at all.
TEST(OdpToPptxText, OtherLineHeightWritesNoLineSpacing)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/line_height_auto.odp");
    ASSERT_FALSE(dir.empty()) << "ODP -> PPTX conversion failed";

    const std::wstring xml = read_part(dir, L"ppt/slides/slide1.xml");
    ASSERT_FALSE(xml.empty()) << "ppt/slides/slide1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_EQ(xml.find(L"<a:lnSpc>"), std::wstring::npos);
    EXPECT_NE(xml.find(L"Auto line height"), std::wstring::npos);
}
