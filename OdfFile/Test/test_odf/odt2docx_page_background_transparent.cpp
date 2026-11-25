// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX page background when the page colour is "transparent".

#include "gtest/gtest.h"

#include "common.h"
#include "DesktopEditor/common/File.h"

#include <string>

namespace
{
    // Value of attribute name="..." in the first element that starts with tagStart, or empty.
    std::wstring attribute_of(const std::wstring& xml, const std::wstring& tagStart, const std::wstring& name)
    {
        const size_t tag = xml.find(tagStart);
        if (tag == std::wstring::npos)
            return L"";
        const size_t tagEnd = xml.find(L'>', tag);
        const std::wstring key = L" " + name + L"=\"";
        const size_t pos = xml.find(key, tag);
        if (pos == std::wstring::npos || pos > tagEnd)
            return L"";
        const size_t begin = pos + key.size();
        const size_t end = xml.find(L'"', begin);
        return end == std::wstring::npos ? L"" : xml.substr(begin, end - begin);
    }
}

// LibreOffice writes a page picture with fo:background-color="transparent", plus draw:fill="bitmap"
// (current versions) or style:background-image (older versions, used here). The picture is the
// page background and must reach DOCX.
TEST(OdtToDocxPageBackground, TransparentColourWithImageKeepsImage)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_transparent_image.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";

    const size_t background = document.find(L"<w:background");
    ASSERT_NE(background, std::wstring::npos) << document.substr(0, 2000);
    EXPECT_EQ(attribute_of(document, L"<w:background", L"w:color"), L"ffffff");

    const std::wstring rId = attribute_of(document.substr(background), L"<v:fill", L"r:id");
    ASSERT_FALSE(rId.empty()) << document.substr(background, 1000);

    const std::wstring rels = read_part(dir, L"word/_rels/document.xml.rels");
    const std::wstring relTag = L"Id=\"" + rId + L"\"";
    const size_t rel = rels.find(relTag);
    ASSERT_NE(rel, std::wstring::npos) << rels;
    const size_t relStart = rels.rfind(L'<', rel);
    const std::wstring target = attribute_of(rels.substr(relStart), L"<Relationship", L"Target");
    ASSERT_FALSE(target.empty()) << rels;
    EXPECT_TRUE(NSFile::CFileBinary::Exists(dir + FILE_SEPARATOR_STR + L"word" + FILE_SEPARATOR_STR + target))
        << "missing word/" << std::string(target.begin(), target.end());
}

// A transparent colour without a fill still means no page background.
TEST(OdtToDocxPageBackground, TransparentColourWithoutFillWritesNoBackground)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_transparent.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";
    EXPECT_EQ(document.find(L"<w:background"), std::wstring::npos) << document.substr(0, 2000);
}

// A transparent colour with a gradient fill keeps the gradient, as LibreOffice shows it. VML draws a
// gradient from fillcolor to color2, so fillcolor must be the gradient's first colour (red here), not
// the white that stands in for the transparent page colour.
TEST(OdtToDocxPageBackground, TransparentColourWithGradientStartsWithGradientColour)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_transparent_gradient.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";

    const size_t background = document.find(L"<w:background");
    ASSERT_NE(background, std::wstring::npos) << document.substr(0, 2000);
    const std::wstring xml = document.substr(background);
    EXPECT_EQ(attribute_of(xml, L"<v:background", L"fillcolor"), L"#FF0000") << xml.substr(0, 1000);
    EXPECT_EQ(attribute_of(xml, L"<v:fill", L"color2"), L"#0000FF") << xml.substr(0, 1000);
    EXPECT_EQ(attribute_of(xml, L"<v:fill", L"type"), L"gradient") << xml.substr(0, 1000);
    // ODF 0deg runs from top to bottom; in VML that is angle 0, not the DrawingML 90.
    EXPECT_EQ(attribute_of(xml, L"<v:fill", L"angle"), L"0") << xml.substr(0, 1000);
}

// A VML gradient angle counts counterclockwise from the vertical, like ODF draw:angle, so an ODF
// 30deg gradient keeps 30 (it was written as the DrawingML angle 60, turned by 90 degrees).
TEST(OdtToDocxPageBackground, GradientAngleIsTheOdfAngle)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/page_background_transparent_gradient_angle.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";

    const size_t background = document.find(L"<w:background");
    ASSERT_NE(background, std::wstring::npos) << document.substr(0, 2000);
    const std::wstring xml = document.substr(background);
    EXPECT_EQ(attribute_of(xml, L"<v:fill", L"angle"), L"30") << xml.substr(0, 1000);
    EXPECT_EQ(attribute_of(xml, L"<v:background", L"fillcolor"), L"#FF0000") << xml.substr(0, 1000);
}
