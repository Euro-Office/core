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

#ifndef MDTAGS_H
#define MDTAGS_H

#include "HTMLTags.h"
#include "../Writers/MDWriter.h"

namespace HTML
{
template<>
class CAnchor<CMDWriter> : public CTag<CMDWriter>
{
public:
	CAnchor(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBold<CMDWriter> : public CTag<CMDWriter>
{
public:
	CBold(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBreak<CMDWriter> : public CTag<CMDWriter>
{
public:
	CBreak(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CItalic<CMDWriter> : public CTag<CMDWriter>
{
public:
	CItalic(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CStrike<CMDWriter> : public CTag<CMDWriter>
{
public:
	CStrike(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CQuotation<CMDWriter> : public CTag<CMDWriter>
{
public:
	CQuotation(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CPreformatted<CMDWriter> : public CTag<CMDWriter>
{
public:
	CPreformatted(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHeader<CMDWriter> : public CTag<CMDWriter>
{
public:
	CHeader(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CImage<CMDWriter> : public CTag<CMDWriter>
{
public:
	CImage(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CHorizontalRule<CMDWriter> : public CTag<CMDWriter>
{
public:
	CHorizontalRule(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CBlockquote<CMDWriter> : public CTag<CMDWriter>
{
public:
	CBlockquote(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CTable<CMDWriter> : public CTag<CMDWriter>
{
public:
	CTable(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CTableRow<CMDWriter> : public CTag<CMDWriter>
{
	UINT m_unLastRowType;
public:
	CTableRow(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CTableCell<CMDWriter> : public CTag<CMDWriter>
{
	UINT m_unNeedEmptyCells;
public:
	CTableCell(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CList<CMDWriter> : public CTag<CMDWriter>
{
public:
	CList(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CListElement<CMDWriter> : public CTag<CMDWriter>
{
public:
	CListElement(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};

template<>
class CCode<CMDWriter> : public CTag<CMDWriter>
{
public:
	CCode(CMDWriter* pWriter);
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) override;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) override;
};
}

#endif // MDTAGS_H
