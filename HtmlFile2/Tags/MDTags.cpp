#include "MDTags.h"

#include "../DesktopEditor/xml/include/xmlutils.h"

#include "../src/StringFinder.h"
#include "../Table.h"
#include "../Common/3dParty/html/css/src/CCompiledStyle.h"

#include "../../Common/Network/FileTransporter/include/FileTransporter.h"

#include "../../DesktopEditor/common/Base64.h"
#include "../../DesktopEditor/common/Path.h"
#include "../../DesktopEditor/common/Directory.h"

#include "../../DesktopEditor/graphics/pro/Graphics.h"
#include "../../DesktopEditor/raster/BgraFrame.h"

#include <queue>

namespace HTML
{
template<>
bool CAnchorTag<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!Valid())
		return false;

	m_pWriter->WriteString(L"[");
	return true;
}

template<>
void CAnchorTag<CMDWriter>::Close(const NSCSS::CNode& oTagNode)
{
	if (!Valid())
		return;

	m_pWriter->WriteString(L"]");

	std::wstring wsHref, wsTitle;

	oTagNode.GetAttributeValue(L"href", wsHref);
	oTagNode.GetAttributeValue(L"title", wsTitle);

	m_pWriter->WriteString(L'(' + wsHref);

	if (!wsTitle.empty())
		m_pWriter->WriteString(L" \"" + wsTitle + L'"');

	m_pWriter->WriteString(L")");
}

template<>
bool CBreakTag<CMDWriter>::Read(const NSCSS::CNode& oTagNode)
{
	if (!Valid())
		return false;

	m_pWriter->WriteBreakLine();

	return true;
}

template<>
bool CPreformattedTag<CMDWriter>::Open()
{
	if (!Valid())
		return false;

	m_pWriter->WriteOpenSpecialString(L"```");
	m_pWriter->EnteredPreformatted();

	return true;
}

template<>
void CPreformattedTag<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!Valid())
		return;

	bool bNeedBreakLine{false};

	for (std::vector<NSCSS::CNode>::const_reverse_iterator itElement{arSelectors.crbegin()}; itElement < arSelectors.crend(); ++itElement)
	{
		if (L"pre" == itElement->m_wsName)
		{
			bNeedBreakLine = true;
			break;
		}
	}

	if (bNeedBreakLine && !m_pWriter->InTable())
		m_pWriter->WriteBreakLine();

	m_pWriter->WriteCloseSpecialString(L"```");
	m_pWriter->OutPreformatted();

	if (bNeedBreakLine && !m_pWriter->InTable())
		m_pWriter->WriteBreakLine(false);
}

template<>
bool CHeaderTag<CMDWriter>::Open(const NSCSS::CNode& oTagNode)
{
	if (!Valid())
		return false;

	switch(oTagNode.m_wsName[1])
	{
		case L'1' : m_pWriter->WriteString(L"# ",      true); break;
		case L'2' : m_pWriter->WriteString(L"## ",     true); break;
		case L'3' : m_pWriter->WriteString(L"### ",    true); break;
		case L'4' : m_pWriter->WriteString(L"#### ",   true); break;
		case L'5' : m_pWriter->WriteString(L"##### ",  true); break;
		case L'6' : m_pWriter->WriteString(L"###### ", true); break;
		default:
			return false;
	}

	return true;
}

template<>
void CHeaderTag<CMDWriter>::Close()
{
	if (Valid())
		m_pWriter->WriteBreakLine();
}

bool ConvertBufferToBase64(BYTE* pBuffer, size_t unSize, std::wstring& wsBase64)
{
	if (nullptr == pBuffer || 0 == unSize)
		return false;

	int nDecodeSize{NSBase64::Base64EncodeGetRequiredLength(unSize)};

	BYTE* pBase64Buffer = new BYTE[nDecodeSize];

	if (nullptr == pBase64Buffer)
		return false;

	if (FALSE == NSBase64::Base64Encode(pBuffer, unSize, pBase64Buffer, &nDecodeSize, 2))
	{
		RELEASEARRAYOBJECTS(pBase64Buffer);
		return false;
	}

	wsBase64.reserve(22 + nDecodeSize);

	wsBase64 = L"data:image/png;base64,";
	wsBase64.append(pBase64Buffer, pBase64Buffer + nDecodeSize);

	RELEASEARRAYOBJECTS(pBase64Buffer);

	return true;
}

