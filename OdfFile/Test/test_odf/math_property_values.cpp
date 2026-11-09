// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// OMML -> MathML, the formula converter used for DOCX -> ODT
// (StarMath::COOXml2Odf, as OdfFile/Writer/Converter/MathConverter.cpp calls it).

#include "gtest/gtest.h"

#include "common.h"

#include "DesktopEditor/xml/include/xmlutils.h"
#include "OdfFile/Reader/Converter/StarMath2OOXML/cooxml2odf.h"

#include <string>

namespace
{
    // MathML for one m:oMath element given as its inner XML.
    std::wstring convert_omath(const std::wstring& content)
    {
        const std::wstring xml =
            L"<m:oMath xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\">" + content +
            L"</m:oMath>";

        XmlUtils::CXmlLiteReader oReader;
        EXPECT_TRUE(oReader.FromString(xml));
        EXPECT_TRUE(oReader.ReadNextNode());

        OOX::Logic::COMath oMath;
        oMath.fromXML(oReader);

        StarMath::COOXml2Odf oConverter;
        oConverter.StartConversion(&oMath);
        return oConverter.GetOdf();
    }

    const std::wstring sBase = L"<m:e><m:r><m:t>a</m:t></m:r></m:e>";

    // The formula converts to well-formed MathML that still holds the base "a".
    void expect_converted(const std::wstring& odf)
    {
        EXPECT_TRUE(is_well_formed(odf)) << odf;
        EXPECT_NE(odf.find(L">a<"), std::wstring::npos) << odf;
    }
}

// m:accPr is optional; without it the accent is the default one.
TEST(OmmlToMathml, AccentWithoutProperties)
{
    const std::wstring odf = convert_omath(L"<m:acc>" + sBase + L"</m:acc>");
    EXPECT_TRUE(is_well_formed(odf)) << odf;
    EXPECT_NE(odf.find(L"<mover accent=\"true\">"), std::wstring::npos) << odf;
}

// m:accPr without m:chr keeps the converter's fallback accent.
TEST(OmmlToMathml, AccentPropertiesWithoutCharacter)
{
    const std::wstring odf = convert_omath(L"<m:acc><m:accPr><m:ctrlPr/></m:accPr>" + sBase + L"</m:acc>");
    EXPECT_TRUE(is_well_formed(odf)) << odf;
    EXPECT_NE(odf.find(L"<mover accent=\"true\">"), std::wstring::npos) << odf;
}

TEST(OmmlToMathml, AccentCharacterWithoutValue)
{
    const std::wstring odf = convert_omath(L"<m:acc><m:accPr><m:chr/></m:accPr>" + sBase + L"</m:acc>");
    EXPECT_TRUE(is_well_formed(odf)) << odf;
    EXPECT_NE(odf.find(L"<mover accent=\"true\">"), std::wstring::npos) << odf;
}

TEST(OmmlToMathml, AccentWithCharacter)
{
    const std::wstring odf = convert_omath(L"<m:acc><m:accPr><m:chr m:val=\"&#x303;\"/></m:accPr>" + sBase +
                                           L"</m:acc>");
    EXPECT_TRUE(is_well_formed(odf)) << odf;
    EXPECT_NE(odf.find(L"<mover>"), std::wstring::npos) << odf;
    EXPECT_NE(odf.find(L"\u0303"), std::wstring::npos) << odf;
}

// Property elements whose m:val is missing keep the converter's defaults.
TEST(OmmlToMathml, NaryCharacterWithoutValue)
{
    expect_converted(convert_omath(L"<m:nary><m:naryPr><m:chr/></m:naryPr><m:sub/><m:sup/>" + sBase + L"</m:nary>"));
}

TEST(OmmlToMathml, GroupCharacterWithoutValue)
{
    expect_converted(convert_omath(L"<m:groupChr><m:groupChrPr><m:chr/></m:groupChrPr>" + sBase + L"</m:groupChr>"));
}

TEST(OmmlToMathml, GroupPositionWithoutValue)
{
    expect_converted(convert_omath(L"<m:groupChr><m:groupChrPr><m:pos/></m:groupChrPr>" + sBase + L"</m:groupChr>"));
}

TEST(OmmlToMathml, GroupVerticalJustificationWithoutValue)
{
    expect_converted(convert_omath(L"<m:groupChr><m:groupChrPr><m:vertJc/></m:groupChrPr>" + sBase +
                                   L"</m:groupChr>"));
}

TEST(OmmlToMathml, FractionTypeWithoutValue)
{
    expect_converted(convert_omath(L"<m:f><m:fPr><m:type/></m:fPr><m:num><m:r><m:t>a</m:t></m:r></m:num>"
                                   L"<m:den><m:r><m:t>b</m:t></m:r></m:den></m:f>"));
}

TEST(OmmlToMathml, RunStyleWithoutValue)
{
    expect_converted(convert_omath(L"<m:r><m:rPr><m:sty/></m:rPr><m:t>a</m:t></m:r>"));
}
