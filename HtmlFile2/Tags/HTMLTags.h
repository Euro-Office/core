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

#ifndef HTMLTAGS_H
#define HTMLTAGS_H

#include "../Common/3dParty/html/css/src/CNode.h"
#include <boost/any.hpp>

namespace HTML
{
class ITag
{
public:
	virtual ~ITag() = default;
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any())  = 0;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) = 0;
};

class CEmptyTag : public ITag
{
public:
	virtual ~CEmptyTag() = default;
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any())
	{
		return true;
	};
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) {};
};

template<class T>
class CTag : public ITag
{
protected:
	T* m_pWriter;
public:
	CTag(T* pWriter)
		: m_pWriter(pWriter)
	{}
	virtual ~CTag() = default;
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors, const boost::any& oExtraData = boost::any()) = 0;
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) = 0;

	bool ValidWriter() const
	{
		return nullptr != m_pWriter;
	}
};

#define CREATE_TAG(tag_name)\
template<class T>\
class tag_name : public CTag<T>\
{\
public:\
	virtual bool Open(const std::vector<NSCSS::CNode>& arSelectors)  = 0;\
	virtual void Close(const std::vector<NSCSS::CNode>& arSelectors) = 0;\
}

CREATE_TAG(CAnchor);
CREATE_TAG(CAbbr);
CREATE_TAG(CBold);
CREATE_TAG(CBidirectional);
CREATE_TAG(CBreak);
CREATE_TAG(CCenter);
CREATE_TAG(CItalic);
CREATE_TAG(CKbd);
CREATE_TAG(CStrike);
CREATE_TAG(CUnderline);
CREATE_TAG(CMark);
CREATE_TAG(CQuotation);
CREATE_TAG(CSup);
CREATE_TAG(CSpan);
CREATE_TAG(CDD);
CREATE_TAG(CPreformatted);
CREATE_TAG(CHeader);
CREATE_TAG(CDivision);
CREATE_TAG(CImage);
CREATE_TAG(CFont);
CREATE_TAG(CInput);
CREATE_TAG(CBaseFont);
CREATE_TAG(CBlockquote);
CREATE_TAG(CHorizontalRule);
CREATE_TAG(CList);
CREATE_TAG(CListElement);
CREATE_TAG(CCaption);
CREATE_TAG(CTable);
CREATE_TAG(CTableRow);
CREATE_TAG(CTableCell);
CREATE_TAG(CCode);
CREATE_TAG(CHTML);

}
#endif // HTMLTAGS_H