bool ConvertFromMetafileBase(MetaFile::IMetaFile* pMetafileReader, UINT unWidth, UINT unHeight, NSFonts::IApplicationFonts* pFonts, const std::wstring& wsTempDir, std::wstring& wsResultBase64)
{
	NSGraphics::IGraphicsRenderer* pGrRenderer = NSGraphics::Create();
	pGrRenderer->SetFontManager(pMetafileReader->get_FontManager());

	double dX, dY, dW, dH;
	pMetafileReader->GetBounds(&dX, &dY, &dW, &dH);

	if (dW < 0) dW = -dW;
	if (dH < 0) dH = -dH;

	double dOneMaxSize = (double)1000.;

	if (dW > dH && dW > dOneMaxSize)
	{
		dH *= (dOneMaxSize / dW);
		dW = dOneMaxSize;
	}
	else if (dH > dW && dH > dOneMaxSize)
	{
		dW *= (dOneMaxSize / dH);
		dH = dOneMaxSize;
	}

	if (0 == unWidth && 0 == unHeight)
	{
		unWidth  = (int)(dW * 96 / 25.4);
		unHeight = (int)(dH * 96 / 25.4);
	}
	else if (0 == unWidth)
		unWidth = (int)((double)unHeight * dW / dH);
	else if (0 == unHeight)
		unHeight = (int)((double)unWidth * dH / dW);

	double dWidth  = 25.4 * unWidth / 96;
	double dHeight = 25.4 * unHeight / 96;

	BYTE* pBgraData = (BYTE*)malloc(unWidth * unHeight * 4);
	if (!pBgraData)
	{
		double dKoef = 2000.0 / (unWidth > unHeight ? unWidth : unHeight);

		unWidth = (int)(dKoef * unWidth);
		unHeight = (int)(dKoef * unWidth);

		dWidth  = 25.4 * unWidth / 96;
		dHeight = 25.4 * unHeight / 96;

		pBgraData = (BYTE*)malloc(unWidth * unHeight * 4);
	}

	if (!pBgraData)
		return false;

	unsigned int alfa = 0xffffff;
	//дефолтный тон должен быть прозрачным, а не белым
	//memset(pBgraData, 0xff, nWidth * nHeight * 4);
	for (int i = 0; i < unWidth * unHeight; i++)
		((unsigned int*)pBgraData)[i] = alfa;

	CBgraFrame oFrame;
	oFrame.put_Data(pBgraData);
	oFrame.put_Width(unWidth);
	oFrame.put_Height(unHeight);
	oFrame.put_Stride(-4 * unWidth);

	pGrRenderer->CreateFromBgraFrame(&oFrame);
	pGrRenderer->SetSwapRGB(false);
	pGrRenderer->put_Width(dWidth);
	pGrRenderer->put_Height(dHeight);

	pMetafileReader->SetTempDirectory(wsTempDir);
	pMetafileReader->DrawOnRenderer(pGrRenderer, 0, 0, dWidth, dHeight);

	BYTE *pImageBuffer{nullptr};
	int nImageSize;

	bool bResult{false};

	if (oFrame.Encode(pImageBuffer, nImageSize, 4))
	{
		bResult = ConvertBufferToBase64(pImageBuffer, nImageSize, wsResultBase64);
		RELEASEARRAYOBJECTS(pImageBuffer);
	}

	RELEASEINTERFACE(pGrRenderer);

	return bResult;
}

bool ConvertFromMetafile(const std::wstring& wsFilePath, UINT unWidth, UINT unHeight, NSFonts::IApplicationFonts* pFonts, const std::wstring& wsTempDir, std::wstring& wsResultBase64)
{
	MetaFile::IMetaFile* pMetafileReader = MetaFile::Create(pFonts);

	if (!pMetafileReader->LoadFromFile(wsFilePath.c_str()))
	{
		RELEASEINTERFACE(pMetafileReader);
		return false;
	}

	const bool bResult{ConvertFromMetafileBase(pMetafileReader, unWidth, unHeight, pFonts, wsTempDir, wsResultBase64)};

	RELEASEINTERFACE(pMetafileReader);

	return bResult;
}

bool ConvertFromSVG(const std::wstring& wsSVG, UINT unWidth, UINT unHeight, NSFonts::IApplicationFonts* pFonts, const std::wstring& wsTempDir, std::wstring& wsResultBase64)
{
	MetaFile::IMetaFile* pSvgReader = MetaFile::Create(pFonts);
	if (!pSvgReader->LoadFromString(wsSVG))
	{
		RELEASEINTERFACE(pSvgReader);
		return false;
	}

	const bool bResult{ConvertFromMetafileBase(pSvgReader, unWidth, unHeight, pFonts, wsTempDir, wsResultBase64)};

	RELEASEINTERFACE(pSvgReader);

	return bResult;
}

bool IsSVGExtention(const std::wstring& wsExtention)
{
	return L"svg" == wsExtention || L"svg+xml" == wsExtention;
}

bool IsMetafileExtention(const std::wstring& wsExtention)
{
	return L"emf" == wsExtention || L"wmf" == wsExtention;
}

