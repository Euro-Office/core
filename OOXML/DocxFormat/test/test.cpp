/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "gtest/gtest.h"

#include "../Numbering.h"
// Needed for compile-time completeness of OOX::Logic::CParagraphProperty's own
// members (CLvl's implicit copy constructor, triggered by any return-by-value
// of a CLvl, requires this even though this file never names the type
// directly) -- not for link-time symbol resolution, which is why the file can
// still avoid constructing CParagraphProperty/CRunProperty objects itself.
#include "../Logic/SectionProperty.h"

// DOCX bullets/numbers were misclassified by Microsoft 365 Online. Root cause:
// OOX::Numbering::CLvl::toXML() emitted its <w:lvl> children in alphabetical order
// instead of the ECMA-376 CT_Lvl schema sequence (start, numFmt, lvlRestart, pStyle,
// isLgl, suff, lvlText, lvlPicBulletId, legacy, lvlJc, pPr, rPr). This suite asserts
// the schema order directly against the production serializer/reader, for both list
// types.

namespace
{
	OOX::Numbering::CLvl BuildLvl(SimpleTypes::ENumberFormat eFormat, const std::wstring& sLvlText)
	{
		OOX::Numbering::CLvl oLvl;

		oLvl.m_oStart.Init();
		*oLvl.m_oStart = ComplexTypes::Word::CDecimalNumber();
		oLvl.m_oStart->m_oVal = 1;

		oLvl.m_oNumFmt.Init();
		oLvl.m_oNumFmt->m_oVal.Init();
		*oLvl.m_oNumFmt->m_oVal = SimpleTypes::CNumberFormat(eFormat);

		oLvl.m_oIsLgl.Init();
		*oLvl.m_oIsLgl = ComplexTypes::Word::COnOff2();
		oLvl.m_oIsLgl->m_oVal = SimpleTypes::COnOff(SimpleTypes::onoffFalse);

		oLvl.m_oSuffix.Init();
		*oLvl.m_oSuffix = ComplexTypes::Word::CLevelSuffix();
		oLvl.m_oSuffix->m_oVal = SimpleTypes::CLevelSuffix(SimpleTypes::levelsuffixTab);

		oLvl.m_oLvlText.Init();
		*oLvl.m_oLvlText = ComplexTypes::Word::CLevelText();
		oLvl.m_oLvlText->m_sVal = sLvlText;

		oLvl.m_oLvlJc.Init();
		*oLvl.m_oLvlJc = ComplexTypes::Word::CJc();
		oLvl.m_oLvlJc->m_oVal = SimpleTypes::CJc(SimpleTypes::jcLeft);

		return oLvl;
	}

	// Populates all twelve CT_Lvl child elements (BuildLvl above only covers the
	// six most commonly emitted ones), so the schema-order assertion has full
	// sequence coverage instead of skipping whichever fields are absent. Built
	// via fromXML() -- which the pre-fix-order tolerance tests below already
	// exercise successfully -- rather than by default-constructing
	// OOX::Logic::CParagraphProperty/CRunProperty directly: those pull in the
	// PPTXFormat::Logic dependency graph (UniFill, Ln, EffectProperties, ...),
	// whose symbols have hidden visibility inside the shared x2tlib.so and
	// aren't resolvable from a separate test binary linked against it.
	OOX::Numbering::CLvl BuildFullyPopulatedLvl()
	{
		std::wstring sXml =
			L"<w:lvl w:ilvl=\"0\">"
			L"<w:start w:val=\"1\"/>"
			L"<w:numFmt w:val=\"decimal\"/>"
			L"<w:lvlRestart w:val=\"1\"/>"
			L"<w:pStyle w:val=\"ListParagraph\"/>"
			L"<w:isLgl w:val=\"false\"/>"
			L"<w:suff w:val=\"tab\"/>"
			L"<w:lvlText w:val=\"%1.\"/>"
			L"<w:lvlPicBulletId w:val=\"0\"/>"
			L"<w:legacy w:legacy=\"1\" w:legacyIndent=\"0\" w:legacySpace=\"0\"/>"
			L"<w:lvlJc w:val=\"left\"/>"
			L"<w:pPr><w:ind w:left=\"0\"/></w:pPr>"
			L"<w:rPr><w:sz w:val=\"24\"/></w:rPr>"
			L"</w:lvl>";

		XmlUtils::CXmlLiteReader oReader;
		oReader.FromString(sXml);
		oReader.ReadNextNode();

		OOX::Numbering::CLvl oLvl;
		oLvl.fromXML(oReader);
		return oLvl;
	}

