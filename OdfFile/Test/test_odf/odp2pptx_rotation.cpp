// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Rotation of shapes written by the ODF -> DrawingML converter (oox_drawing.cpp).

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
// The values of all rot="..." attributes of a part, in document order.
std::vector<long long> rot_values(const std::wstring& xml)
{
    std::vector<long long> values;
    const std::wstring attr = L" rot=\"";
    for (size_t pos = xml.find(attr); pos != std::wstring::npos; pos = xml.find(attr, pos + 1))
    {
        values.push_back(std::stoll(xml.substr(pos + attr.size())));
    }
    return values;
}
}

// The fixture has rotate(0.3), rotate(-6), rotate(-6.283185307179586), one
// whole turn, and rotate(-6.2831853), one whole turn with 7 decimals.
// a:xfrm/@rot is in 0..21600000 (ST_Angle, 60000ths of a degree). Before,
// rotate(-6) gave 42226480 and the whole turn 43200000.
TEST(OdpToPptxShape, RotationIsInOneTurn)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/rotate_range.odp");
    ASSERT_FALSE(dir.empty()) << "ODP -> PPTX conversion failed";

    const std::wstring xml = read_part(dir, L"ppt/slides/slide1.xml");
    ASSERT_FALSE(xml.empty()) << "ppt/slides/slide1.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const std::vector<long long> rot = rot_values(xml);
    ASSERT_EQ(rot.size(), 4u) << "one rot per rotated shape";
    for (long long value : rot)
    {
        EXPECT_GE(value, 0);
        EXPECT_LT(value, 21600000);
    }
    EXPECT_EQ(rot[2], 0) << "a whole turn is no rotation";
}
