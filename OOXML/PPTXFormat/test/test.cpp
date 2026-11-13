/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "gtest/gtest.h"

#include "../FileFactory.h"
#include "../FileMap.h"
#include "../Logic/Pic.h"
#include "../Logic/GraphicFrame.h"
#include "../../DocxFormat/Rels.h"
#include "../../DocxFormat/Media/OleObject.h"
#include "../../Binary/Presentation/BinaryFileReaderWriter.h"
#include "../../Binary/Presentation/XmlWriter.h"
#include "../../../DesktopEditor/common/File.h"
#include "../../../DesktopEditor/common/Directory.h"
#include "../../../DesktopEditor/xml/include/xmlutils.h"

// A linked OLE object (TargetMode="External" + <p:link>) used to be exported as an embedded
// part that did not exist, so PowerPoint asked to repair the file. Each case pins one step.

namespace
{
	const std::wstring c_sOleRelType = L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/oleObject";
	const std::wstring c_sLinkTarget = L"file:///srv/R&D%20share/book.xlsx!Sheet1!R1C1:R2C2";
	const std::wstring c_sLinkTargetXml = L"file:///srv/R&amp;D%20share/book.xlsx!Sheet1!R1C1:R2C2";
	const std::wstring c_sNs = L" xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
							   L" xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
							   L" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"";

	// Smallest valid PNG (1x1), used as the OLE preview image.
	const BYTE c_aPng[] = {
		0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
		0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
		0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
		0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
		0x42, 0x60, 0x82
	};

	void ReadRelationship(const std::wstring& sXml, OOX::Rels::CRelationShip& oRel)
	{
		XmlUtils::CXmlLiteReader oReader;
		oReader.FromString(sXml);
		oReader.ReadNextNode();
		oRel.fromXML(oReader);
	}

	void WriteFile(const std::wstring& sPath, const BYTE* pData, DWORD nSize)
	{
		NSFile::CFileBinary oFile;
		oFile.CreateFileW(sPath);
		oFile.WriteFile(pData, nSize);
		oFile.CloseFile();
	}

	void InitLinkedOle(PPTX::Logic::COLEObject& oOle, bool bAutoUpdate)
	{
		oOle.m_sProgId = std::wstring(L"Excel.Sheet.12");
		oOle.m_OleObjectFile = new OOX::OleObject(NULL, false, false);
		oOle.m_OleObjectFile->set_filename(c_sLinkTarget, true);
		oOle.setLink(bAutoUpdate);
	}

	void WriteAndReadBack(const PPTX::Logic::COLEObject& oSrc, PPTX::Logic::COLEObject& oDst)
	{
		NSBinPptxRW::CBinaryFileWriter oWriter;
		oWriter.StartRecord(0);
		oSrc.toPPTY(&oWriter);
		oWriter.EndRecord();

		NSBinPptxRW::CBinaryFileReader oReader;
		oReader.Init(oWriter.GetBuffer(), 0, (_INT32)oWriter.GetPosition());
		oReader.Skip(1); // record type: fromPPTY starts at the record length
		oDst.fromPPTY(&oReader);
	}

	// Runs CRelsGenerator::WriteImage for one OLE object and returns the written .rels XML.
	std::wstring WriteOleRels(NSCommon::smart_ptr<OOX::OleObject> pOle, NSBinPptxRW::_relsGeneratorInfo& oInfo)
	{
		std::wstring sDir = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());
		std::wstring sPreview = sDir + FILE_SEPARATOR_STR + L"preview.png";
		WriteFile(sPreview, c_aPng, sizeof(c_aPng));

		NSBinPptxRW::CImageManager2 oManager;
		oManager.SetDstFolder(sDir + FILE_SEPARATOR_STR + L"out");

		NSBinPptxRW::CRelsGenerator oRels(&oManager);
		oRels.StartRels();
		std::vector<NSCommon::smart_ptr<OOX::File>> arFiles;
		arFiles.push_back(pOle.smart_dynamic_cast<OOX::File>());
		oInfo = oRels.WriteImage(sPreview, arFiles, L"", L"");
		oRels.CloseRels();

