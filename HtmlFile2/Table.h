/*
 * Copyright (C) Ascensio System SIA, 2009-2026
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation, together with the
 * additional terms provided in the LICENSE file.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For
 * details, see the GNU AGPL at: https://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA by email at info@onlyoffice.com
 * or by postal mail at 20A-6 Ernesta Birznieka-Upisha Street, Riga,
 * LV-1050, Latvia, European Union.
 *
 * The interactive user interfaces in modified versions of the Program
 * are required to display Appropriate Legal Notices in accordance with
 * Section 5 of the GNU AGPL version 3.
 *
 * No trademark rights are granted under this License.
 *
 * All non-code elements of the Product, including illustrations,
 * icon sets, and technical writing content, are licensed under the
 * Creative Commons Attribution-ShareAlike 4.0 International License:
 * https://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 * This license applies only to such non-code elements and does not
 * modify or replace the licensing terms applicable to the Program's
 * source code, which remains licensed under the GNU Affero General
 * Public License v3.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#ifndef TABLE_H
#define TABLE_H

#include "../DesktopEditor/common/StringBuilder.h"
#include "../Common/3dParty/html/css/src/StyleProperties.h"
#include "../Common/3dParty/html/css/src/CNode.h"

#include <vector>

namespace HTML
{
#define MAXCOLUMNSINTABLE 63
#define MAXROWSINTABLE    32767

enum class ERowParseMode
{
	Header,
	Body,
	Foother
};

enum class ERowPosition
{
	First,
	Middle,
	Last
};

struct TTableRowStyle
{
	UINT m_unMaxIndex;
	UINT m_unMaxHeight;
	bool m_bIsHeader;

	TTableRowStyle();

	bool Empty() const;
};

struct TTableCellStyle
{
	NSCSS::NSProperties::CDigit  m_oWidth;
	NSCSS::NSProperties::CDigit  m_oHeight;
	NSCSS::NSProperties::CBorder m_oBorder;
	NSCSS::NSProperties::CIndent m_oPadding;
	NSCSS::NSProperties::CColor  m_oBackground;

	std::wstring m_wsHAlign;
	std::wstring m_wsVAlign;

	TTableCellStyle();

	bool Empty();
	void Copy(const TTableCellStyle* pTableCellStyle);

	TTableCellStyle& operator+=(const TTableCellStyle* pCellStyle);
};

class CStorageTable;

class CStorageTableCell
{
public:
	CStorageTableCell();
	CStorageTableCell(UINT unColspan, UINT unRowspan, bool bIsMerged, bool bIsEmpty);
	CStorageTableCell(CStorageTableCell& oCell);

	bool Empty() const;
	bool Merged() const;

	CStorageTableCell* Copy();

	static CStorageTableCell* CreateEmpty(UINT unColspan = 1, bool m_bIsMerged = false, const TTableCellStyle* pStyle = NULL);
	static CStorageTableCell* CreateEmpty(const TTableCellStyle* pStyle);

	void SetColspan(UINT unColspan, UINT unCurrentIndex);
	UINT GetColspan() const;

	void SetRowspan(UINT unRowspan);
	UINT GetRowspan() const;

	NSStringUtils::CStringBuilder* GetData();

	const TTableCellStyle* GetStyles() const;
	TTableCellStyle* GetStyles();

	void SetWidth(const NSCSS::NSProperties::CDigit& oWidth);
	void SetHeight(const NSCSS::NSProperties::CDigit& oHeight);

	UINT GetWidth() const;
	UINT GetHeight() const;

	void SetBorder(const NSCSS::NSProperties::CBorder& oBorder);

	void ClearTopBorder();
	void ClearLeftBorder();
	void ClearBottomBorder();
	void ClearRightBorder();

	void SetPadding(const NSCSS::NSProperties::CIndent& oPadding);
	void SetHAlign(const std::wstring& wsAlign);
	void SetVAlign(const std::wstring& wsAlign);
	void SetBackground(const NSCSS::NSProperties::CColor& oColor);
private:
	UINT m_unColspan;
	UINT m_unRowSpan;

	bool m_bIsMerged;
	bool m_bIsEmpty;

	TTableCellStyle m_oStyles;
	NSStringUtils::CStringBuilder m_oData;
};

class CStorageTableRow
{
public:
	CStorageTableRow();
	~CStorageTableRow();

	void AddCell(CStorageTableCell* pCell);
	void InsertCell(CStorageTableCell *pCell, int nPosition);

	UINT GetIndex() const;
	UINT GetCount() const;

	CStorageTableCell* operator[](UINT unIndex);

	bool Empty() const;
	const TTableRowStyle& GetStyles() const;
	const std::vector<CStorageTableCell*>& GetCells() const;
private:
	TTableRowStyle m_oStyles;
	std::vector<CStorageTableCell*> m_arCells;
};

class CTableCol
{
public:
	CTableCol(UINT unSpan);
	CTableCol(const NSCSS::CNode& oTableColNode);

	UINT GetSpan() const;
	TTableCellStyle* GetStyle();
	const TTableCellStyle* GetStyle() const;
private:
	UINT m_unSpan;
	TTableCellStyle m_oStyle;
};

class CTableColgroup
{
public:
	CTableColgroup(NSCSS::CNode& oTableColgroupNode);
	~CTableColgroup();

	bool Empty() const;

	void AddCol(CTableCol* pCol);

	const std::vector<CTableCol*>& GetCols() const;
private:
	std::vector<CTableCol*> m_arCols;
	UINT                    m_unWidth;
};

//Required table styles
struct TTableStyles
{
	NSCSS::NSProperties::CIndent m_oPadding;
	NSCSS::NSProperties::CIndent m_oMargin;
	NSCSS::NSProperties::CBorder m_oBorder;
	NSCSS::NSProperties::CDigit  m_oWidth;

	int  m_nCellSpacing;

	std::wstring m_wsAlign;

	enum ETableRules
	{
		All,
		Groups,
		Cols,
		None,
		Rows
	} m_enRules;

	TTableStyles();

	bool Empty() const;
};

class CStorageTable
{
public:
	CStorageTable();
	~CStorageTable();

	CStorageTableRow* operator[](UINT unIndex);

	bool Empty() const;
	bool HaveCaption();
	bool HaveColgroups() const;
	bool HaveHeader() const;
	UINT GetRowCount() const;
	const TTableStyles& GetTableStyles() const;
	const TTableCellStyle* GetColStyle(UINT unColumnNumber) const;

	void AddRows(std::vector<CStorageTableRow*>& m_arRows, ERowParseMode eParseMode = ERowParseMode::Body);
	void AddRow(CStorageTableRow* pRow, ERowParseMode eParseMode = ERowParseMode::Body);

	NSStringUtils::CStringBuilder* GetCaptionData();

	void SetPadding(const NSCSS::NSProperties::CIndent& oPadding);
	const NSCSS::NSProperties::CIndent& GetPadding() const;

	void SetMargin(const NSCSS::NSProperties::CIndent& oMargin);
	void SetBorder(const NSCSS::NSProperties::CBorder& oBorder);

	void SetWidth(const NSCSS::NSProperties::CDigit& oWidth);
	void SetCellSpacing(int nCellSpacing);
	void SetAlign(const std::wstring& wsValue);
	void SetRules(const std::wstring& wsValue);

	void AddColgroup(CTableColgroup* pElement);

	void RecalculateMaxColumns();
	void Shorten();
	void CompleteTable();

	//TODO:: refactor to const std::vector<const T*> Get...() const;
	const std::vector<std::vector<CStorageTableRow*>>& GetHeaders()   const;
	const std::vector<CStorageTableRow*>&              GetFoothers()  const;
	const std::vector<CStorageTableRow*>&              GetRows()      const;
	const std::vector<CTableColgroup*>          GetColgroups() const;

	UINT GetMaxColumns() const;
private:
	std::vector<std::vector<CStorageTableRow*>> m_arHeaders;
	std::vector<CStorageTableRow*>              m_arFoother;
	std::vector<CStorageTableRow*>              m_arRows;

	std::vector<UINT> m_arMinColspan;

	NSStringUtils::CStringBuilder m_oCaption;

	std::vector<CTableColgroup*> m_arColgroups;

	TTableStyles m_oStyles;

	UINT m_unMaxColumns;
};
}

#endif // TABLE_H
