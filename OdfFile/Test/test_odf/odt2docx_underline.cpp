// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of underlines: the w:u values the WordprocessingML
// writer puts into word/document.xml and word/styles.xml.

#include "gtest/gtest.h"

#include "common.h"

#include "OdfFile/Reader/Format/odfcontext.h"
#include "OdfFile/Reader/Format/style_text_properties.h"

#include <ostream>
#include <sstream>
#include <string>

namespace
{
// w:u takes ST_Underline values (ECMA-376-1 §17.18.99): "double", "wave",
// "wavyDouble". The DrawingML names "dbl", "wavy" and "wavyDbl" are not
// ST_Underline values, and x2t's own DOCX reader maps them to no underline.

struct UnderlineCase
{
    const wchar_t* fixture;
    const wchar_t* value;
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const UnderlineCase& c, std::ostream* os)
{
    *os << c.name;
}

class OdtToDocxUnderline : public ::testing::TestWithParam<UnderlineCase>
{
};

std::wstring underline_attr(const std::wstring& value)
{
    return L"<w:u w:val=\"" + value + L"\"";
}

void expect_underline(const std::wstring& xml, const std::wstring& value)
{
    EXPECT_NE(xml.find(underline_attr(value)), std::wstring::npos) << "no w:u w:val=\"" << value << "\"";
    for (const wchar_t* wrong : {L"dbl", L"wavy", L"wavyDbl"})
        EXPECT_EQ(xml.find(underline_attr(wrong)), std::wstring::npos) << "w:u w:val=\"" << wrong << "\"";
}

// Each fixture sets the same underline on an automatic text style (a span in
// the body, written into the run's w:rPr) and on a common paragraph style
// (written into word/styles.xml).
TEST_P(OdtToDocxUnderline, WritesStUnderlineValue)
{
    const UnderlineCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";
    expect_underline(document, c.value);

    const std::wstring styles = read_part(dir, L"word/styles.xml");
    ASSERT_FALSE(styles.empty()) << "word/styles.xml is missing";
    expect_underline(styles, c.value);
}

const UnderlineCase kUnderlineCases[] = {
    { L"underline_double.odt", L"double", "Double" },
    { L"underline_wave.odt", L"wave", "Wave" },
    { L"underline_wave_double.odt", L"wavyDouble", "WaveDouble" },
};

std::string UnderlineCaseName(const ::testing::TestParamInfo<UnderlineCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(Underline, OdtToDocxUnderline, ::testing::ValuesIn(kUnderlineCases), UnderlineCaseName);

// The simplified w:rPr writer used for text inside formulas has its own copy
// of the mapping; check it directly.
TEST(DocxSerializeUnderline, WritesStUnderlineValue)
{
    using cpdoccore::odf_types::line_style;
    using cpdoccore::odf_types::line_type;

    struct
    {
        _CP_OPT(line_type) type;
        line_style         style;
        const wchar_t*     value;
    } const cases[] = {
        { line_type(line_type::Double), line_style(line_style::Solid), L"double" },
        { _CP_OPT(line_type)(), line_style(line_style::Wave), L"wave" },
        { line_type(line_type::Double), line_style(line_style::Wave), L"wavyDouble" },
    };

    for (const auto& c : cases)
    {
        cpdoccore::odf_reader::text_format_properties props;
        props.style_text_underline_type_  = c.type;
        props.style_text_underline_style_ = c.style;

        cpdoccore::odf_reader::fonts_container fonts;
        std::wstringstream strm;
        props.docx_serialize(strm, fonts);

        expect_underline(strm.str(), c.value);
    }
}
} // namespace
