// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only
//
// Unit coverage for RtfLex, the tokenizer that fronts every RTF import
// (rtf2docx and every conversion that accepts RTF input). The lexer accepts
// only file paths (StringStream reads via NSFile), so every case writes its
// RTF snippet to a temp file and lexes it end to end.
//
// The pinned behaviors come straight from RtfLex::NextToken/parseKeyword/
// parseText:
//   - token dispatch: GroupStart/GroupEnd, Keyword, Control, Text, Eof
//   - keyword parameters: positive, negative, trailing-space consumption
//   - escaped delimiters (\\ \{ \}) become Text tokens
//   - control symbols (\~ \- \| \_ \:) become Keywords with the symbol as key
//   - \'hh hex escapes become Control tokens whose Key is the *decimal code*
//     of the apostrophe and whose Parameter carries the byte, with the +1
//     shift for control bytes (so \'00 survives as 1); the shift bound makes
//     0x1E and 0x1F indistinguishable -- a known quirk, pinned as current
//     behavior in HexEscapeControlRangeShift
//   - backslash + literal tab/newline become the keywords tab/par
//   - newlines inside text runs are dropped; a bare tab survives as text
//   - control bytes between tokens are skipped
//   - SetSource rejects files under 5 bytes (error contract)
//
// Note: SetSource refuses files under 5 bytes, so every fixture below is
// padded with a minimal {\rtf1 ...} envelope -- the behavior under test is
// always the innermost tokens.

#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "DesktopEditor/common/File.h"
#include "RtfFile/Format/RtfLex.h"
#include "RtfFile/Format/RtfToken.h"

namespace {

// Unique temp file in the system temp directory. A per-process counter
// would collide between build trees and CI jobs on a shared host; NSFile
// gives a unique name on every platform the suite builds for.
std::wstring MakeFixturePath()
{
    return NSFile::CFileBinary::CreateTempFileWithUniqueName(
        NSFile::CFileBinary::GetTempPath(), L"rtf_lex_test");
}

// Writes the snippet and returns its path (paths are plain ASCII, so the
// narrow/wide conversion below is loss-free).
std::wstring WriteFixture(const std::string& sContent)
{
    const std::wstring sPath = MakeFixturePath();
    {
        std::ofstream oFile(std::string(sPath.begin(), sPath.end()), std::ios::binary);
        oFile.write(sContent.data(), (std::streamsize)sContent.size());
    }
    return sPath;
}

// Lexes the snippet to EOF and returns the non-EOF tokens. If SetSource
// rejects the file, the result is empty.
std::vector<RtfToken> LexAll(const std::string& sContent, bool* pbOpened = nullptr)
{
    const std::wstring sPath = WriteFixture(sContent);

    RtfLex oLex;
    const bool bOpened = oLex.SetSource(sPath);
    if (pbOpened)
        *pbOpened = bOpened;

    std::vector<RtfToken> arTokens;
    if (bOpened)
    {
        for (;;)
        {
            RtfToken oToken = oLex.NextToken();
            if (oToken.Type == RtfToken::Eof)
                break;
            arTokens.push_back(oToken);
        }
        oLex.CloseSource();
    }
    NSFile::CFileBinary::Remove(sPath);
    return arTokens;
}

} // namespace

TEST(RtfLex, SetSourceRejectsFilesUnderFiveBytes)
{
    RtfLex oLex;
    const std::wstring sTooShort = WriteFixture("abc");  // 3 bytes
    EXPECT_FALSE(oLex.SetSource(sTooShort));

    RtfLex oLexMin;
    const std::wstring sExactlyFive = WriteFixture("{\\rtf");  // exactly 5 bytes
    EXPECT_TRUE(oLexMin.SetSource(sExactlyFive));
    oLexMin.CloseSource();

    NSFile::CFileBinary::Remove(sTooShort);
    NSFile::CFileBinary::Remove(sExactlyFive);
}

