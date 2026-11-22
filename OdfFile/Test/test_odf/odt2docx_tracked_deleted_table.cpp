// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX of a tracked deletion that holds a table.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// The fixture deletes a 2x2 table (one cell spans two columns) with tracked changes; the change
// mark stands at the start of the heading after it. The deleted content is written inside that
// heading's paragraph, where a table cannot stand: the cell text must come out as deleted runs,
// the document must stay well-formed, and the text around it must be kept.
TEST(OdtToDocxTrackedDeletedTable, CellTextBecomesDeletedRuns)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/tracked_deleted_table.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(document)) << document.substr(0, 3000);

    EXPECT_EQ(document.find(L"<w:tbl>"), std::wstring::npos) << "a table was written inside a paragraph";

    for (const wchar_t* cell : {L"CellOne", L"CellTwo", L"CellThree"})
    {
        const std::wstring deleted = std::wstring(L"<w:delText>") + cell + L"</w:delText>";
        EXPECT_NE(document.find(deleted), std::wstring::npos) << "missing deleted cell text " << cell;
    }

    // The text around the deletion stays normal text (w:t), not deleted text.
    for (const wchar_t* kept : {L"Before the table", L"Heading after the table", L"Text after the table"})
    {
        const std::wstring normal = std::wstring(L">") + kept + L"</w:t>";
        EXPECT_NE(document.find(normal), std::wstring::npos) << "missing text " << kept;
    }
}

// The deleted content holds a paragraph with a text box, and the text box holds a table. The text
// box content starts outside a paragraph, so that table can stay a table inside w:txbxContent.
TEST(OdtToDocxTrackedDeletedTable, TableInTextBoxStaysTable)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/tracked_deleted_text_box_table.odt");
    ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

    const std::wstring document = read_part(dir, L"word/document.xml");
    ASSERT_FALSE(document.empty()) << "word/document.xml is missing";
    EXPECT_TRUE(is_well_formed(document)) << document.substr(0, 3000);

    const size_t box = document.find(L"<w:txbxContent>");
    ASSERT_NE(box, std::wstring::npos) << document.substr(0, 3000);
    const size_t table = document.find(L"<w:tbl>", box);
    ASSERT_NE(table, std::wstring::npos) << "the table in the text box was not written as a table";
    const size_t boxEnd = document.find(L"</w:txbxContent>", box);
    EXPECT_LT(table, boxEnd);
    EXPECT_NE(document.find(L"<w:tc>", table), std::wstring::npos);
    EXPECT_NE(document.find(L">BoxOne</w:delText>", table), std::wstring::npos);
}
