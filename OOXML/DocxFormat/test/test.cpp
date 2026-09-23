#include "gtest/gtest.h"

#include "../Numbering.h"
#include "../Logic/SectionProperty.h"

// Feature 001-docx-list-numbering: DOCX bullets/numbers misclassified by Microsoft 365
// Online. Root cause (research.md Finding 4): OOX::Numbering::CLvl::toXML() emits its
// <w:lvl> children in alphabetical order instead of the ECMA-376 CT_Lvl schema sequence
// (start, numFmt, lvlRestart, pStyle, isLgl, suff, lvlText, lvlPicBulletId, legacy,
// lvlJc, pPr, rPr). This suite asserts the schema order directly against the production
// serializer/reader, for both list types (T004/T005/T007).

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

// T007: the reader must classify numFmt correctly even when fed the pre-fix
// (non-schema-ordered) element order — this is what makes "open and re-save" a
// sufficient repair for already-produced defective files (FR-011), without a
// separate migration tool, and proves FR-004 (reading files not written by the
// current writer).
TEST(DocxNumbering, LvlFromXml_ToleratesPreFixElementOrder_Bullet)
{
	// Exact pre-fix element order reproduced from research.md Finding 4 / the
	// sample file's abstractNumId=0 (bullet) level, byte-for-byte.
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
	// Exact pre-fix element order reproduced from research.md Finding 4 / the
	// sample file's abstractNumId=1 (decimal) level, byte-for-byte.
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

// T012/T017 (user-customised marker): classification must key on numFmt, not on
// lvlText. A customised, unrecognisable lvlText string must not change the result.
TEST(DocxNumbering, LvlToXml_CustomisedMarker_StillKeyedOnNumFmt)
{
	OOX::Numbering::CLvl oBullet = BuildLvl(SimpleTypes::numberformatBullet, L"✦"); // custom glyph
	OOX::Numbering::CLvl oOrdered = BuildLvl(SimpleTypes::numberformatDecimal, L"[[Step %1]]"); // custom format string

	EXPECT_NE(oBullet.toXML().find(L"w:val=\"bullet\""), std::wstring::npos);
	EXPECT_NE(oOrdered.toXML().find(L"w:val=\"decimal\""), std::wstring::npos);
}