	// Returns, for each ECMA-376 CT_Lvl element name (in schema order), its first
	// occurrence offset in the serialized XML, or std::wstring::npos if absent.
	std::vector<size_t> SchemaOrderPositions(const std::wstring& sXml)
	{
		static const wchar_t* schemaOrderTags[] = {
			L"<w:start ", L"<w:numFmt ", L"<w:lvlRestart ", L"<w:pStyle ",
			L"<w:isLgl ", L"<w:suff ", L"<w:lvlText ", L"<w:lvlPicBulletId ",
			L"<w:legacy ", L"<w:lvlJc ", L"<w:pPr", L"<w:rPr"
		};
		std::vector<size_t> positions;
		for (const wchar_t* tag : schemaOrderTags)
			positions.push_back(sXml.find(tag));
		return positions;
	}
}

TEST(DocxNumbering, LvlToXml_BulletLevel_ElementsInSchemaOrder)
{
	OOX::Numbering::CLvl oLvl = BuildLvl(SimpleTypes::numberformatBullet, L"·");
	std::wstring sXml = oLvl.toXML();

	std::vector<size_t> positions = SchemaOrderPositions(sXml);
	std::vector<size_t> present;
	for (size_t pos : positions)
		if (pos != std::wstring::npos)
			present.push_back(pos);

	// Every present element must appear in non-decreasing schema-sequence order.
	ASSERT_FALSE(present.empty()) << "no recognised <w:lvl> child elements were emitted";
	for (size_t i = 1; i < present.size(); ++i)
	{
		EXPECT_LT(present[i - 1], present[i])
			<< "element order violates ECMA-376 CT_Lvl schema sequence; got XML: "
			<< std::string(sXml.begin(), sXml.end());
	}

	EXPECT_NE(sXml.find(L"w:val=\"bullet\""), std::wstring::npos);
}

TEST(DocxNumbering, LvlToXml_DecimalLevel_ElementsInSchemaOrder)
{
	OOX::Numbering::CLvl oLvl = BuildLvl(SimpleTypes::numberformatDecimal, L"%1.");
	std::wstring sXml = oLvl.toXML();

	std::vector<size_t> positions = SchemaOrderPositions(sXml);
	std::vector<size_t> present;
	for (size_t pos : positions)
		if (pos != std::wstring::npos)
			present.push_back(pos);

	ASSERT_FALSE(present.empty()) << "no recognised <w:lvl> child elements were emitted";
	for (size_t i = 1; i < present.size(); ++i)
	{
		EXPECT_LT(present[i - 1], present[i])
			<< "element order violates ECMA-376 CT_Lvl schema sequence; got XML: "
			<< std::string(sXml.begin(), sXml.end());
	}

	EXPECT_NE(sXml.find(L"w:val=\"decimal\""), std::wstring::npos);
}