bool ConvertFromBase64(const std::wstring& wsBase64, UINT unWidth, UINT unHeight, NSFonts::IApplicationFonts* pFonts, const std::wstring& wsTempDir, std::wstring& wsResultBase64)
{
	const size_t unStartExtention{wsBase64.find(L"/", 4)};
	if (unStartExtention == std::wstring::npos)
		return false;

	const size_t unEndExtention{wsBase64.find(L";", unStartExtention)};
	if (unEndExtention == std::wstring::npos)
		return false;

	const size_t nBase = wsBase64.find(L"base64", unEndExtention);
	if (nBase == std::wstring::npos)
		return false;

	bool bResult{false};

	const int nOffset = nBase + 7; //Skip "base64,"
	int nSrcLen = (int)(wsBase64.length() - nBase + 1);
	int nDecodeLen = NSBase64::Base64DecodeGetRequiredLength(nSrcLen);

	if (nDecodeLen != 0)
	{
		BYTE* pImageData = new BYTE[nDecodeLen];

		if (!pImageData || FALSE == NSBase64::Base64Decode(wsBase64.c_str() + nOffset, nSrcLen, pImageData, &nDecodeLen))
			return false;

		if (IsSVGExtention(wsBase64.substr(unStartExtention + 1, unEndExtention - unStartExtention - 1)))
		{
			const std::wstring wsSvg(pImageData, pImageData + nDecodeLen);

			RELEASEARRAYOBJECTS(pImageData);

			bResult = ConvertFromSVG(wsSvg, unWidth, unHeight, pFonts, wsTempDir, wsResultBase64);
		}
		else
		{
			CBgraFrame oFrame;
			oFrame.Decode(pImageData, nDecodeLen);

			RELEASEARRAYOBJECTS(pImageData);

			oFrame.Resize(unWidth, unHeight);

			BYTE *pBuffer{nullptr};
			int nEncodeSize{0};

			oFrame.Encode(pBuffer, nEncodeSize, 4);

			bResult = ConvertBufferToBase64(pBuffer, nEncodeSize, wsResultBase64);
		}
	}

	return bResult;
}

bool ConvertFileToBase64(const std::wstring& wsFilePath, UINT unWidth, UINT unHeight, bool bIsAllowExternalLocalFiles, NSFonts::IApplicationFonts* pFonts, const std::wstring& wsTempDir, std::wstring& wsBase64)
{
	if (wsFilePath.empty() || !NSFile::CFileBinary::Exists(wsFilePath))
		return false;

	const std::wstring wsExtention{NSFile::GetFileExtention(wsFilePath)};

	if (IsSVGExtention(wsExtention))
	{
		std::wstring wsSVG;

		if (!NSFile::CFileBinary::ReadAllTextUtf8(wsFilePath, wsSVG))
			return false;

		return ConvertFromSVG(wsSVG, unWidth, unHeight, pFonts, wsTempDir, wsBase64);
	}
	else if(IsMetafileExtention(wsExtention))
		return ConvertFromMetafile(wsFilePath, unWidth, unHeight, pFonts, wsTempDir, wsBase64);

	CBgraFrame oFrame;

	if (!oFrame.OpenFile(wsFilePath))
		return false;

	if (!oFrame.Resize(unWidth, unHeight))
		return false;

	BYTE *pBuffer{nullptr};
	int nEncodeSize{0};

	if (!oFrame.Encode(pBuffer, nEncodeSize, 4))
		return false;

	const bool bResult{ConvertBufferToBase64(pBuffer, nEncodeSize, wsBase64)};

	RELEASEARRAYOBJECTS(pBuffer);

	return bResult;
}

