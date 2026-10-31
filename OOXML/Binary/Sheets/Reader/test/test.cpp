/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "gtest/gtest.h"

#include "OOXML/Binary/Sheets/Reader/CSVReader.h"
#include "OOXML/XlsxFormat/Xlsx.h"
#include "OOXML/XlsxFormat/Worksheets/Worksheet.h"
#include "Common/OfficeFileErrorDescription.h"

#include <fstream>
#include <string>

// The bin-format CSV reader (OOXML/Binary/Sheets/Reader/CSVReader.cpp) is
// exercised by every CSV/TXT import in the stack and had no test coverage --
// which is how the #160 ghost-row bug shipped undetected (an empty row was
// added for UTF-8 files ending in a line break; fixed in this PR). This suite
// covers the reader's public API directly: content is generated in-test,
// written to the GoogleTest temp dir, read back, and asserted against the
// in-memory CXlsx the reader produces. No fixtures.
//
// Each test below pins one behaviour of Read(): the encoding dispatch (by
// codepage), BOM handling, the sep= directive, RFC4180 quoting, delimiter
// selection, tab removal, column positions of empty cells, the 500k-char
// chunked-erase path, the xml-cache read mode, and the error contract.
//
// Deliberately NOT asserted as correct behaviour (follow-up issue
// candidates): UTF-16BE content under codepage 48 (the reader assumes
// little-endian and performs no endian conversion), and a UTF-32LE file with
// BOM (the BOM strip handles 2-3 byte marks only, so half the 4-byte BOM
// survives as leading NULs and the first cell is dropped).

namespace
{
    const _UINT32 CP_ANSI  = 1000;
    const _UINT32 CP_UTF8  = 46;
    const _UINT32 CP_UTF16 = 48;
    const _UINT32 CP_UTF32 = 50;

    // GoogleTest's temp dir is a narrow ASCII path on Linux; widening it via
    // std::wstring(begin, end) is safe for these names.
    std::wstring TempPath(const char* sName)
    {
        std::string sPath = testing::TempDir() + sName;
        return std::wstring(sPath.begin(), sPath.end());
    }

    void WriteBytes(const std::wstring& sPath, const void* pData, size_t nSize)
    {
        std::string sPathA(sPath.begin(), sPath.end());
        std::ofstream oFile(sPathA.c_str(), std::ios::binary);
        oFile.write((const char*)pData, (std::streamsize)nSize);
    }

    void WriteUtf8(const std::wstring& sPath, const std::string& sContent, bool bBom)
    {
        static const char pBom[3] = { '\xEF', '\xBB', '\xBF' };
        std::string sPathA(sPath.begin(), sPath.end());
        std::ofstream oFile(sPathA.c_str(), std::ios::binary);
        if (bBom)
            oFile.write(pBom, 3);
        oFile.write(sContent.data(), (std::streamsize)sContent.size());
    }

    // Manual UTF-16LE encoding: the test content is BMP-only, so each wchar_t
    // is exactly one UTF-16 code unit (no surrogate pairs to build by hand).
    std::string Utf16LeBytes(const std::wstring& sContent)
    {
        std::string sBytes;
        for (wchar_t wc : sContent)
        {
            unsigned short nUnit = (unsigned short)wc;
            sBytes.push_back((char)(nUnit & 0xFF));
            sBytes.push_back((char)((nUnit >> 8) & 0xFF));
        }
        return sBytes;
    }

    std::string Utf32LeBytes(const std::wstring& sContent)
    {
        std::string sBytes;
        for (wchar_t wc : sContent)
        {
            unsigned int nUnit = (unsigned int)wc;
            sBytes.push_back((char)(nUnit & 0xFF));
            sBytes.push_back((char)((nUnit >> 8) & 0xFF));
            sBytes.push_back((char)((nUnit >> 16) & 0xFF));
            sBytes.push_back((char)((nUnit >> 24) & 0xFF));
        }
        return sBytes;
    }