TEST(RtfLex, GroupsAndHeaderKeyword)
{
    // {\rtf1}
    const std::vector<RtfToken> ar = LexAll("{\\rtf1}");

    ASSERT_EQ(3u, ar.size());
    EXPECT_EQ(RtfToken::GroupStart, ar[0].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("rtf", ar[1].Key);
    EXPECT_TRUE(ar[1].HasParameter);
    EXPECT_EQ(1, ar[1].Parameter);
    EXPECT_EQ(RtfToken::GroupEnd, ar[2].Type);
}

TEST(RtfLex, KeywordParameterAndSpaceConsumption)
{
    // \fs24 Bold -- the space after the parameter is the keyword delimiter
    // and must be consumed, so the run starts at "Bold".
    const std::vector<RtfToken> ar = LexAll("\\fs24 Bold");

    ASSERT_EQ(2u, ar.size());
    ASSERT_EQ(RtfToken::Keyword, ar[0].Type);
    EXPECT_EQ("fs", ar[0].Key);
    EXPECT_TRUE(ar[0].HasParameter);
    EXPECT_EQ(24, ar[0].Parameter);
    ASSERT_EQ(RtfToken::Text, ar[1].Type);
    EXPECT_EQ("Bold", ar[1].Key);
}

TEST(RtfLex, NegativeParameter)
{
    // {\li-100}
    const std::vector<RtfToken> ar = LexAll("{\\li-100}");

    ASSERT_EQ(3u, ar.size());
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("li", ar[1].Key);
    EXPECT_TRUE(ar[1].HasParameter);
    EXPECT_EQ(-100, ar[1].Parameter);
}

TEST(RtfLex, EscapedDelimitersAreText)
{
    // \\ \{ \} (no spaces: the escapes sit next to each other)
    const std::vector<RtfToken> ar = LexAll("\\\\\\{\\}");

    ASSERT_EQ(3u, ar.size());
    ASSERT_EQ(RtfToken::Text, ar[0].Type);
    EXPECT_EQ("\\", ar[0].Key);
    ASSERT_EQ(RtfToken::Text, ar[1].Type);
    EXPECT_EQ("{", ar[1].Key);
    ASSERT_EQ(RtfToken::Text, ar[2].Type);
    EXPECT_EQ("}", ar[2].Key);
}

TEST(RtfLex, ControlSymbolsBecomeKeywords)
{
    // \~ \- \| \_ \:
    const std::vector<RtfToken> ar = LexAll("\\~\\-\\|\\_\\:");

    ASSERT_EQ(5u, ar.size());
    static const char* arKeys[5] = {"~", "-", "|", "_", ":"};
    for (size_t i = 0; i < ar.size(); i++)
    {
        ASSERT_EQ(RtfToken::Keyword, ar[i].Type) << "token " << i;
        EXPECT_EQ(arKeys[i], ar[i].Key) << "token " << i;
        EXPECT_FALSE(ar[i].HasParameter) << "token " << i;
    }
}

TEST(RtfLex, HexEscapeBecomesControlToken)
{
    // \'e9 -> Control token; Key is the decimal code of the apostrophe ("39"),
    // Parameter carries the byte value (0xE9 = 233).
    const std::vector<RtfToken> ar = LexAll("{\\'e9}");

    ASSERT_EQ(3u, ar.size());
    ASSERT_EQ(RtfToken::Control, ar[1].Type);
    EXPECT_EQ("39", ar[1].Key);
    EXPECT_TRUE(ar[1].HasParameter);
    EXPECT_EQ(233, ar[1].Parameter);
}

TEST(RtfLex, HexEscapeControlRangeShift)
{
    // Known quirk, pinned as current behavior: control bytes up to 0x1D are
    // shifted +1 so they survive the round trip, but the <=30 bound in
    // RtfLex.cpp makes 0x1E and 0x1F indistinguishable -- both come back as
    // 31. Do not read this as round-trip intent.
    const std::vector<RtfToken> ar = LexAll("{\\'00\\'1e\\'1f}");

    ASSERT_EQ(5u, ar.size());
    ASSERT_EQ(RtfToken::Control, ar[1].Type);
    EXPECT_EQ(1, ar[1].Parameter);
    ASSERT_EQ(RtfToken::Control, ar[2].Type);
    EXPECT_EQ(31, ar[2].Parameter);  // 0x1E collides with 0x1F (known quirk)
    ASSERT_EQ(RtfToken::Control, ar[3].Type);
    EXPECT_EQ(31, ar[3].Parameter);
}

TEST(RtfLex, BackslashControlCharsBecomeKeywords)
{
    // {\rtf1\<TAB>} -- backslash + literal tab -> keyword "tab"
    const std::vector<RtfToken> arTab = LexAll("{\\rtf1\\\t}");
    ASSERT_EQ(4u, arTab.size());
    EXPECT_EQ(RtfToken::GroupStart, arTab[0].Type);
    ASSERT_EQ(RtfToken::Keyword, arTab[1].Type);
    EXPECT_EQ("rtf", arTab[1].Key);
    ASSERT_EQ(RtfToken::Keyword, arTab[2].Type);
    EXPECT_EQ("tab", arTab[2].Key);
    EXPECT_FALSE(arTab[2].HasParameter);
    EXPECT_EQ(RtfToken::GroupEnd, arTab[3].Type);

    // {\rtf1\<CR>} -- backslash + literal CR -> keyword "par"
    const std::vector<RtfToken> arCr = LexAll("{\\rtf1\\\r}");
    ASSERT_EQ(4u, arCr.size());
    ASSERT_EQ(RtfToken::Keyword, arCr[2].Type);
    EXPECT_EQ("par", arCr[2].Key);

    // {\rtf1\<LF>} -- backslash + literal LF -> keyword "par"
    const std::vector<RtfToken> arLf = LexAll("{\\rtf1\\\n}");
    ASSERT_EQ(4u, arLf.size());
    ASSERT_EQ(RtfToken::Keyword, arLf[2].Type);
    EXPECT_EQ("par", arLf[2].Key);
}

TEST(RtfLex, TextRunsStopAtDelimiters)
{
    // Hello{\b Bold}
    const std::vector<RtfToken> ar = LexAll("Hello{\\b Bold}");

    ASSERT_EQ(5u, ar.size());
    ASSERT_EQ(RtfToken::Text, ar[0].Type);
    EXPECT_EQ("Hello", ar[0].Key);
    EXPECT_EQ(RtfToken::GroupStart, ar[1].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[2].Type);
    EXPECT_EQ("b", ar[2].Key);
    ASSERT_EQ(RtfToken::Text, ar[3].Type);
    EXPECT_EQ("Bold", ar[3].Key);
    EXPECT_EQ(RtfToken::GroupEnd, ar[4].Type);
}

TEST(RtfLex, NewlinesInsideTextRunsAreDropped)
{
    // {\rtf1a\r\nb} -- CR/LF inside a text run are ignored, the run stays
    // whole ("ab").
    const std::vector<RtfToken> ar = LexAll("{\\rtf1a\r\nb}");

    ASSERT_EQ(4u, ar.size());
    EXPECT_EQ(RtfToken::GroupStart, ar[0].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("rtf", ar[1].Key);
    ASSERT_EQ(RtfToken::Text, ar[2].Type);
    EXPECT_EQ("ab", ar[2].Key);
    EXPECT_EQ(RtfToken::GroupEnd, ar[3].Type);
}

TEST(RtfLex, BareTabSurvivesInsideText)
{
    // {\rtf1\tHi} -- a tab not preceded by a backslash is ordinary text.
    const std::vector<RtfToken> ar = LexAll("{\\rtf1\tHi}");

    ASSERT_EQ(4u, ar.size());
    EXPECT_EQ(RtfToken::GroupStart, ar[0].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("rtf", ar[1].Key);
    ASSERT_EQ(RtfToken::Text, ar[2].Type);
    EXPECT_EQ("\tHi", ar[2].Key);
    EXPECT_EQ(RtfToken::GroupEnd, ar[3].Type);
}

TEST(RtfLex, ControlBytesBetweenTokensAreSkipped)
{
    // {\rtf1<0x01><0x02>ab} -- control bytes ahead of the text run are
    // skipped before dispatch.
    const std::string sContent = "{\\rtf1" + std::string("\x01\x02", 2) + "ab}";
    const std::vector<RtfToken> ar = LexAll(sContent);

    ASSERT_EQ(4u, ar.size());
    EXPECT_EQ(RtfToken::GroupStart, ar[0].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("rtf", ar[1].Key);
    ASSERT_EQ(RtfToken::Text, ar[2].Type);
    EXPECT_EQ("ab", ar[2].Key);
    EXPECT_EQ(RtfToken::GroupEnd, ar[3].Type);
}

TEST(RtfLex, UnicodeEscapeFollowedByFallbackText)
{
    // \u8211? -- the classic Unicode escape with its ANSI fallback char:
    // the lexer yields the keyword with the code point, then the fallback
    // character as an ordinary text run.
    const std::vector<RtfToken> ar = LexAll("{\\u8211?}");

    ASSERT_EQ(4u, ar.size());
    EXPECT_EQ(RtfToken::GroupStart, ar[0].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("u", ar[1].Key);
    EXPECT_TRUE(ar[1].HasParameter);
    EXPECT_EQ(8211, ar[1].Parameter);
    ASSERT_EQ(RtfToken::Text, ar[2].Type);
    EXPECT_EQ("?", ar[2].Key);
    EXPECT_EQ(RtfToken::GroupEnd, ar[3].Type);
}

TEST(RtfLex, LongTextRunExceedsInitialReadBuffer)
{
    // SetSource grows the 5 MB read buffer to the file size, so a run past
    // the initial buffer must still lex as one Text token of full length.
    const size_t nLen = 6u * 1024u * 1024u;
    const std::string sContent(nLen, 'x');

    const std::vector<RtfToken> ar = LexAll(sContent);

    ASSERT_EQ(1u, ar.size());
    ASSERT_EQ(RtfToken::Text, ar[0].Type);
    EXPECT_EQ(nLen, ar[0].Key.size());
}

TEST(RtfLex, EofAtEndOfStream)
{
    // {\rtf} -> three real tokens, then the Eof token, which repeats once
    // the stream is exhausted.
    const std::vector<RtfToken> ar = LexAll("{\\rtf}");
    ASSERT_EQ(3u, ar.size());
    EXPECT_EQ(RtfToken::GroupStart, ar[0].Type);
    ASSERT_EQ(RtfToken::Keyword, ar[1].Type);
    EXPECT_EQ("rtf", ar[1].Key);
    EXPECT_EQ(RtfToken::GroupEnd, ar[2].Type);

    RtfLex oLex;
    const std::wstring sPath = WriteFixture("{\\rtf}");
    ASSERT_TRUE(oLex.SetSource(sPath));
    EXPECT_EQ(RtfToken::GroupStart, oLex.NextToken().Type);
    EXPECT_EQ(RtfToken::Keyword, oLex.NextToken().Type);
    EXPECT_EQ(RtfToken::GroupEnd, oLex.NextToken().Type);
    EXPECT_EQ(RtfToken::Eof, oLex.NextToken().Type);  // first Eof
    EXPECT_EQ(RtfToken::Eof, oLex.NextToken().Type);  // Eof persists
    oLex.CloseSource();
    NSFile::CFileBinary::Remove(sPath);
}