		std::wstring sRelsPath = sDir + FILE_SEPARATOR_STR + L"slide1.xml.rels";
		oRels.SaveRels(sRelsPath);
		std::wstring sXml;
		NSFile::CFileBinary::ReadAllTextUtf8(sRelsPath, sXml);
		NSDirectory::DeleteDirectory(sDir);
		return sXml;
	}

	void InitOlePic(PPTX::Logic::Pic& oPic)
	{
		oPic.nvPicPr.cNvPr.id = 5;
		oPic.nvPicPr.cNvPr.name = L"Object 4";
		oPic.blipFill.blip.Init();
		oPic.blipFill.blip->embed = new OOX::RId(L"rId3");
		oPic.oleObject.Init();
		oPic.oleObject->m_sProgId = std::wstring(L"Excel.Sheet.12");
		oPic.oleObject->m_oId = OOX::RId(L"rId4");
	}

	std::wstring WritePic(PPTX::Logic::Pic& oPic)
	{
		NSBinPptxRW::CXmlWriter oWriter(XMLWRITER_DOC_TYPE_PPTX);
		oPic.toXmlWriter(&oWriter);
		return oWriter.GetXmlString();
	}

	bool Contains(const std::wstring& sText, const std::wstring& sPart)
	{
		return std::wstring::npos != sText.find(sPart);
	}
}

TEST(PptxLinkedOle, FactoryKeepsExternalOleRelationshipAsLink)
{
	OOX::Rels::CRelationShip oRel;
	ReadRelationship(L"<Relationship Id=\"rId4\" Type=\"" + c_sOleRelType + L"\" Target=\"" + c_sLinkTargetXml + L"\" TargetMode=\"External\"/>", oRel);
	ASSERT_TRUE(oRel.IsExternal());

	PPTX::FileMap oMap;
	NSCommon::smart_ptr<OOX::File> pFile = PPTX::FileFactory::CreateFilePPTX(oRel.Target(), oRel, oMap, NULL);
	NSCommon::smart_ptr<OOX::OleObject> pOle = pFile.smart_dynamic_cast<OOX::OleObject>();

	ASSERT_TRUE(pOle.IsInit());
	EXPECT_TRUE(pOle->isLink());
	EXPECT_EQ(c_sLinkTarget, pOle->filename().GetPath());
}

TEST(PptxLinkedOle, FactoryKeepsInternalOleRelationshipEmbedded)
{
	std::wstring sDir = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());
	std::wstring sPayload = sDir + FILE_SEPARATOR_STR + L"oleObject1.bin";
	WriteFile(sPayload, c_aPng, sizeof(c_aPng));

	OOX::Rels::CRelationShip oRel;
	ReadRelationship(L"<Relationship Id=\"rId4\" Type=\"" + c_sOleRelType + L"\" Target=\"../embeddings/oleObject1.bin\"/>", oRel);

	PPTX::FileMap oMap;
	NSCommon::smart_ptr<OOX::File> pFile = PPTX::FileFactory::CreateFilePPTX(OOX::CPath(sPayload), oRel, oMap, NULL);
	NSCommon::smart_ptr<OOX::OleObject> pOle = pFile.smart_dynamic_cast<OOX::OleObject>();

	ASSERT_TRUE(pOle.IsInit());
	EXPECT_FALSE(pOle->isLink());
	NSDirectory::DeleteDirectory(sDir);
}

// sdkjs always writes the OLE type attribute as 0 (embed); the link must survive on the object-file string alone.
TEST(PptxLinkedOle, BinaryRoundTripKeepsLinkWhenTypeSaysEmbed)
{
	PPTX::Logic::COLEObject oSrc;
	InitLinkedOle(oSrc, true);
	oSrc.m_oType = (BYTE)0;

	PPTX::Logic::COLEObject oDst;
	WriteAndReadBack(oSrc, oDst);

	ASSERT_TRUE(oDst.m_OleObjectFile.IsInit());
	EXPECT_TRUE(oDst.m_OleObjectFile->isLink());
	EXPECT_EQ(c_sLinkTarget, oDst.m_OleObjectFile->filename().GetPath());
	ASSERT_TRUE(oDst.m_oType.IsInit());
	EXPECT_EQ(1, (int)oDst.m_oType->GetBYTECode());
	ASSERT_TRUE(oDst.m_oUpdateMode.IsInit());
	EXPECT_EQ(0, (int)oDst.m_oUpdateMode->GetBYTECode());
}

