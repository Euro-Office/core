/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "PtgExtraArray.h"
#include "SyntaxPtg.h"
#include "SerBool.h"
#include "SerErr.h"
#include "SerNil.h"
#include "SerNum.h"
#include "SerStr.h"

namespace XLS
{


PtgExtraArray::PtgExtraArray()
{
}


PtgExtraArray::PtgExtraArray(const std::wstring& word)
{
	fromString(word);
}


BiffStructurePtr PtgExtraArray::clone()
{
	return BiffStructurePtr(new PtgExtraArray(*this));
}

void PtgExtraArray::load(CFRecord& record)
{
    auto tempcols = 0;
    auto temprows = 0;
    if (record.getGlobalWorkbookInfo()->Version < 0x0800)
    {
        DColunByteU cols_xls;
        DRw rows_xls;
        record >> cols_xls >> rows_xls;
        cols = cols_xls;
        rows = rows_xls;
        tempcols = cols_xls+1;
        temprows = rows_xls+1;
    }
    else
    {
        record >> cols >> rows;
        tempcols = cols;
        temprows = rows;
		--cols;
		--rows;
    }
    for(int i = 0; i < (tempcols) * (temprows); ++i)
	{
		if (record.getRdPtr() >= record.getDataSize())
		{
			unsigned char rec_type = SerAr::SerType::typeSerNil;
			SerArPtr ser(SerAr::createSerAr(rec_type));
			array_.push_back(ser);
			continue;
		}
		unsigned char rec_type;
		record >> rec_type;
        if (record.getGlobalWorkbookInfo()->Version >= 0x0800)
        {
            switch(rec_type)
            {
                case 0:
                    rec_type = SerAr::SerType::typeSerNum;
                    break;
                case 1:
                    rec_type = SerAr::SerType::typeSerStr;
                    break;
                case 2:
                    rec_type = SerAr::SerType::typeSerBool;
                case 4:
                    rec_type = SerAr::SerType::typeSerErr;
                    break;
                default:
                    rec_type = SerAr::SerType::typeSerNil;
                    break;
            }
        }
		SerArPtr ser(SerAr::createSerAr(rec_type));
        if(ser.get())
        {
            record >> *ser;
            array_.push_back(ser);
        }
	}
}

void PtgExtraArray::save(CFRecord& record)
{
	if (record.getGlobalWorkbookInfo()->Version < 0x0800)
	{
		DColunByteU cols_xls;
		DRw rows_xls;
		cols_xls = cols;
		rows_xls = rows;
		record << cols_xls << rows_xls;
	}
	else
	{
		cols++;
		rows++;
        record << rows << cols;
	}
	for (auto& item : array_)
	{
		record << *item;
	}
}

const std::wstring PtgExtraArray::toString() const
{
	std::wstring ret_val;
	unsigned char col_cnt = cols;

	if (array_.empty()) return L"";

	for(std::vector<SerArPtr>::const_iterator it = array_.begin(), itEnd = --array_.end(); it != itEnd; ++it)
	{
		auto tempVal = (*it)->toString();
		if(tempVal.empty())
			tempVal = L"#N/A";
		ret_val += tempVal;
		if (col_cnt)
		{
			ret_val += L',';
			col_cnt--;
		}
		else
		{
			ret_val += L';';
			col_cnt = cols;
		}
	}
	auto tempVal = array_.back()->toString();
	if(tempVal.empty())
			tempVal = L"#N/A";
	ret_val += tempVal;
	return ret_val;
}


void PtgExtraArray::fromString(const std::wstring& str)
{
	if(str.length() <= 2)
	{
		// EXCEPT::LE::WhatIsTheFuck("Unknown PtgExtraArray format extracted from formula.", __FUNCTION__);

	}
	std::wstring::const_iterator first = str.begin() + 1;
	std::wstring::const_iterator last = str.end() - 1;

	cols = 0;
    rows = 0;
	size_t elementStart = 0; // index in array_ of the first value of the current element
	// Every element must end up with exactly one constant, so that the values match the row and column count.
	// An element with no value or more than one (e.g. "{1,,2}" or "{1 2}") is stored as #NAME?.
	auto closeElement = [&]()
	{
		if(array_.size() != elementStart + 1)
		{
			array_.resize(elementStart);
			array_.push_back(SerArPtr(new SerErr(L"#NAME?")));
		}
		elementStart = array_.size();
	};
	auto isBlank = [](wchar_t c) { return c == L' ' || c == L'\t' || c == L'\r' || c == L'\n'; };
	while(first != last)
	{
		std::wstring operand_str;
		if(SyntaxPtg::extract_PtgNum(first, last, operand_str))
		{
			array_.push_back(SerArPtr(new SerNum(operand_str)));
		}
		else if(SyntaxPtg::extract_PtgBool(first, last, operand_str))
		{
			array_.push_back(SerArPtr(new SerBool(operand_str)));
		}
		else if(SyntaxPtg::extract_PtgErr(first, last, operand_str))
		{
			array_.push_back(SerArPtr(new SerErr(operand_str)));
		}
		else if(SyntaxPtg::extract_PtgStr(first, last, operand_str))
		{
			array_.push_back(SerArPtr(new SerStr(operand_str)));
		}
		else if(SyntaxPtg::extract_comma(first, last))
		{
			closeElement();
			++cols;
		}
		else if(SyntaxPtg::extract_semicolon(first, last)) // extract semicolon
		{
			closeElement();
			++rows;
			cols = 0;
		}
		else if(isBlank(*first))
		{
			while(first != last && isBlank(*first))
				++first;
		}
		else
		{
			// Not a number, boolean, error or string (e.g. a name): an array may hold only constants. Skip the
			// token and store #NAME? for the element, so that the loop always advances.
			while(first != last && *first != L',' && *first != L';')
				++first;
			array_.resize(elementStart);
			array_.push_back(SerArPtr(new SerErr(L"#NAME?")));
		}

	}
	closeElement();

}


} // namespace XLS

