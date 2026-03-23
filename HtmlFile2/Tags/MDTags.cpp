#include "MDTags.h"

#include "../DesktopEditor/xml/include/xmlutils.h"

#include "../src/StringFinder.h"
#include "../Table.h"
#include "../Common/3dParty/html/css/src/CCompiledStyle.h"

#include <boost/tuple/tuple.hpp>
#include <queue>

namespace HTML
{
CAnchor<CMDWriter>::CAnchor(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CAnchor<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->WriteString(L"[");
	return true;
}

void CAnchor<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
		return;

	m_pWriter->WriteString(L"]");

	std::wstring wsHref, wsTitle;

	arSelectors.back().GetAttributeValue(L"href", wsHref);
	arSelectors.back().GetAttributeValue(L"title", wsTitle);

	m_pWriter->WriteString(L'(' + wsHref);

	if (!wsTitle.empty())
		m_pWriter->WriteString(L" \"" + wsTitle + L'"');

	m_pWriter->WriteString(L")");
}

CBold<CMDWriter>::CBold(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CBold<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	if (m_pWriter->IsBold())
		return true;

	m_pWriter->WriteOpenSpecialString(L"**");
	m_pWriter->EnteredBold();

	return true;
}

void CBold<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter() || !m_pWriter->IsBold())
		return;

	m_pWriter->WriteCloseSpecialString(L"**");
	m_pWriter->OutBold();
}

CBreak<CMDWriter>::CBreak(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CBreak<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->WriteBreakLine();

	return true;
}

void CBreak<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{}

CItalic<CMDWriter>::CItalic(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CItalic<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	if (m_pWriter->IsItalic())
		return true;

	m_pWriter->WriteOpenSpecialString(L"*");
	m_pWriter->EnteredItalic();

	return true;
}

void CItalic<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter() || !m_pWriter->IsItalic())
		return;

	m_pWriter->WriteCloseSpecialString(L"*");
	m_pWriter->OutItalic();
}

CStrike<CMDWriter>::CStrike(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CStrike<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	if (m_pWriter->IsStrike())
		return true;

	m_pWriter->WriteOpenSpecialString(L"~~");
	m_pWriter->EnteredStrike();

	return true;
}

void CStrike<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter() || !m_pWriter->IsStrike())
		return;

	m_pWriter->WriteCloseSpecialString(L"~~");
	m_pWriter->OutStrike();
}

CQuotation<CMDWriter>::CQuotation(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CQuotation<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	return true;
}

void CQuotation<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{}

CPreformatted<CMDWriter>::CPreformatted(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CPreformatted<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->WriteOpenSpecialString(L"```");
	m_pWriter->EnteredPreformatted();

	return true;
}

void CPreformatted<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
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

CHeader<CMDWriter>::CHeader(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CHeader<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	switch(arSelectors.back().m_wsName[1])
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

void CHeader<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
		return;
}

CImage<CMDWriter>::CImage(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CImage<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	std::wstring wsAlt, wsSrc, wsTitle;

	if (!arSelectors.back().GetAttributeValue(L"src", wsSrc) &&
	    !arSelectors.back().GetAttributeValue(L"alt", wsAlt))
		return false;

	arSelectors.back().GetAttributeValue(L"title", wsTitle);

	m_pWriter->WriteString(L"![" + wsAlt + L"](" + wsSrc);

	if (!wsTitle.empty())
		m_pWriter->WriteString(L" \"" + wsTitle + L'"');

	m_pWriter->WriteString(L")");

	return true;
}

void CImage<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
		return;
}

CHorizontalRule<CMDWriter>::CHorizontalRule(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CHorizontalRule<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->WriteBreakLine(false);
	m_pWriter->WriteString(L"---");
	m_pWriter->WriteBreakLine(false);

	return true;
}

void CHorizontalRule<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{}

CBlockquote<CMDWriter>::CBlockquote(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CBlockquote<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->WriteBreakLine();
	m_pWriter->EnteredBlockquote();

	return true;
}

void CBlockquote<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
		return;

	m_pWriter->OutBlockquote();
	m_pWriter->WriteBreakLine();
	m_pWriter->WriteBreakLine(false);
}

CList<CMDWriter>::CList(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CList<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->WriteBreakLine();
	m_pWriter->EnteredList(L"ol" == arSelectors.back().m_wsName);

	if (!m_pWriter->InOrederedList())
		return true;

	std::wstring wsIndex;

	if (arSelectors.back().GetAttributeValue(L"start", wsIndex))
		m_pWriter->SetIndexOrderedList(NSStringFinder::ToInt(wsIndex, 1));

	return true;
}

void CList<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
		return;

	m_pWriter->OutList();
}

