#ifndef TABLE_H
#define TABLE_H

#include "../DesktopEditor/xml/include/xmlutils.h"

#include "Writers/IWriter.h"
#include "src/StringFinder.h"

#include <stack>
#include <vector>

namespace HTML
{
#define MAXCOLUMNSINTABLE 63
#define MAXROWSINTABLE    32767

struct TCurentTablePosition
{
	size_t m_unRowIndex{0};
	size_t m_unColumnIndex{0};

	size_t m_unStartRowIndex{0};
	size_t m_unStartColumnIndex{0};
};


enum class ETableElement
{
	FillingCell,
	Cell,
	FlatTable,
	Table
};

class ITableElementCell
{
public:
	ITableElementCell() = default;
	virtual ~ITableElementCell() = default;

	virtual ETableElement GetType() const = 0;
};

class CTableElementCell : public ITableElementCell
{
	ETableElement m_eType;
public:
	CTableElementCell(bool bIsFilling = false);
	virtual ~CTableElementCell();

	ETableElement GetType() const override;

	void IsFlatTable();
};

class ITag;
class IWriter;

struct TExternalTableData
{
	typedef std::function<bool(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors)> FuncReadStream;
	typedef std::function<void(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors)> FuncGetSubClass;

	FuncReadStream  ReadStream;
	FuncGetSubClass GetSubClass;

	IWriter* m_pWriter{nullptr};

	bool IsValid() const;
};

typedef std::vector<ITableElementCell*> Row;
typedef std::vector<Row> Table;

class IWriter;

class CTableMatrix
{
public:
	CTableMatrix();
	virtual ~CTableMatrix();

	bool Empty() const;

	bool SetCell(size_t unRowIndex, size_t unColumnIndex, ITableElementCell* pCell);

	bool IsFillingCell(size_t unRowIndex, size_t unColumnIndex) const;
	bool IsNotNullCell(size_t unRowIndex, size_t unColumnIndex) const;

	size_t GetRowSize() const;
	size_t GetColumnSize() const;

	const Table& GetMatrixCells() const;
	Table& GetMatrixCells();
	const ITableElementCell* GetCell(size_t unRowIndex, size_t unColumnIndex) const;
protected:
	Table m_arCells;
};

class CTableCol
{
public:
	CTableCol(NSCSS::CNode& oTableColNode);
	~CTableCol();

	UINT GetSpan() const;
	const NSCSS::CCompiledStyle* GetStyle() const;
private:
	UINT m_unSpan;
	NSCSS::CCompiledStyle* m_pStyle;
};

class CTableColgroup
{
public:
	CTableColgroup(const NSCSS::CNode& oTableColgroupNode);
	~CTableColgroup();

	bool Empty() const;

	void AddCol(CTableCol* pCol);

	UINT GetTotalSpans() const;
	const std::vector<CTableCol*>& GetCols() const;
	const NSCSS::CCompiledStyle* GetColStyle(size_t unIndex) const;
private:
	std::vector<CTableCol*> m_arCols;
	UINT                    m_unWidth;
	UINT                    m_unTotalSpans;
};

class CTableElement : public ITableElementCell
{
protected:
	CTableElement(TExternalTableData *pExternalData);
public:
	virtual ~CTableElement();

	ETableElement GetType() const override;

	bool Empty() const;
	bool HaveCaption() const;

	virtual bool PreParse(XmlUtils::CXmlLiteReader& oReader) = 0;
	virtual void Normalize() = 0;
	virtual bool Convert(XmlUtils::CXmlLiteReader& oReader, const NSCSS::CNode& oTableNode) = 0;
protected:
	CTableMatrix m_oHeader;
	CTableMatrix m_oBody;
	CTableMatrix m_oFoother;

	XmlString *m_pCaption;

	std::vector<CTableColgroup*> m_arColgroups;

	TExternalTableData *m_pExternalData;

	const NSCSS::CCompiledStyle* GetColStyle(size_t unColIndex) const;

