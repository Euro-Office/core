// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of cells that hold text and pictures: the text stays in the cell. An SVG
// picture is one draw:frame with two draw:image elements (the SVG and a PNG fallback).

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
const std::wstring& out_dir()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/cell_text_two_pictures.ods");
    return dir;
}

// The text of each shared string, tags removed, in order.
std::vector<std::wstring> shared_strings()
{
    std::vector<std::wstring> out;
    const std::wstring xml = out_dir().empty() ? std::wstring() : read_part(out_dir(), L"xl/sharedStrings.xml");
    size_t pos = 0;
    while ((pos = xml.find(L"<si>", pos)) != std::wstring::npos)
    {
        const size_t end = xml.find(L"</si>", pos);
        if (end == std::wstring::npos)
            break;
        std::wstring text;
        bool in_tag = false;
        for (size_t i = pos + 4; i < end; ++i)
        {
            if (xml[i] == L'<')
                in_tag = true;
            else if (xml[i] == L'>')
                in_tag = false;
            else if (!in_tag)
                text += xml[i];
        }
        out.push_back(text);
        pos = end;
    }
    return out;
}

bool has_string_with(const std::wstring& part)
{
    for (const std::wstring& s : shared_strings())
        if (s.find(part) != std::wstring::npos)
            return true;
    return false;
}
}

TEST(OdsToXlsxCellTextPictures, TextBeforeSvgPictureWithFallback)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string_with(L"Partners")) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextPictures, TextBeforeSinglePicture)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string_with(L"Single")) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextPictures, TextBeforeAndAfterPicture)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    bool found = false;
    for (const std::wstring& s : shared_strings())
    {
        const size_t head = s.find(L"Head");
        if (head != std::wstring::npos && s.find(L"Tail", head) != std::wstring::npos)
            found = true;
    }
    EXPECT_TRUE(found) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextPictures, TextBeforeTwoFrames)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string_with(L"Two frames")) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextPictures, PicturesKept)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    const std::wstring drawing = read_part(out_dir(), L"xl/drawings/drawing1.xml");
    size_t count = 0;
    for (size_t pos = drawing.find(L"<xdr:pic>"); pos != std::wstring::npos; pos = drawing.find(L"<xdr:pic>", pos + 1))
        ++count;
    EXPECT_EQ(count, 5u) << drawing;
}
