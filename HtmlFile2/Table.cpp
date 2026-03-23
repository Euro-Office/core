#include "Table.h"

#include "Common.h"
#include "src/CCompiledStyle.h"

namespace HTML
{
#define MAX_STRING_BLOCK_SIZE (size_t)10485760

#define FIRST_ELEMENT 0x00000001
#define LAST_ELEMENT  0x00000002
#define MID_ELEMENT   0x00000004

#define PARSE_MODE_HEADER  0x00000100
#define PARSE_MODE_BODY    0x00000200
#define PARSE_MODE_FOOTHER 0x00000400

#define COL_POSITION_MASK 0x0000000F
#define ROW_POSITION_MASK 0x000000F0
#define PARSE_MODE_MASK   0x00000F00

#define DEFAULT_PAGE_WIDTH  12240 // Значение в Twips
#define DEFAULT_PAGE_HEIGHT 15840 // Значение в Twips

// const TTableCellStyle* CStorageTable::GetColStyle(UINT unColumnNumber) const
// {
// 	if (m_arColgroups.empty())
// 		return NULL;

// 	UINT unCurrentNumber = 0;

// 	for (const CTableColgroup* pColgroup : m_arColgroups)
// 	{
// 		for (const CTableCol* pCol : pColgroup->GetCols())
// 		{
// 			unCurrentNumber += pCol->GetSpan();

// 			if (unCurrentNumber >= unColumnNumber)
// 				return pCol->GetStyle();
// 		}
// 	}

// 	return NULL;
// }

// void CStorageTable::AddColgroup(CTableColgroup* pElement)
// {
// 	if (NULL != pElement)
// 		m_arColgroups.push_back(pElement);
// }

// void CStorageTable::RecalculateMaxColumns()
// {
// 	for (const std::vector<CStorageTableRow*>& arHeaders : m_arHeaders)
// 		for (const CStorageTableRow* pHeader : arHeaders)
// 			m_unMaxColumns = std::max(m_unMaxColumns, pHeader->GetIndex());

// 	for (const CStorageTableRow* pRow : m_arRows)
// 		m_unMaxColumns = std::max(m_unMaxColumns, pRow->GetIndex());

// 	for (const CStorageTableRow* pFoother : m_arFoother)
// 		m_unMaxColumns = std::max(m_unMaxColumns, pFoother->GetIndex());
// }

// void CStorageTable::Shorten()
// {
// 	UINT unIndex      = 0;
// 	CStorageTableCell* pCell = NULL;

// 	UINT unMaxIndex = 0; //Максимальный индекс без учета строк, где имеется только 1 ячейка

// 	for (const CStorageTableRow* pRow : m_arRows)
// 	{
// 		if (1 < pRow->GetCount())
// 			unMaxIndex = std::max(unMaxIndex, pRow->GetIndex());
// 	}

// 	while (unIndex < m_arMinColspan.size())
// 	{
// 		for (CStorageTableRow* pRow : m_arRows)
// 		{
// 			if (0 != unMaxIndex && 1 == pRow->GetCount() && pRow->GetIndex() > unMaxIndex)
// 			{
// 				pCell = (*pRow)[unIndex];

// 				if (NULL == pCell)
// 					continue;

// 				pCell->SetColspan(unMaxIndex, MAXCOLUMNSINTABLE);
// 				continue;
// 			}

// 			if (1 == m_arMinColspan[unIndex])
// 				break;

// 			pCell = (*pRow)[unIndex];

// 			if (NULL == pCell)
// 				continue;

// 			if (1 < pCell->GetColspan() && pCell->GetColspan() > m_arMinColspan[unIndex])
// 			{
// 				pCell->SetColspan(m_arMinColspan[unIndex], MAXCOLUMNSINTABLE);
// 				continue;
// 			}

// 			if ((*pRow)[unIndex]->GetColspan() == m_arMinColspan[unIndex] + 1)
// 				(*pRow)[unIndex]->SetColspan(2, MAXCOLUMNSINTABLE);
// 			else if ((*pRow)[unIndex]->GetColspan() > m_arMinColspan[unIndex])
// 				(*pRow)[unIndex]->SetColspan((*pRow)[unIndex]->GetColspan() - m_arMinColspan[unIndex], MAXCOLUMNSINTABLE);
// 		}

// 		++unIndex;
// 	}
// }


CTableElementCell::CTableElementCell(bool bIsFilling)
	: m_eType{(bIsFilling) ? ETableElement::FillingCell : ETableElement::Cell}
{}

CTableElementCell::~CTableElementCell()
{}

ETableElement CTableElementCell::GetType() const
{
	return m_eType;
}

void CTableElementCell::IsFlatTable()
{
	m_eType = ETableElement::FlatTable;
}

CTableMatrix::CTableMatrix()
{}

CTableMatrix::~CTableMatrix()
{
	for (Row& oRow : m_arCells)
	{
		for (ITableElementCell* pCell : oRow)
		{
			if (nullptr != pCell)
				delete pCell;
		}
	}
}

bool CTableMatrix::Empty() const
{
	return m_arCells.empty();
}

bool CTableMatrix::SetCell(size_t unRowIndex, size_t unColumnIndex, ITableElementCell* pCell)
{
	if (nullptr == pCell)
		return false;

	if (m_arCells.size() <= unRowIndex)
	{
		if (!m_arCells.empty())
		{
			const size_t unMaxColumn{m_arCells.back().size()};
			const size_t unOldSize{m_arCells.size()};
			m_arCells.resize(unRowIndex + 1);

			for (size_t unIndex = unOldSize; unIndex < m_arCells.size(); ++unIndex)
				m_arCells[unIndex].resize(unMaxColumn);
		}
		else
			m_arCells.resize(unRowIndex + 1);
	}

	if (m_arCells[unRowIndex].size() <= unColumnIndex)
	{
		m_arCells[unRowIndex].resize(unColumnIndex + 1);
		m_arCells[unRowIndex][unColumnIndex] = pCell;
		return true;
	}

	if (nullptr != m_arCells[unRowIndex][unColumnIndex])
		delete m_arCells[unRowIndex][unColumnIndex];

	m_arCells[unRowIndex][unColumnIndex] = pCell;

	return true;
}

bool CTableMatrix::IsFillingCell(size_t unRowIndex, size_t unColumnIndex) const
{
	if (unRowIndex >= m_arCells.size())
		return false;

	if (unColumnIndex >= m_arCells[unRowIndex].size())
		return false;

	return (nullptr != m_arCells[unRowIndex][unColumnIndex]) ? ETableElement::FillingCell == m_arCells[unRowIndex][unColumnIndex]->GetType() : false;
}

bool CTableMatrix::IsNotNullCell(size_t unRowIndex, size_t unColumnIndex) const
{
	if (unRowIndex >= m_arCells.size())
		return false;

	if (unColumnIndex >= m_arCells[unRowIndex].size())
		return false;

	return nullptr != m_arCells[unRowIndex][unColumnIndex];
}

size_t CTableMatrix::GetRowSize() const
{
	return m_arCells.size();
}

size_t CTableMatrix::GetColumnSize() const
{
	return (m_arCells.empty()) ? 0 : m_arCells.front().size();
}

Table& CTableMatrix::GetMatrixCells()
{
	return m_arCells;
}

const Table& CTableMatrix::GetMatrixCells() const
{
	return m_arCells;
}

const ITableElementCell* CTableMatrix::GetCell(size_t unRowIndex, size_t unColumnIndex) const
{
	if (unRowIndex >= m_arCells.size())
		return nullptr;

	if (unColumnIndex >= m_arCells[unRowIndex].size())
		return nullptr;

	return m_arCells[unRowIndex][unColumnIndex];
}

bool TExternalTableData::IsValid() const
{
	return true;
}

CTableElement::CTableElement(TExternalTableData *pExternalData)
	: m_pCaption(nullptr), m_pExternalData(pExternalData)
{}

CTableElement::~CTableElement()
{
	if (nullptr != m_pCaption)
		delete m_pCaption;

	for (CTableColgroup* pColgroup : m_arColgroups)
		if (nullptr != pColgroup)
			delete pColgroup;
}

ETableElement CTableElement::GetType() const
{
	return ETableElement::Table;
}

bool CTableElement::Empty() const
{
	return m_oHeader.Empty() && m_oBody.Empty() && m_oFoother.Empty();
}

bool CTableElement::HaveCaption() const
{
	return nullptr != m_pCaption && 0 != m_pCaption->GetCurSize();
}

const NSCSS::CCompiledStyle* CTableElement::GetColStyle(size_t unColIndex) const
{
	if (m_arColgroups.empty())
		return nullptr;

	const NSCSS::CCompiledStyle* pColStyle{nullptr};
	size_t unSkipSpans{0};

	for (const CTableColgroup* pColgroup : m_arColgroups)
	{
		pColStyle = pColgroup->GetColStyle(unColIndex - unSkipSpans);

		if (nullptr != pColStyle)
			return pColStyle;

		unSkipSpans += pColgroup->GetTotalSpans();
	}

	return nullptr;
}

bool MoveToNextTableCell(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, std::stack<int>& arDepths, TExternalTableData::FuncGetSubClass& GetSubClass)
{
	while (!arDepths.empty() && !oReader.ReadNextSiblingNode2(arDepths.top()))
	{
		arDepths.pop();
		arSelectors.pop_back();
	}

	if (arDepths.empty())
		return false;

	do
	{
		const std::wstring wsName = oReader.GetName();

		if (L"#text" == wsName || L"colgroup" == wsName || L"caption" == wsName)
			return MoveToNextTableCell(oReader, arSelectors, arDepths, GetSubClass);

		GetSubClass(oReader, arSelectors);

		if (L"td" == wsName || L"th" == wsName)
			return true;
		else if (L"table" == wsName)
			return false;

		arDepths.push(oReader.GetDepth());

		return MoveToNextTableCell(oReader, arSelectors, arDepths, GetSubClass);
	}while(oReader.ReadNextSiblingNode2(arDepths.top()));

	return false;
}

CTableCol::CTableCol(NSCSS::CNode& oTableColNode)
	: m_pStyle(oTableColNode.m_pCompiledStyle)
{
	oTableColNode.m_pCompiledStyle = nullptr;
	m_unSpan = NSStringFinder::ToInt(oTableColNode.GetAttributeValue(L"span"), 1);
}

CTableCol::~CTableCol()
{
	if (nullptr != m_pStyle)
		delete m_pStyle;
}

UINT CTableCol::GetSpan() const
{
	return m_unSpan;
}

const NSCSS::CCompiledStyle* CTableCol::GetStyle() const
{
	return m_pStyle;
}

CTableColgroup::CTableColgroup(const NSCSS::CNode& oTableColgroupNode)
	: m_unTotalSpans(0)
{
	m_unWidth = NSStringFinder::ToInt(oTableColgroupNode.GetAttributeValue(L"width"), 0);
}

CTableColgroup::~CTableColgroup()
{
	for (CTableCol* pCol : m_arCols)
		if (nullptr != pCol)
			delete pCol;
}

bool CTableColgroup::Empty() const
{
	return m_arCols.empty();
}

void CTableColgroup::AddCol(CTableCol* pCol)
{
	if (nullptr == pCol)
		return;

	m_arCols.push_back(pCol);
	m_unTotalSpans += pCol->GetSpan();
}

UINT CTableColgroup::GetTotalSpans() const
{
	return m_unTotalSpans;
}

const std::vector<CTableCol*>& CTableColgroup::GetCols() const
{
	return m_arCols;
}

const NSCSS::CCompiledStyle* CTableColgroup::GetColStyle(size_t unIndex) const
{
	if (m_arCols.empty())
		return nullptr;

	size_t unCurrentIndex{0};
	for (const CTableCol* pCol : m_arCols)
	{
		if (unCurrentIndex <= unIndex && (unCurrentIndex + pCol->GetSpan()) > unIndex)
			return pCol->GetStyle();

		unCurrentIndex += pCol->GetSpan();
	}

	return nullptr;
}
}