	virtual bool ParseCaption(XmlUtils::CXmlLiteReader& oReader, XmlString*& pCaption) = 0;
	virtual bool ParseColgroup(XmlUtils::CXmlLiteReader& oReader, std::vector<CTableColgroup*>& arColgroups) = 0;
	template<typename T>
	inline bool ParseTable(XmlUtils::CXmlLiteReader& oReader, T* pTable);
	template<typename T>
	inline bool ParseMatrix(XmlUtils::CXmlLiteReader& oReader, CTableMatrix* pMatrix, size_t& unRowIndex, size_t& unColumnIndex);
};

template<typename T>
inline bool CTableElement::ParseTable(XmlUtils::CXmlLiteReader& oReader, T* pTable)
{
	const int nDeath{oReader.GetDepth()};
	std::wstring wsName;

	while(oReader.ReadNextSiblingNode(nDeath))
	{
		wsName = oReader.GetName();

		size_t unRowIndex{0}, unColumnIndex{0};

		if(L"thead" == wsName)
			ParseMatrix<T>(oReader, &pTable->m_oHeader, unRowIndex, unColumnIndex);
		if(L"tbody" == wsName)
			ParseMatrix<T>(oReader, &pTable->m_oBody, unRowIndex, unColumnIndex);
		else if(L"tfoot" == wsName)
			ParseMatrix<T>(oReader, &pTable->m_oFoother, unRowIndex, unColumnIndex);
		else if (L"caption" == wsName)
			ParseCaption(oReader, pTable->m_pCaption);
		else if (L"colgroup" == wsName)
			ParseColgroup(oReader, pTable->m_arColgroups);
	}

	return true;
}

template<typename T>
inline bool CTableElement::ParseMatrix(XmlUtils::CXmlLiteReader& oReader, CTableMatrix* pMatrix, size_t& unRowIndex, size_t& unColumnIndex)
{
	const std::wstring wsElementName{oReader.GetName()};

	if (L"table" == wsElementName)
	{
		T* pNewTable{new T(m_pExternalData)};

		if (nullptr == pNewTable)
			return false;

		pMatrix->SetCell(unRowIndex - 1, unColumnIndex - 1, pNewTable);

		ParseTable<T>(oReader, pNewTable);
	}
	else if (L"tr" == wsElementName)
	{
		const int nDepth{oReader.GetDepth()};
		++unRowIndex;
		unColumnIndex = 0;

		while (oReader.ReadNextSiblingNode(nDepth))
		{
			if (L"td" != oReader.GetName() && L"th" != oReader.GetName())
				continue;

			ParseMatrix<T>(oReader, pMatrix, unRowIndex, unColumnIndex);
		}
	}
	else if (L"td" == wsElementName || L"th" == wsElementName)
	{
		++unColumnIndex;

		while (pMatrix->IsFillingCell(unRowIndex - 1, unColumnIndex - 1))
			++unColumnIndex;

		if (!pMatrix->SetCell(unRowIndex - 1, unColumnIndex - 1, new CTableElementCell()))
			return false;

		if (oReader.MoveToFirstAttribute())
		{
			do
			{
				if (L"rowspan" == oReader.GetName())
				{
					const int nRowSpan{NSStringFinder::ToInt(oReader.GetText(), 1)};

					for (int nRow = 1; nRow < nRowSpan; ++nRow)
						pMatrix->SetCell(unRowIndex + nRow - 1, unColumnIndex - 1, new CTableElementCell(true));
				}
				else if (L"colspan" == oReader.GetName())
					unColumnIndex += NSStringFinder::ToInt(oReader.GetText(), 1) - 1;
			}while (oReader.MoveToNextAttribute());
			oReader.MoveToElement();
		}

		const int nDepth{oReader.GetDepth()};
		while (oReader.ReadNextSiblingNode(nDepth))
			ParseMatrix<T>(oReader, pMatrix, unRowIndex, unColumnIndex);
	}
	else if (oReader.IsEmptyNode())
		return false;
	else
	{
		const int nDepth{oReader.GetDepth()};
		while (oReader.ReadNextSiblingNode(nDepth))
			ParseMatrix<T>(oReader, pMatrix, unRowIndex, unColumnIndex);
	}

	return true;
}

bool MoveToNextTableCell(XmlUtils::CXmlLiteReader& oReader, std::vector<NSCSS::CNode>& arSelectors, std::stack<int>& arDepths, TExternalTableData::FuncGetSubClass& GetSubClass);
}

#endif // TABLE_H