template<>
bool CImageTag<CMDWriter>::Read(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!Valid())
		return false;

	std::wstring wsAlt, wsSrc, wsTitle;

	if (!arSelectors.back().GetAttributeValue(L"src", wsSrc) &&
	    !arSelectors.back().GetAttributeValue(L"alt", wsAlt))
		return false;

	NSCSS::NSProperties::CDigit oWidth{arSelectors.back().m_pCompiledStyle->m_oDisplay.GetWidth()};
	NSCSS::NSProperties::CDigit oHeight{arSelectors.back().m_pCompiledStyle->m_oDisplay.GetHeight()};

	std::wstring wsValue;

	if (arSelectors.back().GetAttributeValue(L"width", wsValue))
		oWidth.SetValue(wsValue);

	if (arSelectors.back().GetAttributeValue(L"height", wsValue))
		oHeight.SetValue(wsValue);

	const bool bNeedSetSize{(!oWidth.Empty() && !oWidth.Zero()) || (!oHeight.Empty() && !oHeight.Zero())};
	bool bResult{false};
	std::wstring wsResultBase64;

	// Предполагаем картинку в Base64
	if (wsSrc.length() > 4 && wsSrc.substr(0, 4) == L"data" && wsSrc.find(L"/", 4) != std::wstring::npos)
	{
		if (bNeedSetSize)
			bResult = ConvertFromBase64(wsSrc, oWidth.ToInt(NSCSS::UnitMeasure::Pixel), oHeight.ToInt(NSCSS::UnitMeasure::Pixel), m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsResultBase64);
		else
			bResult = true;
	}

	if (!bResult && (wsSrc.length() <= 7 || L"http" != wsSrc.substr(0, 4)))
	{
		wsSrc = NSSystemPath::ShortenPath(wsSrc);

		if (!CanUseThisPath(wsSrc, m_pWriter->GetSrcPath(), m_pWriter->GetCorePath(), GetStatusUsingExternalLocalFiles()))
		{
			wsSrc.clear();
			bResult = true;
		}
	}

	const std::wstring wsBasePath{m_pWriter->GetBasePath()};

	// Предполагаем картинку в сети
	if (!bResult &&
	    ((!wsBasePath.empty() && wsBasePath.length() > 4 && wsBasePath.substr(0, 4) == L"http") ||
	      (wsSrc.length() > 4 && wsSrc.substr(0, 4) == L"http")))
	{
		const std::wstring wsDst{NSFile::CFileBinary::CreateTempFileWithUniqueName(m_pWriter->GetTempDir(), L"IMG")};

		// Проверка gc_allowNetworkRequest предполагается в kernel_network
		NSNetwork::NSFileTransport::CFileDownloader oDownloadImg(m_pWriter->GetBasePath() + wsSrc, false);
		oDownloadImg.SetFilePath(wsDst);
		bResult = oDownloadImg.DownloadSync();

		if (!bResult)
		{
			bResult = true;
			wsSrc.clear();
		}

		if (IsSVGExtention(NSFile::GetFileExtention(wsSrc)))
		{
			std::wstring wsFileData;

			if (NSFile::CFileBinary::ReadAllTextUtf8(wsDst, wsFileData) &&
			    ConvertFromSVG(wsFileData, oWidth.ToInt(NSCSS::UnitMeasure::Pixel), oHeight.ToInt(NSCSS::UnitMeasure::Pixel), m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsResultBase64))
				bResult = true;

			NSFile::CFileBinary::Remove(wsDst);
		}
	}

	// Предполагаем картинку по локальному пути
	if (!bResult)
	{
		const bool bIsAllowExternalLocalFiles{GetStatusUsingExternalLocalFiles()};

		if (!m_pWriter->GetBasePath().empty())
		{
			if (!bResult)
				bResult = ConvertFileToBase64(NSSystemPath::Combine(m_pWriter->GetBasePath(), wsSrc), oWidth.ToInt(NSCSS::UnitMeasure::Pixel), oHeight.ToInt(NSCSS::UnitMeasure::Pixel), bIsAllowExternalLocalFiles, m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsResultBase64);
			if (!bResult)
				bResult = ConvertFileToBase64(NSSystemPath::Combine(m_pWriter->GetSrcPath(), NSSystemPath::Combine(m_pWriter->GetBasePath(), wsSrc)), oWidth.ToInt(NSCSS::UnitMeasure::Pixel), oHeight.ToInt(NSCSS::UnitMeasure::Pixel), bIsAllowExternalLocalFiles, m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsResultBase64);
		}
		if (!bResult)
			bResult = ConvertFileToBase64(NSSystemPath::Combine(m_pWriter->GetSrcPath(), wsSrc), oWidth.ToInt(NSCSS::UnitMeasure::Pixel), oHeight.ToInt(NSCSS::UnitMeasure::Pixel), bIsAllowExternalLocalFiles, m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsResultBase64);
		if (!bResult)
			bResult = ConvertFileToBase64(wsSrc, oWidth.ToInt(NSCSS::UnitMeasure::Pixel), oHeight.ToInt(NSCSS::UnitMeasure::Pixel), bIsAllowExternalLocalFiles, m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsResultBase64);
	}

	arSelectors.back().GetAttributeValue(L"title", wsTitle);

	m_pWriter->WriteString(L"![" + wsAlt + L"](" + ((bResult) ? wsResultBase64 : wsSrc));

	if (!wsTitle.empty())
		m_pWriter->WriteString(L" \"" + wsTitle + L'"');

	m_pWriter->WriteString(L")");

	return true;
}

template<>
bool CImageTag<CMDWriter>::ReadSVG(const std::vector<NSCSS::CNode>& arSelectors, const std::wstring& wsSVG)
{
	if (!Valid())
		return false;

	std::wstring wsBase64;

	if (!ConvertFromSVG(wsSVG, 0, 0, m_pWriter->GetFonts(), m_pWriter->GetTempDir(), wsBase64))
		return false;

	m_pWriter->WriteString(L"![](" + wsBase64 + L")");

	return true;
}