// The reader must classify numFmt correctly even when fed the pre-fix
// (non-schema-ordered) element order — this is what makes "open and re-save" a
// sufficient repair for already-produced defective files, without a separate
// migration tool, and confirms reading files not written by the current writer
// still works.
TEST(DocxNumbering, LvlFromXml_ToleratesPreFixElementOrder_Bullet)
{
	// Exact pre-fix element order, reproduced byte-for-byte from a real sample
	// file's abstractNumId=0 (bullet) level.
	std::wstring sXml =
		L"<w:lvl w:ilvl=\"0\">"
		L"<w:isLgl w:val=\"false\"/>"
		L"<w:lvlJc w:val=\"left\"/>"
		L"<w:lvlText w:val=\"·\"/>"
		L"<w:numFmt w:val=\"bullet\"/>"
		L"<w:start w:val=\"1\"/>"
		L"<w:suff w:val=\"tab\"/>"
		L"</w:lvl>";

	XmlUtils::CXmlLiteReader oReader;
	ASSERT_TRUE(oReader.FromString(sXml));
	ASSERT_TRUE(oReader.ReadNextNode());

	OOX::Numbering::CLvl oLvl;
	oLvl.fromXML(oReader);

	ASSERT_TRUE(oLvl.m_oNumFmt.IsInit());
	ASSERT_TRUE(oLvl.m_oNumFmt->m_oVal.IsInit());
	EXPECT_EQ(oLvl.m_oNumFmt->m_oVal->GetValue(), SimpleTypes::numberformatBullet);
}

TEST(DocxNumbering, LvlFromXml_ToleratesPreFixElementOrder_Decimal)
{
	// Exact pre-fix element order, reproduced byte-for-byte from a real sample
	// file's abstractNumId=1 (decimal) level.
	std::wstring sXml =
		L"<w:lvl w:ilvl=\"0\">"
		L"<w:isLgl w:val=\"false\"/>"
		L"<w:lvlJc w:val=\"left\"/>"
		L"<w:lvlText w:val=\"%1.\"/>"
		L"<w:numFmt w:val=\"decimal\"/>"
		L"<w:start w:val=\"1\"/>"
		L"<w:suff w:val=\"tab\"/>"
		L"</w:lvl>";

	XmlUtils::CXmlLiteReader oReader;
	ASSERT_TRUE(oReader.FromString(sXml));
	ASSERT_TRUE(oReader.ReadNextNode());

	OOX::Numbering::CLvl oLvl;
	oLvl.fromXML(oReader);

	ASSERT_TRUE(oLvl.m_oNumFmt.IsInit());
	ASSERT_TRUE(oLvl.m_oNumFmt->m_oVal.IsInit());
	EXPECT_EQ(oLvl.m_oNumFmt->m_oVal->GetValue(), SimpleTypes::numberformatDecimal);
}

// User-customised marker: classification must key on numFmt, not on lvlText. A
// customised, unrecognisable lvlText string must not change the result.
TEST(DocxNumbering, LvlToXml_CustomisedMarker_StillKeyedOnNumFmt)
{
	OOX::Numbering::CLvl oBullet = BuildLvl(SimpleTypes::numberformatBullet, L"✦"); // custom glyph
	OOX::Numbering::CLvl oOrdered = BuildLvl(SimpleTypes::numberformatDecimal, L"[[Step %1]]"); // custom format string

	EXPECT_NE(oBullet.toXML().find(L"w:val=\"bullet\""), std::wstring::npos);
	EXPECT_NE(oOrdered.toXML().find(L"w:val=\"decimal\""), std::wstring::npos);
}

TEST(DocxNumbering, LvlToXml_FullyPopulatedLevel_ElementsInSchemaOrder)
{
	OOX::Numbering::CLvl oLvl = BuildFullyPopulatedLvl();
	std::wstring sXml = oLvl.toXML();

	std::vector<size_t> positions = SchemaOrderPositions(sXml);
	for (size_t pos : positions)
		ASSERT_NE(pos, std::wstring::npos) << "fully populated level did not emit every CT_Lvl child element";

	for (size_t i = 1; i < positions.size(); ++i)
	{
		EXPECT_LT(positions[i - 1], positions[i])
			<< "element order violates ECMA-376 CT_Lvl schema sequence; got XML: "
			<< std::string(sXml.begin(), sXml.end());
	}
}
