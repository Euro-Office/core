// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX of a StarView metafile (SVM) picture, the format LibreOffice writes for metafile
// pictures and for the replacement images of embedded objects: it is converted to a PNG. Without
// SVM support in the graphics library the PNG was never written and only its relationship stayed.

#include "gtest/gtest.h"

#include "common.h"

#include "../../../DesktopEditor/common/File.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
std::wstring package()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/svm_image.odt");
    return dir;
}

std::vector<unsigned char> read_bytes(const std::wstring& path)
{
    std::vector<unsigned char> bytes;
    NSFile::CFileBinary file;
    if (!file.OpenFile(path))
        return bytes;
    bytes.resize(static_cast<size_t>(file.GetFileSize()));
    DWORD read = 0;
    file.ReadFile(bytes.data(), static_cast<DWORD>(bytes.size()), read);
    bytes.resize(read);
    return bytes;
}
} // namespace

TEST(OdtToDocxSvmImage, PictureBecomesAPng)
{
    const std::wstring dir = package();
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
    const std::wstring rels = read_part(dir, L"word/_rels/document.xml.rels");
    ASSERT_NE(rels.find(L"media/image1.png"), std::wstring::npos) << rels;

    const std::vector<unsigned char> png = read_bytes(dir + FILE_SEPARATOR_STR + L"word/media/image1.png");
    ASSERT_GT(png.size(), 8u) << "the PNG for the SVM picture is missing";
    const unsigned char signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    EXPECT_TRUE(std::equal(signature, signature + 8, png.begin()));
}
