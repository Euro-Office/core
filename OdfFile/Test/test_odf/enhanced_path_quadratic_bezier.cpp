// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Quadratic Bezier segments between DrawingML custom geometry (a:quadBezTo)
// and draw:enhanced-path (Q), in both directions.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// a:quadBezTo (control point, end point) is the enhanced-path command Q.
TEST(PptxToOdp, QuadraticBezierIsEnhancedPathQ)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/custom_geometry_quad_bezier.pptx");
    ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

    const std::wstring xml = read_part(dir, L"content.xml");
    ASSERT_FALSE(xml.empty()) << "content.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const std::wstring attr = L"draw:enhanced-path=\"";
    const size_t pos = xml.find(attr);
    ASSERT_NE(pos, std::wstring::npos) << "no draw:enhanced-path";
    const size_t begin = pos + attr.size();
    const std::wstring path = xml.substr(begin, xml.find(L'"', begin) - begin);

    EXPECT_NE(path.find(L"L 100 0 Q 50 100 0 50 C "), std::wstring::npos) << path;
}

// Q in the enhanced path of an OOXML-type custom shape (as written above, and
// by other ODF producers) is read back as a:quadBezTo.
TEST(OdpToPptx, EnhancedPathQIsQuadraticBezier)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/quad_bezier_enhanced_path.odp");
    ASSERT_FALSE(dir.empty()) << "ODP -> PPTX conversion failed";

    const std::wstring xml = read_part(dir, L"ppt/slides/slide1.xml");
    ASSERT_FALSE(xml.empty()) << "ppt/slides/slide1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"<a:quadBezTo><a:pt x=\"50\" y=\"100\"/><a:pt x=\"0\" y=\"50\"/></a:quadBezTo>"),
              std::wstring::npos) << xml;
}