template<>
bool CHorizontalRuleTag<CMDWriter>::Write(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!Valid())
		return false;

	m_pWriter->WriteBreakLine(false);
	m_pWriter->WriteString(L"---");
	m_pWriter->WriteBreakLine(false);

	return true;
}

template<>
bool CBlockquoteTag<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!Valid())
		return false;

	m_pWriter->WriteBreakLine();
	m_pWriter->EnteredBlockquote();

	return true;
}

template<>
void CBlockquoteTag<CMDWriter>::Close()
{
	if (!Valid())
		return;

	m_pWriter->OutBlockquote();
	m_pWriter->WriteBreakLine();
	m_pWriter->WriteBreakLine(false);
}

template<>
bool CListTag<CMDWriter>::Open(const NSCSS::CNode& oTagNode)
{
	if (!Valid())
		return false;

	m_pWriter->WriteBreakLine();
	m_pWriter->EnteredList(L"ol" == oTagNode.m_wsName);

	if (!m_pWriter->InOrederedList())
		return true;

	std::wstring wsIndex;

	if (oTagNode.GetAttributeValue(L"start", wsIndex))
		m_pWriter->SetIndexOrderedList(NSStringFinder::ToInt(wsIndex, 1));

	return true;

	return true;
}

template<>
void CListTag<CMDWriter>::Close()
{
	if (!Valid())
		return;

	m_pWriter->OutList();
}

template<>
bool CListElementTag<CMDWriter>::Open()
{
	if (!Valid())
		return false;

	if (0 !=  m_pWriter->GetLevelList())
	{
		for (UINT unLevelList = 0; unLevelList < m_pWriter->GetLevelList() - 1; ++unLevelList)
			m_pWriter->WriteString(L"  ");
	}

	if (m_pWriter->InOrederedList())
	{
		m_pWriter->WriteString(std::to_wstring(m_pWriter->GetIndexOrderedList()) + m_pWriter->GetParametrs().m_wchOrderedList + L' ');
		m_pWriter->IncreaseIndexOrderedList();
	}
	else
		m_pWriter->WriteString({m_pWriter->GetParametrs().m_wchUnorderedList, L' '});

	return true;
}

template<>
void CListElementTag<CMDWriter>::Close()
{}

template<>
bool CCodeTag<CMDWriter>::Open(const NSCSS::CNode& oTagNode)
{
	if (!Valid())
		return false;

	m_pWriter->EnteredCode();

	if (m_pWriter->InPreformatted())
	{
		if (!oTagNode.m_wsClass.empty() && oTagNode.m_wsClass.size() >= 9 &&
		    0 == oTagNode.m_wsClass.compare(0, 9, L"language-"))
			m_pWriter->WriteString(oTagNode.m_wsClass.substr(9, oTagNode.m_wsClass.size() - 9));
		m_pWriter->WriteBreakLine(false);
	}
	else
		m_pWriter->WriteOpenSpecialString(L"`");

	return true;
}

template<>
void CCodeTag<CMDWriter>::Close()
{
	if (!Valid())
		return;

	if (!m_pWriter->InPreformatted())
		m_pWriter->WriteCloseSpecialString(L"`");

	m_pWriter->OutCode();
}

bool IsTable(const ITableElementCell* pCell)
{
	return nullptr != pCell && ETableElement::Table == pCell->GetType();
}

CMarkdownTable::CMarkdownTable(TExternalTableData &oExternalData)
	: CTableElement(oExternalData)
{}

CMarkdownTable::~CMarkdownTable()
{}

bool CMarkdownTable::PreParse(XmlUtils::CXmlLiteReader& oReader)
{
	return ParseTable(oReader, this);
}

void CMarkdownTable::Normalize()
{
	#define FLATTEN_TABLE(table_variable) table_variable.GetMatrixCells() = Flatten(std::move(table_variable .GetMatrixCells()))

	FLATTEN_TABLE(m_oHeader );
	FLATTEN_TABLE(m_oBody   );
	FLATTEN_TABLE(m_oFoother);

	size_t unMaxColumns{m_oBody.GetColumnSize()};

	if (!m_oHeader.Empty())
		unMaxColumns = std::max(unMaxColumns, m_oHeader.GetColumnSize());

	if (!m_oFoother.Empty())
		unMaxColumns = std::max(unMaxColumns, m_oFoother.GetColumnSize());

	#define NORMALIZE_NUMBER_COLUMN(table_variable)\
	if (!table_variable.Empty() && unMaxColumns != table_variable.GetColumnSize())\
		table_variable.NormalizeNumberColumns(unMaxColumns)

	NORMALIZE_NUMBER_COLUMN(m_oHeader);
	NORMALIZE_NUMBER_COLUMN(m_oBody);
	NORMALIZE_NUMBER_COLUMN(m_oFoother);
}