    _UINT32 ReadFile(const std::wstring& sPath, OOX::Spreadsheet::CXlsx& oXlsx,
        _UINT32 nCodePage, const std::wstring& sDelimiter, bool bCache = false)
    {
        CSVReader oReader;
        oReader.readToxmlCache_ = bCache;
        return oReader.Read(sPath, oXlsx, nCodePage, sDelimiter, 1033);
    }

    // The reader always produces exactly one worksheet; every test asserts
    // against its sheet data.
    OOX::Spreadsheet::CSheetData& Sheet(OOX::Spreadsheet::CXlsx& oXlsx)
    {
        EXPECT_FALSE(oXlsx.m_arWorksheets.empty());
        EXPECT_TRUE(oXlsx.m_arWorksheets[0]->m_oSheetData.IsInit());
        return *oXlsx.m_arWorksheets[0]->m_oSheetData;
    }

    // AddCell stores the raw cell text in m_oCacheValue "as is"; this is the
    // stable, format-detection-independent view of what the reader parsed.
    std::wstring CellText(const OOX::Spreadsheet::CCell* pCell)
    {
        return pCell->m_oCacheValue.IsInit() ? *pCell->m_oCacheValue : std::wstring();
    }
}

// Happy path: plain UTF-8 with the default comma delimiter. Pins the basic
// row/cell model (row count, cell order, cell text, 1-based row numbers)
// before the edge cases below start varying it.
TEST(CsvReader, Utf8BasicRowsAndCells)
{
    std::wstring sPath = TempPath("utf8_basic.csv");
    WriteUtf8(sPath, "a,b\nc,d\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    ASSERT_EQ(oData.m_arrItems[0]->m_arrItems.size(), 2u);
    ASSERT_EQ(oData.m_arrItems[1]->m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[0]), L"a");
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[1]), L"b");
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[0]), L"c");
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[1]), L"d");

    EXPECT_TRUE(oData.m_arrItems[1]->m_oR.IsInit());
    EXPECT_EQ(oData.m_arrItems[1]->m_oR->GetValue(), 2u); // row numbers are 1-based
}

// Regression for #160: UTF-8 content ending in a line break produced a
// trailing empty row, because the reader's converted buffer was not shrunk
// to the converted length and the NUL padding made the final "leftover text"
// check pass. This is the case the fix in this PR carries.
TEST(CsvReader, Utf8TrailingLineBreakAddsNoEmptyRow)
{
    std::wstring sPath = TempPath("utf8_trailing.csv");
    WriteUtf8(sPath, "a,b\nc,d\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    for (const OOX::Spreadsheet::CRow* pRow : oData.m_arrItems)
        EXPECT_FALSE(pRow->m_arrItems.empty()) << "ghost row with no cells";
}

// A lone \r must end a row like any other line break -- some legacy exporters
// emit CR-only files -- and like the LF case it must not add a trailing empty
// row when the file ends with it.
TEST(CsvReader, TrailingCarriageReturnOnly)
{
    std::wstring sPath = TempPath("utf8_cr.csv");
    WriteUtf8(sPath, "a,b\rc,d\r", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[1]), L"d");
}

// Windows line endings: the reader must collapse \r\n into a single break
// ("there should be only 1 carriage return per combination", per its own
// source comment), or every Windows-authored CSV would grow empty rows.
TEST(CsvReader, CrlfLineEndings)
{
    std::wstring sPath = TempPath("utf8_crlf.csv");
    WriteUtf8(sPath, "a,b\r\nc,d\r\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u); // \r\n must count as one break
    ASSERT_EQ(oData.m_arrItems[1]->m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[1]), L"d");
}

// The EF BB BF BOM must be consumed before decoding; a surviving BOM would
// corrupt the first cell's text.
TEST(CsvReader, Utf8BomIsSkipped)
{
    std::wstring sPath = TempPath("utf8_bom.csv");
    WriteUtf8(sPath, "a,b\n", true);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u);
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[0]), L"a");
}