TEST(PptxLinkedOle, BinaryRoundTripKeepsManualUpdate)
{
	PPTX::Logic::COLEObject oSrc;
	InitLinkedOle(oSrc, false);

	PPTX::Logic::COLEObject oDst;
	WriteAndReadBack(oSrc, oDst);

	ASSERT_TRUE(oDst.m_oUpdateMode.IsInit());
	EXPECT_EQ(1, (int)oDst.m_oUpdateMode->GetBYTECode());
}

TEST(PptxLinkedOle, RelsGeneratorWritesExternalLinkTarget)
{
	NSCommon::smart_ptr<OOX::OleObject> pOle(new OOX::OleObject(NULL, false, false));
	pOle->set_filename(c_sLinkTarget, true);

	NSBinPptxRW::_relsGeneratorInfo oInfo;
	std::wstring sXml = WriteOleRels(pOle, oInfo);

	EXPECT_GT(oInfo.nOleRId, 0u);
	EXPECT_TRUE(Contains(sXml, L"Id=\"rId" + std::to_wstring(oInfo.nOleRId) + L"\" Type=\"" + c_sOleRelType + L"\" Target=\"" + c_sLinkTargetXml + L"\" TargetMode=\"External\"")) << sXml;
	EXPECT_FALSE(Contains(sXml, L"embeddings/")) << sXml;
}

// Sessions cached before links were kept carry "maskFile.<ext>" with no payload: no rel may point at it.
TEST(PptxLinkedOle, RelsGeneratorSkipsEmbeddedOleWithoutPayload)
{
	NSCommon::smart_ptr<OOX::OleObject> pOle(new OOX::OleObject(NULL, false, false));
	pOle->set_filename(L"maskFile.xlsx!returns!r12c2:r16c9", false);

	NSBinPptxRW::_relsGeneratorInfo oInfo;
	std::wstring sXml = WriteOleRels(pOle, oInfo);

	EXPECT_EQ(0u, oInfo.nOleRId);
	EXPECT_FALSE(Contains(sXml, c_sOleRelType)) << sXml;
}

TEST(PptxLinkedOle, PicWritesLinkElement)
{
	PPTX::Logic::Pic oAuto;
	InitOlePic(oAuto);
	oAuto.oleObject->setLink(true);
	std::wstring sAuto = WritePic(oAuto);
	EXPECT_TRUE(Contains(sAuto, L"<p:link updateAutomatic=\"1\"/>")) << sAuto;
	EXPECT_FALSE(Contains(sAuto, L"<p:embed/>")) << sAuto;

	PPTX::Logic::Pic oManual;
	InitOlePic(oManual);
	oManual.oleObject->setLink(false);
	std::wstring sManual = WritePic(oManual);
	EXPECT_TRUE(Contains(sManual, L"<p:link/>")) << sManual;
}

TEST(PptxLinkedOle, PicKeepsEmbedForEmbeddedOle)
{
	PPTX::Logic::Pic oPic;
	InitOlePic(oPic);
	std::wstring sXml = WritePic(oPic);
	EXPECT_TRUE(Contains(sXml, L"<p:embed/>")) << sXml;
	EXPECT_FALSE(Contains(sXml, L"<p:link")) << sXml;
}

TEST(PptxLinkedOle, PicWithoutOleRelationshipIsPlainPicture)
{
	PPTX::Logic::Pic oPic;
	InitOlePic(oPic);
	oPic.oleObject->m_oId.reset();
	oPic.oleObject->m_OleObjectFile = new OOX::OleObject(NULL, false, false);
	std::wstring sXml = WritePic(oPic);
	EXPECT_FALSE(Contains(sXml, L"<p:oleObj")) << sXml;
	EXPECT_TRUE(Contains(sXml, L"<p:pic>")) << sXml;
}

