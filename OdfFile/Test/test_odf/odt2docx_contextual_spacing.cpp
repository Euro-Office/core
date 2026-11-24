// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of style:contextual-spacing (no spacing between
// paragraphs of the same style): w:contextualSpacing in the paragraph style.
// Also ODT -> RTF, which goes through the DOCX: \contextualspace is a flag.

#include "gtest/gtest.h"

#include "common.h"

#include "DesktopEditor/common/Directory.h"
#include "DesktopEditor/common/File.h"
#include "RtfFile/Format/ConvertationManager.h"

#include <ostream>
#include <string>

namespace
{
enum class Expected
{
    On,
    Off,
    Absent,
};

struct ContextualCase
{
    const wchar_t* fixture;
    Expected       expected;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const ContextualCase& c, std::ostream* os)
{
    *os << c.name;
}

class OdtToDocxContextualSpacing : public ::testing::TestWithParam<ContextualCase>
{
};

// Each fixture has one paragraph style "Ctx" with or without the attribute.
// "false" is written out too, so that it overrides a parent style.
TEST_P(OdtToDocxContextualSpacing, WritesElement)
{
    const ContextualCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring styles = read_part(dir, L"word/styles.xml");
    ASSERT_FALSE(styles.empty()) << "word/styles.xml is missing";
    EXPECT_TRUE(is_well_formed(styles));
    const std::wstring xml = styles + read_part(dir, L"word/document.xml");

    const bool on = xml.find(L"<w:contextualSpacing/>") != std::wstring::npos;
    const bool off = xml.find(L"<w:contextualSpacing w:val=\"0\"/>") != std::wstring::npos;
    const bool any = xml.find(L"<w:contextualSpacing") != std::wstring::npos;
    EXPECT_EQ(on, c.expected == Expected::On);
    EXPECT_EQ(off, c.expected == Expected::Off);
    EXPECT_EQ(any, c.expected != Expected::Absent);
}

// The RTF control word \contextualspace has no off form: a reader turns the
// property on when it sees the word, whatever number follows. So "false" and a
// missing attribute both give no control word at all (off is the state after
// \pard), and "true" gives the bare word.
TEST_P(OdtToDocxContextualSpacing, WritesRtfFlagOnlyWhenOn)
{
    const ContextualCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring rtfPath = dir + FILE_SEPARATOR_STR + L"out.rtf";
    RtfConvertationManager rtfConvert;
    ASSERT_EQ(0u, rtfConvert.ConvertOOXToRtf(rtfPath, dir)) << "DOCX -> RTF conversion failed";

    std::string rtf;
    ASSERT_TRUE(NSFile::CFileBinary::ReadAllTextUtf8A(rtfPath, rtf));
    ASSERT_NE(rtf.find("\\rtf1"), std::string::npos) << "not an RTF file";

    const std::string word = "\\contextualspace";
    size_t      bare = 0;
    size_t      withValue = 0;
    for (size_t pos = rtf.find(word); pos != std::string::npos; pos = rtf.find(word, pos + word.size()))
    {
        const char next = pos + word.size() < rtf.size() ? rtf[pos + word.size()] : ' ';
        if (('0' <= next && next <= '9') || next == '-')
            ++withValue;
        else
            ++bare;
    }
    EXPECT_EQ(0u, withValue) << "\\contextualspace takes no value";
    if (c.expected == Expected::On)
        EXPECT_GT(bare, 0u) << "contextual spacing is on, the RTF has no \\contextualspace";
    else
        EXPECT_EQ(0u, bare) << "contextual spacing is not on, the RTF has \\contextualspace";
}

const ContextualCase kContextualCases[] = {
    { L"contextual_spacing_true.odt", Expected::On, "True" },
    { L"contextual_spacing_false.odt", Expected::Off, "False" },
    { L"contextual_spacing_missing.odt", Expected::Absent, "Missing" },
    { L"contextual_spacing_loext_true.odt", Expected::On, "LoextTrue" },
    { L"contextual_spacing_loext_false.odt", Expected::Off, "LoextFalse" },
};

std::string ContextualCaseName(const ::testing::TestParamInfo<ContextualCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(ContextualSpacing, OdtToDocxContextualSpacing, ::testing::ValuesIn(kContextualCases),
                         ContextualCaseName);
} // namespace
