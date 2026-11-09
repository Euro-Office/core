// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// PPTX -> ODP: bullet sizes in list level styles.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// a:buSzPts is in hundredths of a point: val="1400" is a 14 pt bullet. It
// used to be taken as points (fo:font-size="1400pt").
TEST(PptxToOdp, BulletSizePointsIsHundredthsOfAPoint)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/bullet_size_points.pptx");
    ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

    const std::wstring xml = read_part(dir, L"content.xml");
    ASSERT_FALSE(xml.empty()) << "content.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const size_t begin = xml.find(L"<text:list-level-style-bullet");
    ASSERT_NE(begin, std::wstring::npos) << "no bullet list level style";
    const std::wstring level = xml.substr(begin, xml.find(L"</text:list-level-style-bullet>", begin) - begin);

    EXPECT_NE(level.find(L"fo:font-size=\"14pt\""), std::wstring::npos) << level;
    EXPECT_EQ(level.find(L"1400pt"), std::wstring::npos) << level;
}

// The same size on a picture bullet sets the image size: 14 pt = 0.4939 cm.
TEST(PptxToOdp, PictureBulletSizePointsIsHundredthsOfAPoint)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/bullet_size_points_picture.pptx");
    ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

    const std::wstring xml = read_part(dir, L"content.xml");
    ASSERT_FALSE(xml.empty()) << "content.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));

    const size_t begin = xml.find(L"<text:list-level-style-image");
    ASSERT_NE(begin, std::wstring::npos) << "no picture bullet list level style";
    const std::wstring level = xml.substr(begin, xml.find(L"</text:list-level-style-image>", begin) - begin);

    EXPECT_NE(level.find(L"fo:width=\"0.4939cm\""), std::wstring::npos) << level;
    EXPECT_NE(level.find(L"fo:height=\"0.4939cm\""), std::wstring::npos) << level;
}
