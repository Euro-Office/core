// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX conversion of text fields. Each fixture holds one paragraph
// "INPUT <field> END" whose field text (and, where the field has them, list
// item value, variable name or referenced name) contains an ampersand. The
// converter must escape these values: an unescaped '&' makes
// word/document.xml not well-formed, and the editor then drops everything
// from the field onwards.

#include "gtest/gtest.h"

#include "common.h"

#include <ostream>
#include <string>

namespace
{
struct FieldCase
{
    const wchar_t* fixture;
    const wchar_t* escaped; // a value from the field as it must appear in the part
    const char*    name;
};

// Name the case in failure messages instead of dumping its bytes.
void PrintTo(const FieldCase& c, std::ostream* os)
{
    *os << c.name;
}

class OdtToDocxFieldText : public ::testing::TestWithParam<FieldCase>
{
};

TEST_P(OdtToDocxFieldText, EscapesFieldValues)
{
    const FieldCase& c = GetParam();

    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + c.fixture);
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring xml = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(xml.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(xml));
    EXPECT_NE(xml.find(L"A &amp; B"), std::wstring::npos);
    EXPECT_NE(xml.find(c.escaped), std::wstring::npos);
    EXPECT_NE(xml.find(L"END"), std::wstring::npos);
}

const FieldCase kFieldCases[] = {
    { L"field_text_input_ampersand.odt", L"<w:t>A &amp; B</w:t>", "TextInput" },
    { L"field_expression_ampersand.odt", L"<w:t>A &amp; B</w:t>", "Expression" },
    { L"field_drop_down_ampersand.odt", L"w:value=\"A &amp; B\"", "DropDown" },
    { L"field_variable_input_ampersand.odt", L"REF &quot;v&amp;w&quot;", "VariableInput" },
    { L"field_bookmark_ref_ampersand.odt", L"<w:instrText>REF v&amp;w \\h</w:instrText>", "BookmarkRef" },
    { L"field_sequence_ref_ampersand.odt", L"w:instr=\" REF v&amp;w \\h\"", "SequenceRef" },
};

std::string CaseName(const ::testing::TestParamInfo<FieldCase>& info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(Fields, OdtToDocxFieldText, ::testing::ValuesIn(kFieldCases), CaseName);
} // namespace
