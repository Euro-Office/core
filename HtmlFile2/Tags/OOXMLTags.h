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

#ifndef OOXMLTAGS_H
#define OOXMLTAGS_H

#include "HTMLTags.h"
#include "../Writers/OOXMLWriter.h"

namespace HTML
{
template<>
class CAnchor<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CAnchor(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CAbbr<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CAbbr(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBreak<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CBreak(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CDivision<COOXMLWriter> : public CTag<COOXMLWriter>
{
	std::stack<UINT> m_arFootnoteIDs;
public:
	CDivision(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CImage<COOXMLWriter> : public CTag<COOXMLWriter>
{
	std::vector<std::wstring> m_arrImages;
public:
	CImage(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CFont<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CFont(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CInput<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CInput(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBaseFont<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CBaseFont(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBlockquote<COOXMLWriter> : public CTag<COOXMLWriter>
{
	std::map<std::wstring, UINT>  m_mDivs;
public:
	CBlockquote(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHorizontalRule<COOXMLWriter> : public CTag<COOXMLWriter>
{
	UINT m_unShapeId;
public:
	CHorizontalRule(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CList<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CList(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CListElement<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CListElement(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CCaption<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CCaption(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CTable<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CTable(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CTableRow<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CTableRow(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CTableCell<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CTableCell(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHTML<COOXMLWriter> : public CTag<COOXMLWriter>
{
public:
	CHTML(COOXMLWriter* pInterpretator);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};
}

#endif // OOXMLTAGS_H
