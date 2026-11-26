// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of fo:keep-with-next: the w:keepNext the
// WordprocessingML writer puts into word/document.xml and word/styles.xml.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
// The w:p element of word/document.xml that holds the given text, or the
// w:style element of word/styles.xml with the given name.
std::wstring element_with(const std::wstring& xml, const std::wstring& open, const std::wstring& close,
                          const std::wstring& text)
{
    const size_t at = xml.find(text);
    if (at == std::wstring::npos)
        return L"";
    const size_t begin = xml.rfind(open, at);
    const size_t end   = xml.find(close, at);
    if (begin == std::wstring::npos || end == std::wstring::npos)
        return L"";
    return xml.substr(begin, end + close.size() - begin);
}

std::wstring paragraph_with(const std::wstring& document, const std::wstring& text)
{
    // "<w:p>" and not "<w:p": that would also match "<w:pPr>".
    return element_with(document, L"<w:p>", L"</w:p>", text);
}

size_t count(const std::wstring& xml, const std::wstring& what)
{
    size_t n = 0;
    for (size_t at = xml.find(what); at != std::wstring::npos; at = xml.find(what, at + what.size()))
        ++n;
    return n;
}

// Keep with next (fo:keep-with-next -> w:keepNext) is independent of the
// paragraph alignment; a centred heading must keep both.
TEST(OdtToDocxKeepWithNext, KeptWithAlignment)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/keep_with_next_centered.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    // Automatic paragraph style: written into the paragraph's w:pPr.
    const std::wstring document = read_part(dir, L"word/document.xml");
    const std::wstring keep     = paragraph_with(document, L"KEEP");
    ASSERT_FALSE(keep.empty()) << "paragraph KEEP not found";
    EXPECT_NE(keep.find(L"<w:jc w:val=\"center\"/>"), std::wstring::npos) << keep;
    EXPECT_NE(keep.find(L"<w:keepNext w:val=\"true\"/>"), std::wstring::npos) << keep;

    // Common paragraph style: written into word/styles.xml.
    const std::wstring styles = read_part(dir, L"word/styles.xml");
    const std::wstring style  = element_with(styles, L"<w:style ", L"</w:style>", L"<w:name w:val=\"CenteredKeep\"/>");
    ASSERT_FALSE(style.empty()) << "style CenteredKeep not found";
    EXPECT_NE(style.find(L"<w:jc w:val=\"center\"/>"), std::wstring::npos) << style;
    EXPECT_NE(style.find(L"<w:keepNext w:val=\"true\"/>"), std::wstring::npos) << style;
}

// A drop cap is written as a framed paragraph that carries its own w:keepNext;
// it must not get a second one, and the rest of the paragraph keeps the style's.
void expect_drop_cap_keep_next(const std::wstring& fixture, bool centered)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/" + fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    const std::wstring frame    = paragraph_with(document, L"w:dropCap=");
    ASSERT_FALSE(frame.empty()) << "drop cap frame paragraph not found";
    EXPECT_EQ(count(frame, L"<w:keepNext"), 1u) << frame;

    const std::wstring rest = paragraph_with(document, L"ropped");
    ASSERT_FALSE(rest.empty()) << "rest of the drop cap paragraph not found";
    if (centered)
        EXPECT_NE(rest.find(L"<w:jc w:val=\"center\"/>"), std::wstring::npos) << rest;
    EXPECT_EQ(count(rest, L"<w:keepNext w:val=\"true\"/>"), 1u) << rest;
}

TEST(OdtToDocxKeepWithNext, CenteredDropCapGetsOneKeepNext)
{
    expect_drop_cap_keep_next(L"keep_with_next_drop_cap.odt", true);
}

TEST(OdtToDocxKeepWithNext, DropCapWithoutAlignmentGetsOneKeepNext)
{
    expect_drop_cap_keep_next(L"keep_with_next_drop_cap_unaligned.odt", false);
}
} // namespace
