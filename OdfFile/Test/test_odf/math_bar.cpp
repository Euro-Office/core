// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// OMML m:bar -> MathML mover/munder (StarMath::COOXml2Odf, the converter used
// for equations on the way into ODF). The start tag has to be closed before
// the base is written, or the formula object is not well-formed.

#include "gtest/gtest.h"

#include "common.h"

#include "OdfFile/Reader/Converter/StarMath2OOXML/cooxml2odf.h"
#include "DesktopEditor/xml/include/xmlutils.h"

#include <string>

namespace
{
// Parses one <m:oMath> element and converts it the way the ODF writer does.
std::wstring convert_omath(const std::wstring& omath)
{
	XmlUtils::CXmlLiteReader oReader;
	if (!oReader.FromString(omath) || !oReader.ReadNextNode())
		return L"";

	OOX::Logic::COMath oMath;
	oMath.fromXML(oReader);

	StarMath::COOXml2Odf oConverter;
	oConverter.StartConversion(&oMath);
	return oConverter.GetOdf();
}

std::wstring bar_omath(const std::wstring& barPr)
{
	return L"<m:oMath xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"><m:bar>" + barPr +
		   L"<m:e><m:r><m:t>a</m:t></m:r></m:e></m:bar></m:oMath>";
}

// The base ("a") must be the first child of the element, followed by the bar.
void expect_bar(const std::wstring& odf, const std::wstring& startTag, const std::wstring& endTag)
{
	EXPECT_TRUE(is_well_formed(odf)) << odf;

	const size_t start = odf.find(startTag + L"<");
	ASSERT_NE(start, std::wstring::npos) << odf;
	const size_t base = odf.find(L">a<", start);
	const size_t bar = odf.find(L"<mo>", start);
	const size_t end = odf.find(endTag, start);
	ASSERT_NE(base, std::wstring::npos) << odf;
	ASSERT_NE(bar, std::wstring::npos) << odf;
	ASSERT_NE(end, std::wstring::npos) << odf;
	EXPECT_LT(base, bar) << odf;
	EXPECT_LT(bar, end) << odf;
}
} // namespace

TEST(OmmlToMathml, OverbarIsWellFormed)
{
	const std::wstring odf = convert_omath(bar_omath(L"<m:barPr><m:pos m:val=\"top\"/></m:barPr>"));
	expect_bar(odf, L"<mover accent=\"true\">", L"</mover>");
}

TEST(OmmlToMathml, UnderbarIsWellFormed)
{
	const std::wstring odf = convert_omath(bar_omath(L"<m:barPr><m:pos m:val=\"bot\"/></m:barPr>"));
	expect_bar(odf, L"<munder accentunder=\"true\">", L"</munder>");
}

// Without m:barPr the bar goes below the base, as m:pos defaults to "bot"
// (ECMA-376-1 §22.1.2.84) and as Word does. §22.1.2.8 still says "top", a
// leftover from the 1st edition ([MS-OE376] Part 4 §7.1.2.84).
TEST(OmmlToMathml, BarWithoutPropertiesIsWellFormed)
{
	const std::wstring odf = convert_omath(bar_omath(L""));
	expect_bar(odf, L"<munder accentunder=\"true\">", L"</munder>");
}