typedef std::queue<std::pair<std::pair<size_t, size_t>, NSStringUtils::CStringBuilder*>> NestedCells;

inline void WriteRowStart(CMDWriter& oWriter)
{
	oWriter.WriteOpenSpecialString(L"| ");
}

inline void WriteRowEnd(CMDWriter& oWriter)
{
	oWriter.WriteOpenSpecialString(L" |");
	oWriter.WriteBreakLine(false);
}

inline void WriteCellSeparator(CMDWriter& oWriter)
{
	oWriter.WriteOpenSpecialString(L" | ");
}

void ReadNestedCells(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, TCurentTablePosition& oPosition, NestedCells& arNestedCells, const Table& oCells, CMDWriter* pWriter, const TExternalTableData& oExternalTableData)
{
	const int nDepth{oReader.GetDepth()};

	while(oReader.ReadNextSiblingNode(nDepth))
	{
		const std::wstring wsName = oReader.GetName();
		oExternalTableData.GetSubClass(oReader, arSelectors);

		if (L"td" == wsName || L"th" == wsName)
		{
			if ((oPosition.m_unRowIndex != oPosition.m_unStartRowIndex || oPosition.m_unColumnIndex != oPosition.m_unStartColumnIndex) &&
			    ETableElement::FlatTable == oCells[oPosition.m_unRowIndex][oPosition.m_unColumnIndex]->GetType())
			{
				ReadNestedCells(oReader, arSelectors, oPosition, arNestedCells, oCells, pWriter, oExternalTableData);
				continue;
			}

			NSStringUtils::CStringBuilder *pCellData = new NSStringUtils::CStringBuilder(20);

			if (nullptr != pCellData)
			{
				pWriter->SetDataOutput(pCellData);
				oExternalTableData.ReadStream(oReader, arSelectors);
				if (oPosition.m_unColumnIndex != oCells[oPosition.m_unRowIndex].size() - 1)
					WriteCellSeparator(*pWriter);
				pWriter->RevertDataOutput();
			}

			arNestedCells.push(std::make_pair(std::make_pair(oPosition.m_unRowIndex, oPosition.m_unColumnIndex), pCellData));

			//READ and add to nested cells
			++oPosition.m_unColumnIndex;
		}
		else if (L"tr" == wsName)
		{
			oPosition.m_unColumnIndex = oPosition.m_unStartColumnIndex;

			ReadNestedCells(oReader, arSelectors, oPosition, arNestedCells, oCells, pWriter, oExternalTableData);

			++oPosition.m_unRowIndex;
		}
		else if (L"table" == wsName)
		{
			TCurentTablePosition oNestedPosition{oPosition};

			oNestedPosition.m_unStartRowIndex    = oPosition.m_unRowIndex;
			oNestedPosition.m_unStartColumnIndex = oPosition.m_unColumnIndex;

			std::vector<NSCSS::CNode> arNestedSelectors{arSelectors.back()};

			ReadNestedCells(oReader, arNestedSelectors, oNestedPosition, arNestedCells, oCells, pWriter, oExternalTableData);

			oPosition.m_unRowIndex = oNestedPosition.m_unRowIndex;
		}
		else
			ReadNestedCells(oReader, arSelectors, oPosition, arNestedCells, oCells, pWriter, oExternalTableData);

		arSelectors.pop_back();
	}
}

bool CMarkdownTable::Convert(XmlUtils::CXmlLiteReader& oReader, const NSCSS::CNode& oTableNode)
{
	if (Empty())
		return false;

	CMDWriter *pWriter{dynamic_cast<CMDWriter*>(m_oExternalData.m_pWriter)};

	if (nullptr == pWriter)
		return false;

	std::vector<NSCSS::CNode> arTableSelectors{oTableNode};

	pWriter->WriteBreakLine();

	if (HaveCaption())
		WriteToStringBuilder(*m_pCaption, *pWriter->GetCurrentDocument());

	//Open table
	pWriter->WriteBreakLine();
	pWriter->EnteredTable();

	if (pWriter->InCode())
	{
		if (!pWriter->InPreformatted())
			pWriter->WriteCloseSpecialString(L"`");

		pWriter->OutCode();
	}

	if (pWriter->InPreformatted())
	{
		pWriter->WriteBreakLine();
		pWriter->WriteCloseSpecialString(L"```");
		pWriter->WriteBreakLine(false);
		pWriter->OutPreformatted();
	}
	//-----

	//Convert header
	if (!ConvertMatrix(oReader, arTableSelectors, m_oHeader.GetMatrixCells(), pWriter, true))
	{
		WriteRowStart(*pWriter);

		for (size_t unColumnIndex = 0; unColumnIndex < m_oBody.GetColumnSize() - 1; ++unColumnIndex)
			WriteCellSeparator(*pWriter);

		WriteRowEnd(*pWriter);
	}
	//Convert body
	ConvertMatrix(oReader, arTableSelectors, m_oBody.GetMatrixCells(), pWriter);
	//Convert foother
	ConvertMatrix(oReader, arTableSelectors, m_oFoother.GetMatrixCells(), pWriter);

	//Close table
	pWriter->OutTable();
	pWriter->WriteBreakLine(false);

	return true;
}