// Codepage 48 (the "temporary CP_UTF16" of the source's own dispatch): the
// UTF-16LE + FF FE BOM shape that Windows Excel writes for "Unicode" CSVs.
TEST(CsvReader, Utf16LeWithBom)
{
    std::wstring sPath = TempPath("utf16le.csv");
    static const char pBom[2] = { '\xFF', '\xFE' };
    std::string sBytes = std::string(pBom, 2) + Utf16LeBytes(L"a,b\nc,d\n");
    WriteBytes(sPath, sBytes.data(), sBytes.size());

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF16, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[1]), L"d");
}

// Codepage 50: raw UTF-32LE without BOM. A UTF-32LE file WITH BOM is not
// asserted here -- the reader's BOM strip only handles 2-3 byte marks, so the
// first half of the 4-byte UTF-32LE BOM currently survives as NULs (see the
// header note; follow-up issue candidate).
TEST(CsvReader, Utf32LeWithoutBom)
{
    std::wstring sPath = TempPath("utf32le.csv");
    std::string sBytes = Utf32LeBytes(L"a,b\nc,d\n");
    WriteBytes(sPath, sBytes.data(), sBytes.size());

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF32, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[1]), L"d");
}

// Codepage 1000 goes through ansi_2_unicode, which is locale-dependent, so
// this pins only the ASCII-safe subset of that path.
TEST(CsvReader, AnsiCodePage)
{
    std::wstring sPath = TempPath("ansi.csv");
    WriteUtf8(sPath, "hello,world\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_ANSI, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u);
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[1]), L"world");
}

// Excel writes "sep=;" as the first line when the user chose a non-comma
// delimiter; the reader must consume the directive line and switch delimiter
// from that point on.
TEST(CsvReader, SepDirectiveSetsDelimiter)
{
    std::wstring sPath = TempPath("sep_directive.csv");
    WriteUtf8(sPath, "sep=;\na;b\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u);
    ASSERT_EQ(oData.m_arrItems[0]->m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[0]), L"a");
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[1]), L"b");
}

// Without a sep= line, the delimiter comes from the caller's argument -- the
// value x2t derives from format settings. The delimiter can be any character,
// semicolon being the common European-locale case.
TEST(CsvReader, DelimiterFromArgument)
{
    std::wstring sPath = TempPath("delim_arg.csv");
    WriteUtf8(sPath, "a;b\nc;d\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L";"), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[1]), L"d");
}

// RFC4180 quoting: doubled quotes escape themselves and a quoted delimiter
// does not split the cell. The example is the reader's own source comment's
// example ("Super, ""luxurious"" truck"), so it doubles as documentation of
// the expected behaviour for that comment's author.
TEST(CsvReader, QuotedFieldWithEscapedQuotesAndDelimiter)
{
    std::wstring sPath = TempPath("quoted.csv");
    WriteUtf8(sPath, "\"Super, \"\"luxurious\"\" truck\",1997,Ford\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u);
    ASSERT_EQ(oData.m_arrItems[0]->m_arrItems.size(), 3u);
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[0]), L"Super, \"luxurious\" truck");
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[1]), L"1997");
}

// A line break inside quotes is data, not a row end: the reader marks the cell
// as wrapped and keeps consuming the same row.
TEST(CsvReader, QuotedFieldWithEmbeddedNewline)
{
    std::wstring sPath = TempPath("quoted_nl.csv");
    WriteUtf8(sPath, "\"line1\nline2\",b\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u); // the embedded break must not split the row
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[0]), L"line1\nline2");
}

// Tabs are only meaningful as the sep=\t delimiter; when they are not the
// delimiter the reader collects them (the oDeleteChars path) and deletes them,
// so "a\tb" parses as "ab".
TEST(CsvReader, TabsRemovedWhenNotDelimiter)
{
    std::wstring sPath = TempPath("tabs.csv");
    WriteUtf8(sPath, "a\tb,c\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u);
    ASSERT_EQ(oData.m_arrItems[0]->m_arrItems.size(), 2u);
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[0]), L"ab");
}

