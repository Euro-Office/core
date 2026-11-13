// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// StarView metafile (SVM) map modes, as LibreOffice writes them for charts: a relative map mode
// scales the current one, and POP returns to the state of the matching PUSH. The fixture fills
// the picture blue, then, inside PUSH/POP, draws a red rectangle of the same size at half scale
// (the top-left quarter), and after the POP a green rectangle in the bottom-right corner at full
// scale. The PNG the DOCX gets must show the three colours there.

#include "gtest/gtest.h"

#include "common.h"

#include "../../../DesktopEditor/raster/BgraFrame.h"

#include <cstdlib>
#include <string>

namespace
{
struct Rgb
{
    int r, g, b;
};

// The colour at a fraction of the picture's width and height; -1 values if it can't be read.
Rgb colour_at(CBgraFrame& frame, double fx, double fy)
{
    const int x = static_cast<int>(frame.get_Width() * fx);
    const int y = static_cast<int>(frame.get_Height() * fy);
    BYTE* data = frame.get_Data();
    if (!data || x >= frame.get_Width() || y >= frame.get_Height())
        return {-1, -1, -1};
    // A negative stride means the rows are stored bottom-up.
    const int stride = frame.get_Stride() < 0 ? -frame.get_Stride() : frame.get_Stride();
    const int row = frame.get_Stride() < 0 ? frame.get_Height() - 1 - y : y;
    const BYTE* pixel = data + row * stride + x * 4;
    return frame.get_IsRGBA() ? Rgb{pixel[0], pixel[1], pixel[2]} : Rgb{pixel[2], pixel[1], pixel[0]};
}

bool is(const Rgb& c, int r, int g, int b)
{
    return std::abs(c.r - r) < 40 && std::abs(c.g - g) < 40 && std::abs(c.b - b) < 40;
}
} // namespace

TEST(OdtToDocxSvmMapMode, RelativeMapModeAndPopAreHonoured)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/svm_relative_map_mode.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    CBgraFrame frame;
    ASSERT_TRUE(frame.OpenFile(dir + FILE_SEPARATOR_STR + L"word/media/image1.png")) << "no PNG for the SVM picture";
    ASSERT_GT(frame.get_Width(), 0);

    const Rgb top_left = colour_at(frame, 0.1, 0.1);
    const Rgb middle = colour_at(frame, 0.7, 0.4);
    const Rgb bottom_right = colour_at(frame, 0.95, 0.9);
    EXPECT_TRUE(is(top_left, 255, 0, 0)) << "top left " << top_left.r << "," << top_left.g << "," << top_left.b;
    EXPECT_TRUE(is(middle, 0, 0, 255)) << "middle " << middle.r << "," << middle.g << "," << middle.b;
    EXPECT_TRUE(is(bottom_right, 0, 255, 0)) << "bottom right " << bottom_right.r << "," << bottom_right.g << "," << bottom_right.b;
}

// A crafted SVM: a map mode with a negative scale (ignored, as LibreOffice drops it), 5000 nested
// PUSH records around 40 relative map modes of scale 4294967295 (bounded states and scales), then
// 5000 POP. The picture still converts: red only in the top-left quarter, blue elsewhere, green in
// the bottom-right corner at the base scale.
TEST(OdtToDocxSvmMapMode, CraftedMapModesStayBounded)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/svm_crafted_map_mode.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    CBgraFrame frame;
    ASSERT_TRUE(frame.OpenFile(dir + FILE_SEPARATOR_STR + L"word/media/image1.png")) << "no PNG for the SVM picture";
    ASSERT_GT(frame.get_Width(), 0);

    const Rgb top_left = colour_at(frame, 0.1, 0.1);
    const Rgb top_right = colour_at(frame, 0.7, 0.25);
    const Rgb bottom_right = colour_at(frame, 0.95, 0.9);
    EXPECT_TRUE(is(top_left, 255, 0, 0)) << "top left " << top_left.r << "," << top_left.g << "," << top_left.b;
    EXPECT_TRUE(is(top_right, 0, 0, 255)) << "top right " << top_right.r << "," << top_right.g << "," << top_right.b;
    EXPECT_TRUE(is(bottom_right, 0, 255, 0)) << "bottom right " << bottom_right.r << "," << bottom_right.g << "," << bottom_right.b;
}