bool CMarkdownTable::ParseCaption(XmlUtils::CXmlLiteReader& oReader, XmlString*& pCaption)
{
	if (nullptr == m_oExternalData.m_pWriter)
		return false;

	if (nullptr == pCaption)
		pCaption = new XmlString(50);

	std::vector<NSCSS::CNode> arSelectors;
	m_oExternalData.GetSubClass(oReader, arSelectors);

	arSelectors.back().m_pCompiledStyle->m_oText.SetAlign(L"center", 0, true);

	CMDWriter& oWriter{*(CMDWriter*)m_oExternalData.m_pWriter};

	oWriter.SetDataOutput(m_pCaption);
	m_oExternalData.ReadStream(oReader, arSelectors);
	oWriter.WriteBreakLine();
	oWriter.RevertDataOutput();

	return true;
}

bool CMarkdownTable::ParseColgroup(XmlUtils::CXmlLiteReader& oReader, std::vector<CTableColgroup*>& arColgroups)
{
	return true;
}

bool CMarkdownTable::ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, const Table& oMatrix, CMDWriter* pWriter, bool bIsHeader)
{
	if (oMatrix.empty() || nullptr == pWriter)
		return false;

	std::stack<int> arDepths({0});

	NestedCells arNestedCells;
	ITableElementCell* pTableCell{nullptr};

	for (size_t unRowIndex = 0; unRowIndex < oMatrix.size(); ++unRowIndex)
	{
		WriteRowStart(*pWriter);

		for (size_t unColumnIndex = 0; unColumnIndex < oMatrix[unRowIndex].size(); ++unColumnIndex)
		{
			pTableCell = oMatrix[unRowIndex][unColumnIndex];

			if (nullptr != pTableCell && ETableElement::FillingCell != pTableCell->GetType())
			{
				if (!arNestedCells.empty() && arNestedCells.front().first == std::make_pair(unRowIndex, unColumnIndex))
				{
					if (nullptr != arNestedCells.front().second)
					{
						m_oExternalData.m_pWriter->GetCurrentDocument()->Write(*arNestedCells.front().second);
						delete arNestedCells.front().second;
					}
					arNestedCells.pop();
				}
				else
				{
					MoveToNextTableCell(oReader, arSelectors, arDepths, m_oExternalData.GetSubClass);

					if (ETableElement::FlatTable == pTableCell->GetType())
					{
						TCurentTablePosition oPosition{unRowIndex, unColumnIndex, unRowIndex, unColumnIndex};
						std::vector<NSCSS::CNode> arNestedSelectors;

						ReadNestedCells(oReader, arNestedSelectors, oPosition, arNestedCells, oMatrix, pWriter, m_oExternalData);
						--unColumnIndex;
					}
					else
					{
						m_oExternalData.ReadStream(oReader, arSelectors);
						if (oMatrix[unRowIndex].size() - 1 != unColumnIndex)
							WriteCellSeparator(*pWriter);
					}
					arSelectors.pop_back();
				}
			}
			else
			{
				if (oMatrix[unRowIndex].size() - 1 != unColumnIndex)
					WriteCellSeparator(*pWriter);
			}
		}

		WriteRowEnd(*pWriter);

		if (bIsHeader && 0 == unRowIndex)
		{
			WriteRowStart(*pWriter);

			for (size_t unColumnIndex = 0; unColumnIndex < oMatrix[unRowIndex].size() - 1; ++unColumnIndex)
				pWriter->WriteString(L"-|-", true);

			WriteRowEnd(*pWriter);
		}
	}

	return true;
}

