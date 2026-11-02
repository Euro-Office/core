/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "gtest/gtest.h"

#include "../Numbering.h"
#include "../Logic/SectionProperty.h"
#include "../Logic/Run.h"
#include "../Logic/RunContent.h"

namespace
{
	class DocxRunProperty : public ::testing::Test
	{
	protected:
		OOX::Numbering::CLvl level;

		void Read(const std::wstring& properties)
		{
			const std::wstring xml =
				L"<w:lvl xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" "
				L"xmlns:w14=\"http://schemas.microsoft.com/office/word/2010/wordml\" "
				L"xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\" "
				L"xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
				L"xmlns:wx=\"http://schemas.microsoft.com/office/word/2003/auxHint\" "
				L"xmlns:x=\"urn:unknown\" w:ilvl=\"0\"><w:rPr>" + properties + L"</w:rPr></w:lvl>";
			XmlUtils::CXmlLiteReader reader;
			ASSERT_TRUE(reader.FromString(xml));
			ASSERT_TRUE(reader.ReadNextNode());
			level.fromXML(reader);
			ASSERT_TRUE(level.m_oRPr.IsInit());
		}
	};
}

TEST_F(DocxRunProperty, ReadsOrdinaryPropertiesAndUnicodeAttributeValues)
{
	Read(L"<w:b w:val=\"0\"/><w:i/><w:color w:val=\"123456\"/>"
		 L"<w:rFonts w:ascii=\"Noto Sans العربية\"/><w:lang w:val=\"ar-SA\"/>"
		 L"<w:sz w:val=\"28\"/><w:szCs w:val=\"32\"/><w:u w:val=\"single\"/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	const auto& pr = *level.m_oRPr;
	ASSERT_TRUE(pr.m_oBold.IsInit());
	EXPECT_EQ(SimpleTypes::onoffFalse, pr.m_oBold->m_oVal.GetValue());
	ASSERT_TRUE(pr.m_oItalic.IsInit());
	EXPECT_EQ(SimpleTypes::onoffTrue, pr.m_oItalic->m_oVal.GetValue());
	EXPECT_TRUE(pr.m_oColor.IsInit());
	ASSERT_TRUE(pr.m_oRFonts.IsInit());
	ASSERT_TRUE(pr.m_oRFonts->m_sAscii.IsInit());
	EXPECT_EQ(L"Noto Sans العربية", *pr.m_oRFonts->m_sAscii);
	ASSERT_TRUE(pr.m_oLang.IsInit());
	ASSERT_TRUE(pr.m_oLang->m_oVal.IsInit());
	EXPECT_EQ(L"ar-SA", *pr.m_oLang->m_oVal);
	ASSERT_TRUE(pr.m_oSz.IsInit());
	ASSERT_TRUE(pr.m_oSz->m_oVal.IsInit());
	EXPECT_EQ(28, pr.m_oSz->m_oVal->ToHps());
	ASSERT_TRUE(pr.m_oSzCs.IsInit());
	ASSERT_TRUE(pr.m_oSzCs->m_oVal.IsInit());
	EXPECT_EQ(32, pr.m_oSzCs->m_oVal->ToHps());
	EXPECT_TRUE(pr.m_oU.IsInit());
}

TEST_F(DocxRunProperty, DistinguishesQualifiedNamesWithTheSameLocalName)
{
	Read(L"<w:shadow w:val=\"0\"/><w14:shadow blurRad=\"100\"/>"
		 L"<m:oMath/><w:oMath/><w:font w:val=\"Wrong\"/>"
		 L"<wx:font w:val=\"Legacy\"/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	const auto& pr = *level.m_oRPr;
	ASSERT_TRUE(pr.m_oShadow.IsInit());
	EXPECT_EQ(SimpleTypes::onoffFalse, pr.m_oShadow->m_oVal.GetValue());
	EXPECT_TRUE(pr.m_oShadowExt.IsInit());
	EXPECT_TRUE(pr.m_oMath.IsInit());
	ASSERT_TRUE(pr.m_oRFonts.IsInit());
	ASSERT_TRUE(pr.m_oRFonts->m_sAscii.IsInit());
	EXPECT_EQ(L"Legacy", *pr.m_oRFonts->m_sAscii);
}

TEST_F(DocxRunProperty, IgnoresUnknownQualifiedNamesAndTheirChildren)
{
	Read(L"<x:b/><w14:i/><w:oMath/><w:bExtra/>"
		 L"<x:unknown><w:b/><w:i/></x:unknown><w:caps/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	const auto& pr = *level.m_oRPr;
	EXPECT_FALSE(pr.m_oBold.IsInit());
	EXPECT_FALSE(pr.m_oItalic.IsInit());
	EXPECT_FALSE(pr.m_oMath.IsInit());
	EXPECT_TRUE(pr.m_oCaps.IsInit());
}

TEST_F(DocxRunProperty, AcceptsLegacyComplexScriptSizeAlias)
{
	Read(L"<w:sz-cs w:val=\"30\"/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oSzCs.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oSzCs->m_oVal.IsInit());
	EXPECT_EQ(30, level.m_oRPr->m_oSzCs->m_oVal->ToHps());
}

TEST_F(DocxRunProperty, UsesParagraphStyleAliasUntilAnExplicitRunStyle)
{
	Read(L"<w:pStyle w:val=\"Fallback\"/><w:rStyle w:val=\"Explicit\"/>"
		 L"<w:pStyle w:val=\"Ignored\"/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oRStyle.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oRStyle->m_sVal.IsInit());
	EXPECT_EQ(L"Explicit", *level.m_oRPr->m_oRStyle->m_sVal);
}

TEST_F(DocxRunProperty, AcceptsParagraphStyleAliasWithoutRunStyle)
{
	Read(L"<w:pStyle w:val=\"Fallback\"/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oRStyle.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oRStyle->m_sVal.IsInit());
	EXPECT_EQ(L"Fallback", *level.m_oRPr->m_oRStyle->m_sVal);
}

TEST_F(DocxRunProperty, ExplicitFontsTakePrecedenceOverLegacyFontHint)
{
	Read(L"<wx:font w:val=\"Legacy\"/><w:rFonts w:ascii=\"Explicit\"/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oRFonts.IsInit());
	ASSERT_TRUE(level.m_oRPr->m_oRFonts->m_sAscii.IsInit());
	EXPECT_EQ(L"Explicit", *level.m_oRPr->m_oRFonts->m_sAscii);
}

TEST_F(DocxRunProperty, ContinuesAfterNestedTextFillAndStyleSetReaders)
{
	Read(L"<w14:textFill><a:solidFill><a:srgbClr val=\"112233\"/></a:solidFill></w14:textFill>"
		 L"<w14:stylisticSets><w14:stylisticSet w14:id=\"1\" w14:val=\"1\"/>"
		 L"</w14:stylisticSets><w:b/><w:i/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	const auto& pr = *level.m_oRPr;
	EXPECT_EQ(PPTX::Logic::UniFill::solidFill, pr.m_oTextFill.m_type);
	ASSERT_TRUE(pr.m_oStylisticSets.IsInit());
	EXPECT_EQ(1u, pr.m_oStylisticSets->m_arrItems.size());
	EXPECT_TRUE(pr.m_oBold.IsInit());
	EXPECT_TRUE(pr.m_oItalic.IsInit());
}

TEST_F(DocxRunProperty, DoesNotParseRecursivePropertyChanges)
{
	Read(L"<w:rPrChange w:id=\"1\"><w:rPr><w:i/>"
		 L"<w:rPrChange w:id=\"2\"><w:rPr><w:caps/></w:rPr></w:rPrChange>"
		 L"<w:b/></w:rPr></w:rPrChange><w:strike/>");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	const auto& pr = *level.m_oRPr;
	ASSERT_TRUE(pr.m_oRPrChange.IsInit());
	ASSERT_TRUE(pr.m_oRPrChange->m_pRunPr.IsInit());
	const auto& changed = *pr.m_oRPrChange->m_pRunPr;
	EXPECT_TRUE(changed.m_bRPRChange);
	EXPECT_FALSE(changed.m_oRPrChange.IsInit());
	EXPECT_TRUE(changed.m_oItalic.IsInit());
	EXPECT_TRUE(changed.m_oBold.IsInit());
	EXPECT_FALSE(changed.m_oCaps.IsInit());
	EXPECT_TRUE(pr.m_oStrike.IsInit());
}

TEST_F(DocxRunProperty, HandlesEmptyProperties)
{
	Read(L"");
	ASSERT_TRUE(level.m_oRPr.IsInit());
	EXPECT_FALSE(level.m_oRPr->m_oBold.IsInit());
	EXPECT_FALSE(level.m_oRPr->m_oRStyle.IsInit());
}

TEST(DocxRunPropertyReader, ContinuesAfterAPropertyConsumesNestedRunContent)
{
	const std::wstring xml =
		L"<w:r xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
		L"<w:rPr><w:b><w:t>inside</w:t></w:b><w:i/></w:rPr>"
		L"<w:t>after</w:t></w:r>";
	XmlUtils::CXmlLiteReader reader;
	ASSERT_TRUE(reader.FromString(xml));
	ASSERT_TRUE(reader.ReadNextNode());
	OOX::Logic::CRun run;
	run.fromXML(reader);
	ASSERT_NE(nullptr, run.m_oRunProperty);
	EXPECT_TRUE(run.m_oRunProperty->m_oBold.IsInit());
	EXPECT_TRUE(run.m_oRunProperty->m_oItalic.IsInit());
	std::vector<std::wstring> texts;
	for (const auto* item : run.m_arrItems)
	{
		if (item->getType() == OOX::et_w_t)
			texts.push_back(static_cast<const OOX::Logic::CText*>(item)->m_sText);
	}
	ASSERT_EQ(2u, texts.size());
	EXPECT_EQ(L"inside", texts[0]);
	EXPECT_EQ(L"after", texts[1]);
}
