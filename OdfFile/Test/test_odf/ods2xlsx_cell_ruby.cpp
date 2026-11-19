// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of text:ruby inside cells: a cell has no ruby, so the base text stays in the
// cell text and the annotation is dropped, as LibreOffice Calc does on import. The same holds for the
// page header and footer text.

#include "gtest/gtest.h"
#include "common.h"

#include <string>
#include <vector>

namespace
{
const std::wstring& out_dir()
{
    static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/cell_ruby.ods");
    return dir;
}

std::wstring shared_strings()
{
    return out_dir().empty() ? std::wstring() : read_part(out_dir(), L"xl/sharedStrings.xml");
}

std::wstring sheet()
{
    return out_dir().empty() ? std::wstring() : read_part(out_dir(), L"xl/worksheets/sheet1.xml");
}

// The text of every <si>, with the <t> parts of its runs joined.
std::vector<std::wstring> string_items()
{
    std::vector<std::wstring> items;
    const std::wstring xml = shared_strings();
    size_t pos = 0;
    while ((pos = xml.find(L"<si>", pos)) != std::wstring::npos)
    {
        const size_t end = xml.find(L"</si>", pos);
        if (end == std::wstring::npos) break;
        std::wstring text;
        size_t t = pos;
        while ((t = xml.find(L"<t", t)) != std::wstring::npos && t < end)
        {
            const size_t open_end = xml.find(L'>', t);
            const size_t close = xml.find(L"</t>", open_end);
            if (open_end == std::wstring::npos || close == std::wstring::npos || close > end) break;
            text += xml.substr(open_end + 1, close - open_end - 1);
            t = close;
        }
        items.push_back(text);
        pos = end;
    }
    return items;
}

bool has_item(const std::wstring& text)
{
    for (const auto& item : string_items())
        if (item == text) return true;
    return false;
}
}

TEST(OdsToXlsxCellRuby, BaseTextStaysBetweenTheTextAround)
{
    ASSERT_FALSE(shared_strings().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_item(L"Tokyo: 東京 station")) << shared_strings();
}

TEST(OdsToXlsxCellRuby, BaseTextInsideSpanIsKept)
{
    ASSERT_FALSE(shared_strings().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_item(L"漢字 text")) << shared_strings();
}

TEST(OdsToXlsxCellRuby, CellWithOnlyRubyKeepsItsText)
{
    ASSERT_FALSE(shared_strings().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_item(L"上")) << shared_strings();
}

TEST(OdsToXlsxCellRuby, AnnotationIsDropped)
{
    ASSERT_FALSE(shared_strings().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_EQ(shared_strings().find(L"とうきょう"), std::wstring::npos) << shared_strings();
    EXPECT_EQ(shared_strings().find(L"かんじ"), std::wstring::npos) << shared_strings();
    EXPECT_EQ(shared_strings().find(L"shang"), std::wstring::npos) << shared_strings();
}

TEST(OdsToXlsxCellRuby, CellWithoutRubyUnchanged)
{
    ASSERT_FALSE(shared_strings().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_TRUE(has_item(L"plain")) << shared_strings();
}

TEST(OdsToXlsxCellRuby, HeaderKeepsBaseTextOnly)
{
    ASSERT_FALSE(sheet().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_NE(sheet().find(L"<oddHeader>Head 本 end</oddHeader>"), std::wstring::npos) << sheet();
}

TEST(OdsToXlsxCellRuby, FooterRegionKeepsBaseTextOnly)
{
    ASSERT_FALSE(sheet().empty()) << "ODS -> XLSX conversion failed";
    EXPECT_NE(sheet().find(L"<oddFooter>&amp;LFoot 木 end</oddFooter>"), std::wstring::npos) << sheet();
}