Table CMarkdownTable::Flatten(Table&& srcTable)
{
	if (srcTable.empty())
		return {};

	const size_t unRows{srcTable.size()};
	const size_t unColumns{srcTable[0].size()};

	std::vector<std::vector<TElementInfo>> oInfos{unRows, std::vector<TElementInfo>{unColumns}};

	for (size_t unRowIndex = 0; unRowIndex < unRows; ++unRowIndex)
		for (size_t unColumnIndex = 0; unColumnIndex < unColumns; ++unColumnIndex)
			oInfos[unRowIndex][unColumnIndex] = ComputeInfo(srcTable[unRowIndex][unColumnIndex]);

	std::vector<size_t> arRowHeights(unRows, 1);
	std::vector<size_t> arColumnWidths(unColumns, 1);

	for (size_t unRowIndex = 0; unRowIndex < unRows; ++unRowIndex)
	{
		for (size_t unColumnIndex = 0; unColumnIndex < unColumns; ++unColumnIndex)
		{
			arRowHeights[unRowIndex] = (std::max)(arRowHeights[unRowIndex], oInfos[unRowIndex][unColumnIndex].unRows);
			arColumnWidths[unColumnIndex] = (std::max)(arColumnWidths[unColumnIndex], oInfos[unRowIndex][unColumnIndex].unColumns);
		}
	}

	std::vector<size_t> arRowStart   (unRows    + 1, 0);
	std::vector<size_t> arColumnStart(unColumns + 1, 0);

	for (size_t unRowIndex = 0; unRowIndex < unRows; ++unRowIndex)
		arRowStart[unRowIndex + 1] = arRowStart[unRowIndex] + arRowHeights[unRowIndex];

	for (size_t unColumnIndex = 0; unColumnIndex < unColumns; ++unColumnIndex)
		arColumnStart[unColumnIndex + 1] = arColumnStart[unColumnIndex] + arColumnWidths[unColumnIndex];

	const size_t unTotalRows   {arRowStart[unRows]};
	const size_t unTotalColumns{arColumnStart[unColumns]};

	Table oResult{unTotalRows, Row{unTotalColumns, nullptr}};

	size_t unBaseRow, unBaseColumn;

	for (size_t unRowIndex = 0; unRowIndex < unRows; ++unRowIndex)
	{
		for (size_t unColumnIndex = 0; unColumnIndex < unColumns; ++unColumnIndex)
		{
			ITableElementCell*& pCell{srcTable[unRowIndex][unColumnIndex]};

			if (nullptr == pCell)
				continue;

			const TElementInfo oInfo{oInfos[unRowIndex][unColumnIndex]};

			unBaseRow    = arRowStart[unRowIndex];
			unBaseColumn = arColumnStart[unColumnIndex];

			if (!IsTable(pCell))
			{
				oResult[unBaseRow][unBaseColumn] = pCell;
				pCell = nullptr;
				continue;
			}

			CMarkdownTable *pTable{dynamic_cast<CMarkdownTable*>(pCell)};
			pCell = nullptr;

			Table& oChild{pTable->m_oBody.GetMatrixCells()};
			Table oFlatChild{Flatten(std::move(oChild))};

			for (size_t unChildRowIndex = 0; unChildRowIndex < oInfo.unRows; ++unChildRowIndex)
				for (size_t unChildColumnIndex = 0; unChildColumnIndex < oInfo.unColumns; ++unChildColumnIndex)
					oResult[unBaseRow + unChildRowIndex][unBaseColumn + unChildColumnIndex] = oFlatChild[unChildRowIndex][unChildColumnIndex];

			CTableElementCell* pFlatCell{dynamic_cast<CTableElementCell*>(oResult[unBaseRow][unBaseColumn])};

			if(nullptr != pFlatCell)
				pFlatCell->IsFlatTable();

			delete pTable;
		}
	}

	return oResult;
}

TElementInfo CMarkdownTable::ComputeInfo(const ITableElementCell* pCell)
{
	if (!IsTable(pCell))
		return {1, 1};

	const CMarkdownTable *pMarkdownTable{dynamic_cast<const CMarkdownTable*>(pCell)};

	if (nullptr == pMarkdownTable)
		return {1, 1};

	const CTableMatrix *pTable{&pMarkdownTable->m_oBody};

	const size_t unRows{pTable->GetRowSize()};
	const size_t unColumns{pTable->GetColumnSize()};

	std::vector<size_t> arRowHeights(unRows, 1);
	std::vector<size_t> arColumnWidths(unColumns, 1);

	const Table& arCells{pTable->GetMatrixCells()};

	for (size_t unRowIndex = 0; unRowIndex < unRows; ++unRowIndex)
	{
		for (size_t unColumnIndex = 0; unColumnIndex < unColumns; ++unColumnIndex)
		{
			TElementInfo oInfo{ComputeInfo(arCells[unRowIndex][unColumnIndex])};

			arRowHeights[unRowIndex] = (std::max)(arRowHeights[unRowIndex], oInfo.unRows);
			arColumnWidths[unColumnIndex] = (std::max)(arColumnWidths[unColumnIndex], oInfo.unColumns);
		}
	}

	size_t unTotalRows{0}, unTotalColumns{0};

	for (size_t unHeight : arRowHeights)
		unTotalRows += unHeight;

	for (size_t unWidth : arColumnWidths)
		unTotalColumns += unWidth;

	return {unTotalRows, unTotalColumns};
}
}
