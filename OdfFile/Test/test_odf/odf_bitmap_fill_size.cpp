// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODP -> PPTX conversion of shapes filled with a bitmap: a stretched bitmap fills the whole shape whatever
// draw:fill-image-width/-height say, and for a bitmap drawn once a size of 0 (length or percent) means the
// bitmap's own size. Before, both sizes were turned into fillRect insets, so a size of 0 left an empty rectangle
// (the picture vanished) and a stretched bitmap with a size was drawn larger or smaller than the shape.

#include "gtest/gtest.h"
#include "common.h"

#include <string>

namespace
{
const std::wstring& slide()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/bitmap_fill_size.odp");
    static const std::wstring xml = dir.empty() ? std::wstring() : read_part(dir, L"ppt/slides/slide1.xml");
    return xml;
}

// The <p:sp> element of the shape with this name, or an empty string.
std::wstring shape(const std::wstring& name)
{
    const std::wstring& xml = slide();
    const size_t at = xml.find(L"name=\"" + name + L"\"");
    if (at == std::wstring::npos) return std::wstring();
    const size_t begin = xml.rfind(L"<p:sp>", at);
    const size_t end = xml.find(L"</p:sp>", at);
    if (begin == std::wstring::npos || end == std::wstring::npos) return std::wstring();
    return xml.substr(begin, end - begin);
}

std::wstring blip_fill(const std::wstring& name)
{
    const std::wstring sp = shape(name);
    const size_t begin = sp.find(L"<a:blipFill");
    const size_t end = sp.find(L"</a:blipFill>");
    if (begin == std::wstring::npos || end == std::wstring::npos) return std::wstring();
    return sp.substr(begin, end - begin);
}

// The blip fill without the value of r:embed, to compare the fills of two shapes.
std::wstring blip_fill_without_rel(const std::wstring& name)
{
    std::wstring fill = blip_fill(name);
    const size_t at = fill.find(L"r:embed=\"");
    if (at == std::wstring::npos) return fill;
    const size_t value = at + 9;
    const size_t end = fill.find(L'"', value);
    if (end != std::wstring::npos) fill.erase(value, end - value);
    return fill;
}
}

TEST(OdfBitmapFillSize, StretchedZeroSizeFillsTheShape)
{
    const std::wstring fill = blip_fill(L"StretchZero");
    ASSERT_FALSE(fill.empty()) << shape(L"StretchZero");
    EXPECT_NE(fill.find(L"<a:blip "), std::wstring::npos) << fill;
    EXPECT_NE(fill.find(L"<a:stretch><a:fillRect/></a:stretch>"), std::wstring::npos) << fill;
}

TEST(OdfBitmapFillSize, StretchedSizeIsIgnored)
{
    const std::wstring fill = blip_fill(L"StretchSized");
    ASSERT_FALSE(fill.empty()) << shape(L"StretchSized");
    EXPECT_NE(fill.find(L"<a:stretch><a:fillRect/></a:stretch>"), std::wstring::npos) << fill;
}

TEST(OdfBitmapFillSize, NoRepeatZeroSizeKeepsThePicture)
{
    const std::wstring fill = blip_fill(L"NoRepeatZero");
    ASSERT_FALSE(fill.empty()) << shape(L"NoRepeatZero");
    EXPECT_NE(fill.find(L"<a:blip "), std::wstring::npos) << fill;
    // Insets of 50 % on every side leave nothing of the picture.
    EXPECT_EQ(fill.find(L"l=\"50000\""), std::wstring::npos) << fill;
    EXPECT_EQ(fill.find(L"<a:fillRect l="), std::wstring::npos) << fill;
    // A size of 0 is the same as no size at all.
    EXPECT_EQ(blip_fill_without_rel(L"NoRepeatZero"), blip_fill_without_rel(L"NoRepeatNoSize"));
}

TEST(OdfBitmapFillSize, NoRepeatZeroPercentKeepsThePicture)
{
    const std::wstring fill = blip_fill(L"NoRepeatZeroPercent");
    ASSERT_FALSE(fill.empty()) << shape(L"NoRepeatZeroPercent");
    EXPECT_NE(fill.find(L"<a:blip "), std::wstring::npos) << fill;
    EXPECT_EQ(fill.find(L"l=\"50000\""), std::wstring::npos) << fill;
    EXPECT_EQ(fill.find(L"<a:fillRect l="), std::wstring::npos) << fill;
    EXPECT_EQ(blip_fill_without_rel(L"NoRepeatZeroPercent"), blip_fill_without_rel(L"NoRepeatNoSize"));
}

TEST(OdfBitmapFillSize, TiledSizeStillScalesTheTiles)
{
    // 2 cm tiles of an 8 px bitmap: 56.69 pt * 100 / 8 px * 4 / 3 = 944.8 %.
    const std::wstring fill = blip_fill(L"TileSized");
    ASSERT_FALSE(fill.empty()) << shape(L"TileSized");
    EXPECT_NE(fill.find(L"<a:tile tx=\"0\" ty=\"0\" sx=\"944822\" sy=\"944822\""), std::wstring::npos) << fill;
}

TEST(OdfBitmapFillSize, TiledZeroSizeKeepsTheBitmapSize)
{
    // The editor draws tile scale 0 at the bitmap's own size, and it is saved back to ODF as a size of 0.
    const std::wstring fill = blip_fill(L"TileZero");
    ASSERT_FALSE(fill.empty()) << shape(L"TileZero");
    EXPECT_NE(fill.find(L"<a:tile tx=\"0\" ty=\"0\" sx=\"0\" sy=\"0\""), std::wstring::npos) << fill;
}

TEST(OdfBitmapFillSize, TiledWithoutRepeatAttributeStillScalesTheTiles)
{
    // No style:repeat means a tile (the ODF default), so the size stays the tile size.
    const std::wstring fill = blip_fill(L"TileNoRepeatAttr");
    ASSERT_FALSE(fill.empty()) << shape(L"TileNoRepeatAttr");
    EXPECT_NE(fill.find(L"<a:tile tx=\"0\" ty=\"0\" sx=\"944822\" sy=\"944822\""), std::wstring::npos) << fill;
}