TEST(PptxLinkedOle, ReadsLinkFromOleObjNode)
{
	XmlUtils::CXmlNode oAutoNode;
	ASSERT_TRUE(oAutoNode.FromXmlString((L"<p:oleObj" + c_sNs + L" progId=\"Excel.Sheet.12\" r:id=\"rId4\"><p:link updateAutomatic=\"1\"/></p:oleObj>").c_str()));
	PPTX::Logic::Pic oAuto;
	oAuto.fromXMLOle(oAutoNode);
	ASSERT_TRUE(oAuto.oleObject.IsInit());
	ASSERT_TRUE(oAuto.oleObject->m_oType.IsInit());
	EXPECT_EQ(1, (int)oAuto.oleObject->m_oType->GetBYTECode());
	EXPECT_EQ(0, (int)oAuto.oleObject->m_oUpdateMode->GetBYTECode());

	XmlUtils::CXmlNode oManualNode;
	ASSERT_TRUE(oManualNode.FromXmlString((L"<p:oleObj" + c_sNs + L" progId=\"Excel.Sheet.12\" r:id=\"rId4\"><p:link/></p:oleObj>").c_str()));
	PPTX::Logic::Pic oManual;
	oManual.fromXMLOle(oManualNode);
	ASSERT_TRUE(oManual.oleObject->m_oUpdateMode.IsInit());
	EXPECT_EQ(1, (int)oManual.oleObject->m_oUpdateMode->GetBYTECode());

	XmlUtils::CXmlNode oEmbedNode;
	ASSERT_TRUE(oEmbedNode.FromXmlString((L"<p:oleObj" + c_sNs + L" progId=\"Excel.Sheet.12\" r:id=\"rId4\"><p:embed/></p:oleObj>").c_str()));
	PPTX::Logic::Pic oEmbed;
	oEmbed.fromXMLOle(oEmbedNode);
	EXPECT_FALSE(oEmbed.oleObject->m_oType.IsInit());
}

TEST(PptxLinkedOle, ReadsLinkThroughGraphicFrameReader)
{
	XmlUtils::CXmlLiteReader oReader;
	ASSERT_TRUE(oReader.FromString(L"<p:graphicFrame" + c_sNs + L">"
		L"<p:nvGraphicFramePr><p:cNvPr id=\"5\" name=\"Object 4\"/><p:cNvGraphicFramePr/><p:nvPr/></p:nvGraphicFramePr>"
		L"<p:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"914400\" cy=\"914400\"/></p:xfrm>"
		L"<a:graphic><a:graphicData uri=\"http://schemas.openxmlformats.org/presentationml/2006/ole\">"
		L"<p:oleObj progId=\"Excel.Sheet.12\" r:id=\"rId4\"><p:link updateAutomatic=\"1\"/>"
		L"<p:pic><p:nvPicPr><p:cNvPr id=\"5\" name=\"Object 4\"/><p:cNvPicPr/><p:nvPr/></p:nvPicPr>"
		L"<p:blipFill><a:blip r:embed=\"rId3\"/><a:stretch><a:fillRect/></a:stretch></p:blipFill><p:spPr/></p:pic>"
		L"</p:oleObj></a:graphicData></a:graphic></p:graphicFrame>"));
	ASSERT_TRUE(oReader.ReadNextNode());

	PPTX::Logic::GraphicFrame oFrame;
	oFrame.fromXML(oReader);

	ASSERT_TRUE(oFrame.olePic.IsInit());
	ASSERT_TRUE(oFrame.olePic->oleObject.IsInit());
	ASSERT_TRUE(oFrame.olePic->oleObject->m_oType.IsInit());
	EXPECT_EQ(1, (int)oFrame.olePic->oleObject->m_oType->GetBYTECode());
	EXPECT_EQ(0, (int)oFrame.olePic->oleObject->m_oUpdateMode->GetBYTECode());
}