CListElement<CMDWriter>::CListElement(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CListElement<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
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

void CListElement<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{}

CCode<CMDWriter>::CCode(CMDWriter* pWriter)
	: CTag(pWriter)
{}

bool CCode<CMDWriter>::Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData)
{
	if (!ValidWriter())
		return false;

	m_pWriter->EnteredCode();

	if (m_pWriter->InPreformatted())
	{
		if (!arSelectors.back().m_wsClass.empty() && arSelectors.back().m_wsClass.size() >= 9 &&
		    0 == arSelectors.back().m_wsClass.compare(0, 9, L"language-"))
			m_pWriter->WriteString(arSelectors.back().m_wsClass.substr(9, arSelectors.back().m_wsClass.size() - 9));
		m_pWriter->WriteBreakLine(false);
	}
	else
		m_pWriter->WriteOpenSpecialString(L"`");

	return true;
}

void CCode<CMDWriter>::Close(const std::vector<NSCSS::CNode>& arSelectors)
{
	if (!ValidWriter())
		return;

	if (!m_pWriter->InPreformatted())
		m_pWriter->WriteCloseSpecialString(L"`");

	m_pWriter->OutCode();
}

bool IsTable(const ITableElementCell* pCell)
{
	return nullptr != pCell && ETableElement::Table == pCell->GetType();
}

CMarkdownTable::CMarkdownTable(TExternalTableData* pExternalData)
	: CTableElement(pExternalData)
{}

CMarkdownTable::~CMarkdownTable()
{}

bool CMarkdownTable::PreParse(XmlUtils::CXmlLiteReader& oReader)
{
	return ParseTable(oReader, this);
}

void CMarkdownTable::Normalize()
{
	m_oHeader .GetMatrixCells() = Flatten(std::move(m_oHeader .GetMatrixCells()));
	m_oBody   .GetMatrixCells() = Flatten(std::move(m_oBody   .GetMatrixCells()));
	m_oFoother.GetMatrixCells() = Flatten(std::move(m_oFoother.GetMatrixCells()));
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
	if (nullptr == m_pExternalData || Empty())
		return false;

	CMDWriter *pWriter{dynamic_cast<CMDWriter*>(m_pExternalData->m_pWriter)};

	if (nullptr == pWriter)
		return false;

	std::vector<NSCSS::CNode> arTableSelectors{oTableNode};

	pWriter->WriteBreakLine();
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
	if (!ConvertMatrix(oReader, arTableSelectors, m_oHeader.GetMatrixCells(), pWriter))
	{
		WriteRowStart(*pWriter);

		for (size_t unColumnIndex = 0; unColumnIndex < m_oBody.GetColumnSize() - 1; ++unColumnIndex)
			WriteCellSeparator(*pWriter);

		WriteRowEnd(*pWriter);
	}

	WriteRowStart(*pWriter);
	for (size_t unColumnIndex = 0; unColumnIndex < m_oBody.GetColumnSize() - 1; ++unColumnIndex)
		pWriter->WriteString(L"-|-", true);
	WriteRowEnd(*pWriter);
	//----

	//Convert body
	ConvertMatrix(oReader, arTableSelectors, m_oBody.GetMatrixCells(), pWriter);
	//Convert foother
	ConvertMatrix(oReader, arTableSelectors, m_oFoother.GetMatrixCells(), pWriter);

	//Close table
	pWriter->OutTable();
	pWriter->WriteBreakLine();

	return true;
}

bool CMarkdownTable::ParseCaption(XmlUtils::CXmlLiteReader& oReader, XmlString*& pCaption)
{
	if (nullptr == m_pExternalData || nullptr == m_pExternalData->m_pWriter)
		return false;

	if (nullptr == pCaption)
		pCaption = new XmlString(50);

	std::vector<NSCSS::CNode> arSelectors;
	m_pExternalData->GetSubClass(oReader, arSelectors);

	arSelectors.back().m_pCompiledStyle->m_oText.SetAlign(L"center", 0, true);

	CMDWriter& oWriter{*(CMDWriter*)m_pExternalData->m_pWriter};

	oWriter.SetDataOutput(m_pCaption);
	m_pExternalData->ReadStream(oReader, arSelectors);
	oWriter.WriteBreakLine();
	oWriter.RevertDataOutput();

	return true;
}

bool CMarkdownTable::ParseColgroup(XmlUtils::CXmlLiteReader& oReader, std::vector<CTableColgroup*>& arColgroups)
{
	return true;
}

bool CMarkdownTable::ConvertMatrix(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, const Table& oMatrix, CMDWriter* pWriter)
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
						m_pExternalData->m_pWriter->GetCurrentDocument()->Write(*arNestedCells.front().second);
						delete arNestedCells.front().second;
					}
					arNestedCells.pop();
				}
				else
				{
					MoveToNextTableCell(oReader, arSelectors, arDepths, m_pExternalData->GetSubClass);

					if (ETableElement::FlatTable == pTableCell->GetType())
					{
						TCurentTablePosition oPosition{unRowIndex, unColumnIndex, unRowIndex, unColumnIndex};
						std::vector<NSCSS::CNode> arNestedSelectors;

						ReadNestedCells(oReader, arNestedSelectors, oPosition, arNestedCells, oMatrix, pWriter, *m_pExternalData);
						--unColumnIndex;
					}
					else
					{
						m_pExternalData->ReadStream(oReader, arSelectors);
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