// Empty fields are not written as cells (the reader skips zero-length words),
// but the surviving cells must keep their true column positions via setRowCol,
// so "a,,c" has exactly two cells -- "a" at column 0 and "c" at column 2.
TEST(CsvReader, EmptyCellsKeepColumnPositions)
{
    std::wstring sPath = TempPath("empty_cells.csv");
    WriteUtf8(sPath, "a,,c\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 1u);
    ASSERT_EQ(oData.m_arrItems[0]->m_arrItems.size(), 2u);

    int nRow = -1, nCol = -1;
    EXPECT_TRUE(oData.m_arrItems[0]->m_arrItems[1]->getRowCol(nRow, nCol));
    EXPECT_EQ(nCol, 2); // "c" lands in the third column (0-based 2)
    EXPECT_EQ(CellText(oData.m_arrItems[0]->m_arrItems[1]), L"c");
}

// To bound memory, the reader keeps only a 500,000-char window of the decoded
// text and erases consumed data as it goes (the sFileDataW.erase() path). A
// single line longer than that window must come through with every cell intact.
TEST(CsvReader, LongSingleLineCrossesChunkEraseBoundary)
{
    std::string sCell(2000, 'x');
    std::string sContent;
    for (int i = 0; i < 300; ++i) // 300 cells x 2000 chars > 500k
    {
        if (i)
            sContent += ",";
        sCell[0] = (char)('a' + (i % 26));
        sContent += sCell;
    }
    sContent += "\nend\n";

    std::wstring sPath = TempPath("chunk.csv");
    WriteUtf8(sPath, sContent, false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    ASSERT_EQ(oData.m_arrItems.size(), 2u);
    ASSERT_EQ(oData.m_arrItems[0]->m_arrItems.size(), 300u);

    std::wstring sFirst = CellText(oData.m_arrItems[0]->m_arrItems[0]);
    std::wstring sLast = CellText(oData.m_arrItems[0]->m_arrItems[299]);
    EXPECT_EQ(sFirst.size(), 2000u);
    EXPECT_EQ(sFirst[0], L'a');
    EXPECT_EQ(sFirst[1], L'x');
    EXPECT_EQ(sLast.size(), 2000u);
    EXPECT_EQ(sLast[0], L'n'); // 299 % 26 == 13 -> 'a' + 13
    EXPECT_EQ(CellText(oData.m_arrItems[1]->m_arrItems[0]), L"end");
}

// readToxmlCache_ routes parsed rows through AddRowToCache instead of keeping
// the full CRow tree in memory -- the read mode some conversion paths use.
// Asserts the mode completes cleanly and still produces the worksheet.
TEST(CsvReader, XmlCacheReadModeSucceeds)
{
    std::wstring sPath = TempPath("cache.csv");
    WriteUtf8(sPath, "a,b\n", false);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L",", true), 0u);
    EXPECT_FALSE(oXlsx.m_arWorksheets.empty());
}

// The error contract: an unreadable file returns AVS_FILEUTILS_ERROR_CONVERT
// (the reader bails out before creating any worksheet content).
TEST(CsvReader, MissingFileReturnsConvertError)
{
    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(TempPath("does_not_exist.csv"), oXlsx, CP_UTF8, L","),
        (_UINT32)AVS_FILEUTILS_ERROR_CONVERT);
}

// A zero-byte file is valid input: success return, zero rows -- and notably
// no ghost row (the final left-over-text check must not fire on emptiness).
TEST(CsvReader, EmptyFileYieldsNoRows)
{
    std::wstring sPath = TempPath("empty.csv");
    WriteBytes(sPath, "", 0);

    OOX::Spreadsheet::CXlsx oXlsx;
    EXPECT_EQ(ReadFile(sPath, oXlsx, CP_UTF8, L","), 0u);

    OOX::Spreadsheet::CSheetData& oData = Sheet(oXlsx);
    EXPECT_EQ(oData.m_arrItems.size(), 0u);
}