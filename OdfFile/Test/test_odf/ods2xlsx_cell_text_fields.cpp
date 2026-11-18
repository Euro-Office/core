// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of fields inside cell text (text:sheet-name, text:date, text:title). Cell text in
// SpreadsheetML has no fields, so each field keeps the value stored in the document.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
const std::wstring& out_dir()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/cell_text_fields.ods");
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

bool has_string(const std::wstring& expected)
{
    for (const std::wstring& s : shared_strings())
        if (s == expected)
            return true;
    return false;
}
}

TEST(OdsToXlsxCellTextFields, SheetNameKept)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string(L"Sheet name is Sheet1.")) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextFields, DateKept)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string(L"Date: 07/08/2025")) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextFields, DateInFormattedSpanKept)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string(L"Date 23.07.2013 in bold")) << read_part(out_dir(), L"xl/sharedStrings.xml");
    // The date takes the bold run of its span.
    const std::wstring xml = read_part(out_dir(), L"xl/sharedStrings.xml");
    const size_t date = xml.find(L"23.07.2013");
    ASSERT_NE(date, std::wstring::npos) << xml;
    const size_t run = xml.rfind(L"<r>", date);
    ASSERT_NE(run, std::wstring::npos) << xml;
    EXPECT_NE(xml.substr(run, date - run).find(L"<b"), std::wstring::npos) << xml;
}

TEST(OdsToXlsxCellTextFields, TitleKept)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_string(L"Title: Fixture")) << read_part(out_dir(), L"xl/sharedStrings.xml");
}

TEST(OdsToXlsxCellTextFields, HeaderKeepsDateCode)
{
    ASSERT_FALSE(out_dir().empty()) << "ODS -> XLSX conversion failed";
    const std::wstring sheet = read_part(out_dir(), L"xl/worksheets/sheet1.xml");
    EXPECT_NE(sheet.find(L"<oddHeader>&amp;D</oddHeader>"), std::wstring::npos) << sheet;
}
