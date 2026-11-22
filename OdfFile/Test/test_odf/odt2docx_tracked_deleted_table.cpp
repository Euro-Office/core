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

    for (const wchar_t* kept : {L"Before the table", L"Heading after the table", L"Text after the table"})
        EXPECT_NE(document.find(kept), std::wstring::npos) << "missing text";
}
