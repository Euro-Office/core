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
#include <cwchar>
#include <set>

#include <xml/simple_xml_writer.h>
#include "../../Common/logging.h"

#include "xlsx_table_state.h"
#include "xlsx_utils.h"
#include "xlsxconversioncontext.h"

#include "../Format/odf_document.h"
#include "../Format/odfcontext.h"
#include "../Format/style_table_properties.h"
#include "../../DataTypes/stylefamily.h"
#include "../../DataTypes/tabledatatype.h"
#include "../../DataTypes/tableoperator.h"

#include "../../Formulas/formulasconvert.h"

#include "../../../OfficeCryptReader/source/CryptTransform.h"

namespace cpdoccore {
namespace oox {

void xlsx_data_range::serialize_autofilter(std::wostream& _Wostream)
{
	if (!filter_button && filter_conditions.empty()) return;

	CP_XML_WRITER(_Wostream)
	{
		CP_XML_NODE(L"autoFilter")
		{
			CP_XML_ATTR(L"ref", ref);

			serialize_filterColumns(CP_XML_STREAM(), 0);
		}
	}
}
void xlsx_data_range::serialize_sort (std::wostream & _Wostream)
{
	if (bySort.empty()) return;

	CP_XML_WRITER(_Wostream)
	{			
		CP_XML_NODE(L"sortState")
		{
			CP_XML_ATTR(L"ref", ref);
			CP_XML_ATTR(L"xmlns:xlrd2", "http://schemas.microsoft.com/office/spreadsheetml/2017/richdata2");

			if (!byRow)
				CP_XML_ATTR(L"columnSort", true);

			std::wstring ref_base, ref2, ref1;
			size_t col_base, row_base, col_2, row_2;

			size_t pos = ref.find(L":");
			if (pos != std::wstring::npos)
			{
				ref_base = ref.substr(0, pos);
				ref2 = ref.substr(pos + 1);
			}
			bool bSplit = getCellAddressInv(ref_base, col_base, row_base) && getCellAddressInv(ref2, col_2, row_2);
			
			for (size_t i = 0 ; bSplit && i < bySort.size(); i++)
			{
				bool in_range = true;
				if (byRow)
				{
					if (col_base + bySort[i].first > col_2)	in_range = false;

					ref1 = getCellAddress(col_base + bySort[i].first + (withHeader ? 1 : 0), row_base);
					ref2 = getCellAddress(col_base + bySort[i].first + (withHeader ? 1 : 0), row_2);
				}
				else
				{
					if ( bySort[i].first > row_2)	in_range = false;

					ref1 = getCellAddress(col_base, row_base + bySort[i].first + (withHeader ? 1 : 0));
					ref2 = getCellAddress(col_2, row_base + bySort[i].first + (withHeader ? 1 : 0));
				}
				if (in_range)
				{
					CP_XML_NODE(L"sortCondition")
					{
						CP_XML_ATTR(L"ref", ref1 + L":" + ref2);

						if (bySort[i].second)
							CP_XML_ATTR(L"descending", 1);
					}

				}
			}
		}	
	}
}
namespace
{
	// ODF table:operator -> ST_FilterOperator of customFilter, with the wildcards that turn
	// begins/ends/contains into a pattern (the same mapping LibreOffice uses for XLSX).
	bool custom_filter(int operator_, const std::wstring& value, std::wstring& oox_operator, std::wstring& oox_value)
	{
		oox_value = value;
		switch (operator_)
		{
		case odf_types::table_operator::Equal:				oox_operator = L"equal"; break;
		case odf_types::table_operator::NotEqual:			oox_operator = L"notEqual"; break;
		case odf_types::table_operator::LessThan:			oox_operator = L"lessThan"; break;
		case odf_types::table_operator::GreaterThan:		oox_operator = L"greaterThan"; break;
		case odf_types::table_operator::LessThanOrEqual:	oox_operator = L"lessThanOrEqual"; break;
		case odf_types::table_operator::GreaterThanOrEqual:	oox_operator = L"greaterThanOrEqual"; break;
		case odf_types::table_operator::BeginsWith:			oox_operator = L"equal"; oox_value = value + L"*"; break;
		case odf_types::table_operator::EndsWith:			oox_operator = L"equal"; oox_value = L"*" + value; break;
		case odf_types::table_operator::Contains:			oox_operator = L"equal"; oox_value = L"*" + value + L"*"; break;
		case odf_types::table_operator::NotBeginsWith:		oox_operator = L"notEqual"; oox_value = value + L"*"; break;
		case odf_types::table_operator::NotEndsWith:		oox_operator = L"notEqual"; oox_value = L"*" + value; break;
		case odf_types::table_operator::NotContains:		oox_operator = L"notEqual"; oox_value = L"*" + value + L"*"; break;
		case odf_types::table_operator::Empty:				oox_operator = L"equal"; oox_value = L""; break;
		case odf_types::table_operator::NonEmpty:			oox_operator = L"notEqual"; oox_value = L" "; break;
		default:
			return false;
		}
		return true;
	}
	bool is_top10(int operator_)
	{
		return operator_ == odf_types::table_operator::TopValues || operator_ == odf_types::table_operator::BottomValues ||
			operator_ == odf_types::table_operator::TopPercent || operator_ == odf_types::table_operator::BottomPercent;
	}
	// The N of top/bottom N: a number, limited to 0..500 like LibreOffice's XLSX export.
	bool top10_val(const std::wstring& value, std::wstring& result)
	{
		if (value.empty()) return false;
		wchar_t* end = NULL;
		const double val = wcstod(value.c_str(), &end);
		if (end == value.c_str() || *end != L'\0' || !(val == val)) return false;
		if (val < 0)
			result = L"0";
		else if (val > 500)
			result = L"500";
		else
			result = value;
		return true;
	}
	bool is_color(int type)
	{
		return type == odf_types::table_data_type::background_color || type == odf_types::table_data_type::text_color;
	}
}

void xlsx_data_range::serialize_filterColumns(std::wostream& _Wostream, int colOffset)
{
	// table:orientation="column" is not checked: LibreOffice writes it from the sort settings of the range,
	// and its filters still hide rows by the values of the field's column.
	// ODF field numbers count from the first column of the database range; OOXML wants one
	// filterColumn per column, its colId counted from the first column of the autoFilter range.
	std::map<int, std::vector<const filter_condition*>> fields;
	std::map<int, std::set<int>> or_group_fields;
	const int width = cell_end.first - cell_start.first;
	for (size_t i = 0; i < filter_conditions.size(); ++i)
	{
		const int field = filter_conditions[i].field_number;
		if (filter_conditions[i].or_group >= 0)
			or_group_fields[filter_conditions[i].or_group].insert(field);
		if (field < 0 || field > width) continue; // not a column of the range
		fields[field].push_back(&filter_conditions[i]);
	}
	// The filterColumns of an autoFilter are ANDed: an OR of conditions on more than one field
	// has no OOXML form, and LibreOffice does not export such a filter to XLSX either.
	for (std::map<int, std::set<int>>::iterator it = or_group_fields.begin(); it != or_group_fields.end(); ++it)
	{
		if (it->second.size() > 1) return;
	}

	CP_XML_WRITER(_Wostream)
	{
		for (std::map<int, std::vector<const filter_condition*>>::iterator it = fields.begin(); it != fields.end(); ++it)
		{
			const std::vector<const filter_condition*>& conds = it->second;
			const filter_condition& first = *conds[0];

			// The conditions of a field are ANDed (no filter-or around them) or alternatives of one
			// filter-or. A field that mixes both gets no filterColumn: the other fields are ANDed
			// with it, so leaving it out shows more rows, never fewer.
			const bool bAnd = first.or_group < 0;
			bool bOneGroup = true;
			for (size_t i = 0; i < conds.size(); ++i)
			{
				if (conds[i]->or_group != first.or_group || conds[i]->and_in_or)
					bOneGroup = false;
			}
			if (!bOneGroup) continue;

			// Value list: every condition is "=" (or "empty") and they are alternatives.
			bool bValues = conds.size() == 1 || !bAnd;
			for (size_t i = 0; bValues && i < conds.size(); ++i)
			{
				bValues = (conds[i]->operator_ == odf_types::table_operator::Equal ||
						   conds[i]->operator_ == odf_types::table_operator::Empty) && !is_color(conds[i]->type);
			}
			std::vector<std::pair<std::wstring, std::wstring>> customs;
			for (size_t i = 0; !bValues && i < conds.size(); ++i)
			{
				std::wstring oox_operator, oox_value;
				if (!is_color(conds[i]->type) && custom_filter(conds[i]->operator_, conds[i]->value, oox_operator, oox_value))
					customs.push_back(std::make_pair(oox_operator, oox_value));
			}
			// Alternatives that XLSX cannot all express (a regular expression, a colour, top N): writing only the
			// others would hide the rows that match the left-out ones, so the field gets no filterColumn. ANDed
			// conditions keep the ones XLSX can express, which shows more rows, never fewer.
			if (!bAnd && conds.size() > 1 && !bValues && customs.size() != conds.size()) continue;
			// customFilters holds two conditions; dropping one would change which rows the filter shows.
			if (customs.size() > 2) customs.clear();

			const bool bColor = is_color(first.type) && !first.value.empty();
			std::wstring top10_value;
			const bool bTop10 = conds.size() == 1 && is_top10(first.operator_) && top10_val(first.value, top10_value);

			if (!bColor && !bTop10 && !bValues && customs.empty()) continue;

			CP_XML_NODE(L"filterColumn")
			{
				CP_XML_ATTR(L"colId", it->first + colOffset);
				if (bColor)
				{
					CP_XML_NODE(L"colorFilter")
					{
						CP_XML_ATTR(L"dxfId", first.value);
						CP_XML_ATTR(L"cellColor", first.type == odf_types::table_data_type::background_color ? 1 : 0);
					}
				}
				else if (bTop10)
				{
					CP_XML_NODE(L"top10")
					{
						if (first.operator_ == odf_types::table_operator::BottomValues ||
							first.operator_ == odf_types::table_operator::BottomPercent)
							CP_XML_ATTR(L"top", 0);
						if (first.operator_ == odf_types::table_operator::TopPercent ||
							first.operator_ == odf_types::table_operator::BottomPercent)
							CP_XML_ATTR(L"percent", 1);
						CP_XML_ATTR(L"val", top10_value);
					}
				}
				else if (bValues)
				{
					// The values of a value list are the filter-set-item elements; without them the condition value.
					std::vector<std::wstring> values;
					bool bBlank = false;
					for (size_t i = 0; i < conds.size(); ++i)
					{
						if (conds[i]->operator_ == odf_types::table_operator::Empty)
						{
							bBlank = true;
							continue;
						}
						const std::vector<std::wstring>& items = conds[i]->items;
						if (items.empty())
							values.push_back(conds[i]->value);
						else
							values.insert(values.end(), items.begin(), items.end());
					}
					CP_XML_NODE(L"filters")
					{
						for (size_t i = 0; i < values.size(); ++i)
						{
							if (values[i].empty()) bBlank = true;
						}
						if (bBlank)
							CP_XML_ATTR(L"blank", 1);
						for (size_t i = 0; i < values.size(); ++i)
						{
							if (values[i].empty()) continue;
							CP_XML_NODE(L"filter")
							{
								CP_XML_ATTR_ENCODE_STRING(L"val", values[i]);
							}
						}
					}
				}
				else
				{
					CP_XML_NODE(L"customFilters")
					{
						if (bAnd && customs.size() > 1)
							CP_XML_ATTR(L"and", 1);
						for (size_t i = 0; i < customs.size(); ++i)
						{
							CP_XML_NODE(L"customFilter")
							{
								CP_XML_ATTR(L"operator", customs[i].first);
								CP_XML_ATTR_ENCODE_STRING(L"val", customs[i].second);
							}
						}
					}
				}
			}
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------

xlsx_table_state::xlsx_table_state(xlsx_conversion_context * Context, std::wstring styleName, std::wstring tableName, int tableId)
  : context_				(Context),
    table_style_			(styleName),
    tableName_				(tableName),
	tableId_				(tableId),
    current_table_column_	(-1),
    current_table_row_		(-1),
    columns_spanned_num_	(0),
    columns_count_			(0),
    xlsx_drawing_context_	(Context->get_drawing_context_handle()),
    xlsx_comments_context_	(Context->get_comments_context_handle()),
    table_column_last_width_(0.0),
	in_cell(false),
	bEndTable(false),
	bRTL(false),
	bHidden(false)
{        
	odf_reader::style_table_properties	* table_prop = NULL;
	odf_reader::style_instance			* tableStyle = context_->root()->odf_context().styleContainer().style_by_name(table_style_, odf_types::style_family::Table, false);
	
	if ((tableStyle) && (tableStyle->content()))
		table_prop = tableStyle->content()->get_style_table_properties();

	if (table_prop)
	{
		if (table_prop->content().common_writing_mode_attlist_.style_writing_mode_)
		{
			if (table_prop->content().common_writing_mode_attlist_.style_writing_mode_->get_type() == odf_types::writing_mode::RlTb)
				bRTL = true;
		}
		if ((table_prop->content().table_display_) && (false == table_prop->content().table_display_))
		{
			bHidden = true;
		}
	}
}
    
void xlsx_table_state::start_column(unsigned int repeated, const std::wstring & defaultCellStyleName, bool bHeader)
{
    for (unsigned int i = 0; i < repeated; ++i)
        column_default_cell_style_name_.push_back(defaultCellStyleName);
	
	if (bHeader)
	{
		if ((false == columnsHeaders_.empty()) && (columnsHeaders_.back().first + columnsHeaders_.back().second == columns_count_))
		{
			columnsHeaders_.back().second += repeated;
		}
		else
		{
			columnsHeaders_.push_back(std::make_pair(columns_count_, repeated));
		}
	}

    columns_count_ += repeated;
    columns_.push_back(repeated);
}

void xlsx_table_state::set_rtl(bool val)
{
	bRTL = val;
}

void xlsx_table_state::set_protection(bool val, const std::wstring &key, const std::wstring &algorithm)
{
	protection.bEnabled = val;
	protection.protect_key = key;
	
	size_t pos = algorithm.find(L"#");
	if (pos != std::wstring::npos)
	{
		protection.protect_key_algorithm = algorithm.substr(pos + 1);
	}

	//test
	//CRYPT::odfWriteProtect protect;
	//protect.SetProtectKey(DecodeBase64(protect_key));
	//protect.SetPassword(L"123");

	//bool res = protect.Verify();

}
void xlsx_table_state::set_protection_select_protected_cells(bool val)
{
	protection.selectLockedCells = !val;
}
void xlsx_table_state::set_protection_select_unprotected_cells(bool val)
{
	protection.selectUnockedCells = !val;
}
void xlsx_table_state::set_protection_insert_columns(bool val)
{
	protection.insertColumns = !val;
}
void xlsx_table_state::set_protection_insert_rows(bool val)
{
	protection.insertRows = !val;
}
void xlsx_table_state::set_protection_delete_columns(bool val)
{
	protection.deleteColumns = !val;
}
void xlsx_table_state::set_protection_delete_rows(bool val)
{
	protection.deleteRows = !val;
}
unsigned int xlsx_table_state::columns_count() const
{
    return columns_count_;
}

void xlsx_table_state::start_table_row_group(bool collapsed)
{
	group_rows_.push_back(collapsed);
}
void xlsx_table_state::end_table_row_group()
{
	group_rows_.pop_back();
}
void xlsx_table_state::start_table_column_group(bool collapsed)
{
	group_columns_.push_back(collapsed);
}
void xlsx_table_state::end_table_column_group()
{
	group_columns_.pop_back();
}
void xlsx_table_state::add_empty_row(int count, bool bHeader)
{
	if (bHeader)
	{
		if ((false == rowsHeaders_.empty()) && (rowsHeaders_.back().first + rowsHeaders_.back().second == current_table_row_))
		{
			rowsHeaders_.back().second += count;
		}
		else
		{
			rowsHeaders_.push_back(std::make_pair(current_table_row_ + 1, count));
		}
	}
	current_table_row_ += count;
}
void xlsx_table_state::start_row(const std::wstring & StyleName, const std::wstring & defaultCellStyleName, bool bHeader)
{
    empty_row_ = true;
    // reset column num, column spanned style

    current_table_row_++;
	
	if (bHeader)
	{
		if ((false == rowsHeaders_.empty()) && (rowsHeaders_.back().first + rowsHeaders_.back().second == current_table_row_))
		{
			rowsHeaders_.back().second += 1;
		}
		else
		{
			rowsHeaders_.push_back(std::make_pair(current_table_row_, 1));
		}
	}
    
	current_table_column_ = -1; 
    columns_spanned_style_ = L"";
    row_default_cell_style_name_ = defaultCellStyleName;
        
    // set row style name
    table_row_style_ = StyleName;
}
void xlsx_table_state::set_column_break_before()
{
	column_breaks_.push_back(columns_count_ - columns_.back());
}
void xlsx_table_state::set_column_break_after()
{
	column_breaks_.push_back(columns_count_ + 1 - columns_.back());
}
void xlsx_table_state::set_row_break_before()
{
	row_breaks_.push_back(current_table_row_);
}
void xlsx_table_state::set_row_break_after()
{
	row_breaks_.push_back(current_table_row_ + 1);
}
void xlsx_table_state::non_empty_row()
{
    empty_row_= false;
	last_used_row_ = current_table_row_;
	last_used_column_ = (std::max)(last_used_column_, current_table_column_);
}

bool xlsx_table_state::is_empty_row() const
{
    return empty_row_;
}

std::wstring xlsx_table_state::default_row_cell_style() const
{
    return row_default_cell_style_name_;
}

std::wstring xlsx_table_state::default_column_cell_style() const
{
	if (current_table_column_ + 1 < (int)column_default_cell_style_name_.size())
		return column_default_cell_style_name_.at(current_table_column_ + 1);
	else
	{
		//unclear data!! - incorrectly formed ods???
		return L"";
	}
}

std::wstring xlsx_table_state::default_column_cell_style(unsigned int column) const
{
	return column < column_default_cell_style_name_.size() ? column_default_cell_style_name_[column] : L"";
}

bool xlsx_table_state::same_default_column_cell_style(unsigned int first, unsigned int end) const
{
	const size_t count = column_default_cell_style_name_.size();
	if (first >= count)
		return true;

	// computed once per table (the columns come before the rows), so that a check per row is cheap
	if (column_default_cell_style_end_.size() != count)
	{
		column_default_cell_style_end_.assign(count, count);
		for (size_t i = count - 1; i > 0; i--)
		{
			column_default_cell_style_end_[i - 1] = column_default_cell_style_name_[i - 1] == column_default_cell_style_name_[i]
				? column_default_cell_style_end_[i] : i;
		}
	}
	return (end < count ? end : count) <= column_default_cell_style_end_[first];
}

bool xlsx_table_state::padded_default_column_cell_style(unsigned int first, unsigned int end,
	const std::function<bool(const std::wstring&)> & padded) const
{
	const size_t count = column_default_cell_style_name_.size();

	// computed once per table, so that a check per cell is cheap
	if (padded_default_column_next_.size() != count + 1)
	{
		padded_default_column_next_.assign(count + 1, count);
		for (size_t i = count; i > 0; i--)
		{
			padded_default_column_next_[i - 1] = padded(column_default_cell_style_name_[i - 1])
				? i - 1 : padded_default_column_next_[i];
		}
	}
	if (first < count && padded_default_column_next_[first] < end)
		return true;
	// columns past the declared ones have no default cell style of their own
	return end > count && first < end && padded(std::wstring());
}

void xlsx_table_state::end_row()
{
	in_cell = false;
	table_row_style_ = L"";
}

std::wstring xlsx_table_state::current_row_style() const
{
    return table_row_style_;
}

void xlsx_table_state::start_cell(size_t columnsSpanned, size_t rowsSpanned)
{
    current_table_column_++;

    // if a merger takes place, add a record about it
    if (columnsSpanned != 0 || rowsSpanned != 0)
	{
		xlsx_merge_cells_.add_merge(current_table_column_, current_table_row_, columnsSpanned, rowsSpanned);
	}

    //if ( current_columns_spaned() > 0 )
    //{
    //    _CP_LOG << L"[warning] current columns spanned > 0\n";
    //}

    columns_spanned_num_ = static_cast<int>(columnsSpanned);

    // update the vector that stores row-merging information
    // add a new column to it

    
    for (size_t i = 0; i <= columns_spanned_num_; ++i)
    {
        if (current_table_column_+i >= (int)(rows_spanned_.size()))
            rows_spanned_.push_back(xlsx_row_spanned());   
    }

    if ((int)rows_spanned_.size() <= current_table_column_)
    {
        _CP_LOG << L"[warning] set_rows_spanned error\n";
    }
    else
    {
        //if (rows_spanned_[current_table_column_].num() > 0)
        //{
        //    _CP_LOG << L"[warning] current rows spanned > 0\n";
        //}
        rows_spanned_[current_table_column_].num(static_cast<unsigned int>(rowsSpanned));
        rows_spanned_[current_table_column_].column_spanned(static_cast<unsigned int>(columnsSpanned));
        for (size_t i = 0; i <= columns_spanned_num_; ++i)
        {
            rows_spanned_[current_table_column_ + i].set_style_id(-1);
        }
    }

	in_cell = true;
}

void xlsx_table_state::end_cell()
{
	in_cell = false;
}

void xlsx_table_state::set_current_cell_style_id(unsigned int xfId)
{
    for (size_t i = 0; i <= columns_spanned_num_; ++i)
    {
        rows_spanned_[current_table_column_ + i].set_style_id((int)xfId);
    }
}

int xlsx_table_state::get_current_cell_style_id()
{
    return rows_spanned_[current_table_column_].get_style_id();
}

void xlsx_table_state::start_covered_cell()
{
    current_table_column_++;

    // update the vector that stores row-merging information
    // add a new column to it

    if (current_table_column_ >= (int)(rows_spanned_.size()))
        rows_spanned_.push_back(xlsx_row_spanned());

    // used the current cell, decrease the counters of the remaining merged cells
    // for columns and rows

    if (columns_spanned_num_ > 0)
        columns_spanned_num_--;

    if (rows_spanned_[current_table_column_].num() > 0)
        rows_spanned_[current_table_column_].decrease();

    if (current_table_column_ > 0)
    {
        const unsigned int pred = rows_spanned_[current_table_column_ - 1].column_spanned();
        if (pred > 0)
        {
            rows_spanned_[current_table_column_].column_spanned(pred - 1);
            rows_spanned_[current_table_column_].num(rows_spanned_[current_table_column_ - 1].num());
        }
    }
}

void xlsx_table_state::end_covered_cell()
{
}

int xlsx_table_state::current_column() const
{
    return current_table_column_;
}

int xlsx_table_state::current_row() const
{
    return current_table_row_;
}

unsigned int xlsx_table_state::current_columns_spaned() const
{
    return columns_spanned_num_;
}

unsigned int xlsx_table_state::current_rows_spanned(unsigned int Column) const
{
    if (rows_spanned_.size() <= Column)
    {
        _CP_LOG << L"[warning] current_rows_spanned error\n";
        return 0;
    }
    else
    {
        return rows_spanned_[Column].num();
    }
}

double charsToSize(unsigned int charsCount, double maxDigitSize) 
{
	return 1.0 * int((maxDigitSize * charsCount + 5.0) / maxDigitSize * 256.0) / 256.0;
}

namespace
{
	// The page layout of the sheet's master page, or NULL.
	odf_reader::page_layout_instance * sheet_page_layout(odf_reader::odf_read_context & odfContext,
														const std::wstring & table_style)
	{
		_CP_OPT(std::wstring) masterPageName = odfContext.styleContainer().master_page_name_by_name(table_style);
		if (!masterPageName) return NULL;

		odf_reader::style_master_page* master_style_ = odfContext.pageLayoutContainer().master_page_by_name(*masterPageName);
		if (!master_style_) return NULL;
		if (!master_style_->attlist_.style_page_layout_name_) return NULL;

		return odfContext.pageLayoutContainer().page_layout_by_name(*master_style_->attlist_.style_page_layout_name_);
	}

	// style:display="false" on a master page's header (footer) switches it off: LibreOffice then prints
	// neither it nor its left/first variants and reserves no space for it on the page.
	bool header_footer_hidden(const odf_reader::office_element_ptr & elm)
	{
		const odf_reader::header_footer_impl* header_footer = dynamic_cast<odf_reader::header_footer_impl*>(elm.get());
		return header_footer && !header_footer->attlist_.style_display_;
	}
}

void xlsx_table_state::serialize_page_properties (std::wostream & strm)
{
	odf_reader::odf_read_context & odfContext = context_->root()->odf_context();

	odf_reader::page_layout_instance * page_layout = sheet_page_layout(odfContext, table_style_);
	if (!page_layout) return;

	bool header_hidden = false, footer_hidden = false;

	_CP_OPT(std::wstring) masterPageName = odfContext.styleContainer().master_page_name_by_name(table_style_);
	odf_reader::style_master_page* master_style_ = masterPageName ?
		odfContext.pageLayoutContainer().master_page_by_name(*masterPageName) : NULL;
	if (master_style_)
	{
		header_hidden = header_footer_hidden(master_style_->style_header_);
		footer_hidden = header_footer_hidden(master_style_->style_footer_);
	}

	page_layout->xlsx_serialize(strm, *context_, !header_hidden, !footer_hidden);
}
void xlsx_table_state::serialize_header_footer (std::wostream & strm)
{
	_CP_OPT(std::wstring) masterPageName = context_->root()->odf_context().styleContainer().master_page_name_by_name(table_style_);
	if (!masterPageName) return;

	odf_reader::style_master_page* master_style_ = context_->root()->odf_context().pageLayoutContainer().master_page_by_name(*masterPageName);
	if (!master_style_) return;

	odf_reader::style_header		*header_odd		= dynamic_cast<odf_reader::style_header*>		(master_style_->style_header_.get());
	odf_reader::style_header_first	*header_first	= dynamic_cast<odf_reader::style_header_first*>	(master_style_->style_header_first_.get());
	odf_reader::style_header_left	*header_even	= dynamic_cast<odf_reader::style_header_left*>	(master_style_->style_header_left_.get());

	odf_reader::style_footer		*footer_odd		= dynamic_cast<odf_reader::style_footer*>		(master_style_->style_footer_.get());
	odf_reader::style_footer_first	*footer_first	= dynamic_cast<odf_reader::style_footer_first*>	(master_style_->style_footer_first_.get());
	odf_reader::style_footer_left	*footer_even	= dynamic_cast<odf_reader::style_footer_left*>	(master_style_->style_footer_left_.get());

	if (header_footer_hidden(master_style_->style_header_))
	{
		header_odd = NULL; header_first = NULL; header_even = NULL;
	}
	if (header_footer_hidden(master_style_->style_footer_))
	{
		footer_odd = NULL; footer_first = NULL; footer_even = NULL;
	}

	if (!header_odd && !header_first && !header_even && 
		!footer_odd && !footer_first && !footer_even) return;

	CP_XML_WRITER(strm)
	{			
		CP_XML_NODE(L"headerFooter")
		{
			if ((header_even && header_even->attlist_.style_display_) || (footer_even && footer_even->attlist_.style_display_))
			{
				CP_XML_ATTR(L"differentOddEven",  1);
			}
			if ((header_first && header_first->attlist_.style_display_ /*&& !header_first->content_.empty()*/) ||
				(footer_first && footer_first->attlist_.style_display_/* && !footer_first->content_.empty()*/))
			{
				CP_XML_ATTR(L"differentFirst",  1);
			}

			if (header_odd) header_odd->xlsx_serialize(CP_XML_STREAM(), *context_);
			if (footer_odd) footer_odd->xlsx_serialize(CP_XML_STREAM(), *context_);
		
			if (header_even) header_even->xlsx_serialize(CP_XML_STREAM(), *context_);
			if (footer_even) footer_even->xlsx_serialize(CP_XML_STREAM(), *context_);
			
			if (header_first) header_first->xlsx_serialize(CP_XML_STREAM(), *context_);
			if (footer_first) footer_first->xlsx_serialize(CP_XML_STREAM(), *context_);
		}
	}
}
void xlsx_table_state::serialize_background (std::wostream & strm)
{
	if (tableBackground_.empty()) return;

	CP_XML_WRITER(strm)
	{			
		CP_XML_NODE(L"picture")
		{
			CP_XML_ATTR(L"r:id",  tableBackground_);
		}
	}
}
void xlsx_table_state::serialize_protection (std::wostream & strm)
{
	if (!protection.bEnabled) return;
	
	CP_XML_WRITER(strm)
	{
		CP_XML_NODE(L"sheetProtection")
		{
//convert protection odf->ooxml impossible without password !!!
			//CP_XML_ATTR(L"algorithmName", );
			//CP_XML_ATTR(L"hashValue", );
			//CP_XML_ATTR(L"saltValue", );
			//CP_XML_ATTR(L"spinCount", );

			CP_XML_ATTR(L"sheet", 1);
			CP_XML_ATTR(L"objects", 0);
			CP_XML_ATTR(L"scenarios", 0);
			CP_XML_ATTR(L"formatCells", 1);
			CP_XML_ATTR(L"formatColumns", 1);
			CP_XML_ATTR(L"formatRows", 1);
			CP_XML_ATTR(L"insertColumns", protection.insertColumns);
			CP_XML_ATTR(L"insertRows", protection.insertRows);
			CP_XML_ATTR(L"insertHyperlinks", 1);
			CP_XML_ATTR(L"deleteColumns", protection.deleteColumns);
			CP_XML_ATTR(L"deleteRows", protection.deleteRows);
			CP_XML_ATTR(L"selectLockedCells", protection.selectLockedCells);
			CP_XML_ATTR(L"sort", 1);
			CP_XML_ATTR(L"autoFilter", 1);
			CP_XML_ATTR(L"pivotTables", 1);
			CP_XML_ATTR(L"selectUnlockedCells", protection.selectUnockedCells);
		}
	}
}
void xlsx_table_state::serialize_table_format (std::wostream & strm)
{
	odf_reader::odf_read_context & odfContext = context_->root()->odf_context();

	CP_XML_WRITER(strm)
	{
		odf_reader::style_table_properties	* table_prop = NULL;
		odf_reader::style_instance			* tableStyle = odfContext.styleContainer().style_by_name(table_style_, odf_types::style_family::Table, false);
		
		if ((tableStyle) && (tableStyle->content()))
			table_prop = tableStyle->content()->get_style_table_properties();

		bool fit_to_pages = false;
		odf_reader::page_layout_instance * page_layout = sheet_page_layout(odfContext, table_style_);
		if (page_layout && page_layout->properties())
		{
			unsigned int width = 0, height = 0;
			fit_to_pages = page_layout->properties()->attlist_.get_fit_to_pages(width, height);
		}

		if (table_prop || fit_to_pages)
		{
			CP_XML_NODE(L"sheetPr")
			{
				//at filterMode="false">
				if (table_prop && table_prop->content().tableooo_tab_color_)
				{
					CP_XML_NODE(L"tabColor")
					{
						CP_XML_ATTR(L"rgb", table_prop->content().tableooo_tab_color_->get_hex_value());	
					}
				}
				if (fit_to_pages)
				{
					CP_XML_NODE(L"pageSetUpPr")
					{
						CP_XML_ATTR(L"fitToPage", 1);
					}
				}
			}
		}
		int columns = (std::max)(current_table_column_, (int)columns_count_);
		int rows	= (std::max)(current_table_row_,	1);

		if (columns	< 1024 && columns	> 1 &&
			rows	< 1024 && rows		> 1)
		{
			CP_XML_NODE(L"dimension")
			{
				if (current_table_column_ < 0) current_table_column_ = columns_count_;
				std::wstring ref2 = getCellAddress( current_table_column_, current_table_row_);
				CP_XML_ATTR(L"ref", L"A1:" + ref2);
			}
		}
		if (odfContext.Settings().get_views_count() > 0)
		{
			CP_XML_NODE(L"sheetViews")
			{
				CP_XML_NODE(L"sheetView")
				{
					CP_XML_ATTR(L"workbookViewId", 0);

					if (bRTL)
						CP_XML_ATTR(L"rightToLeft", 1); 

					std::wstring s_col, s_row;

					bool bXSplit = false, bYSplit = false;
					int nXSplit = 0, nYSplit = 0;

					for (int i = 0; i < odfContext.Settings().get_table_view_count(0, tableName_); i++)
					{
						std::pair<std::wstring, std::wstring> value = odfContext.Settings().get_table_view(0, tableName_, i);

						if (value.first == L"ZoomValue")
						{
							CP_XML_ATTR(L"zoomScale",		value.second);
							CP_XML_ATTR(L"zoomScaleNormal", value.second);
						}
							 if (value.first == L"ShowGrid")			CP_XML_ATTR(L"showGridLines",	value.second);
						else if (value.first == L"CursorPositionX")		s_col = value.second;
						else if (value.first == L"CursorPositionY")		s_row = value.second;
						
						else if (value.first == L"HorizontalSplitMode")		bXSplit = (value.second == L"2");
						else if (value.first == L"VerticalSplitMode")		bYSplit = (value.second == L"2");
						else if (value.first == L"HorizontalSplitPosition")	nXSplit =  boost::lexical_cast<int>(value.second);
						else if (value.first == L"VerticalSplitPosition")	nYSplit =  boost::lexical_cast<int>(value.second);
					}

					int col = -1, row = -1;
					try
					{
						if (!s_col.empty())		col =  boost::lexical_cast<int>(s_col);
						if (!s_row.empty())		row =  boost::lexical_cast<int>(s_row);
					}
					catch(...){}

					if (bXSplit || bYSplit)
					{
						CP_XML_NODE(L"pane")
						{	
							CP_XML_ATTR(L"xSplit", bXSplit ? nXSplit : 0);			
							CP_XML_ATTR(L"ySplit", bYSplit ? nYSplit : 0);			
							CP_XML_ATTR(L"topLeftCell", getCellAddress(nXSplit, nYSplit));
							//CP_XML_ATTR(L"activePane", L"bottomLeft");
							CP_XML_ATTR(L"activePane", L"topLeft");
							CP_XML_ATTR(L"state", L"frozen");
						}	
					}
					if (col >= 0 && row >= 0)
					{
						CP_XML_NODE(L"selection")
						{	
							CP_XML_ATTR(L"activeCell", getCellAddress(col, row));			
							CP_XML_ATTR(L"activeCellId", 0);			
							//CP_XML_ATTR(L"pane", ((bXSplit || bYSplit) ? L"bottomLeft" : L"topLeft"));			
							CP_XML_ATTR(L"activePane", L"topLeft");
							CP_XML_ATTR(L"sqref", getCellAddress(col, row));
						}						
					}
				}
			}
			//	-showRowColHeaders
		} 

		double default_height = (2 * context_->getMaxDigitSize().second * 72. / 96. * 100.) /100.;//in point size.
		
		odf_reader::style_instance * rowDefStyle = odfContext.styleContainer().style_default_by_type(odf_types::style_family::TableRow);
		if ((rowDefStyle) && (rowDefStyle->content()))
		{
			const odf_reader::style_table_row_properties * prop = rowDefStyle->content()->get_style_table_row_properties();

			if ( (prop) &&  (prop->attlist_.style_row_height_))
			{
				default_height = prop->attlist_.style_row_height_->get_value_unit(odf_types::length::pt);
			}
			std::wstringstream ht_s;
			ht_s.precision(1);
			ht_s << std::fixed << default_height;
			
			CP_XML_NODE(L"sheetFormatPr")
			{
				CP_XML_ATTR(L"defaultRowHeight", ht_s.str());	
			}
		}  
	}

}
void xlsx_table_state::serialize_breaks(std::wostream & strm)
{
	if (column_breaks_.empty() && row_breaks_.empty()) return;

	CP_XML_WRITER(strm)
	{
		if (false == row_breaks_.empty())
		{
			CP_XML_NODE(L"rowBreaks")
			{
				CP_XML_ATTR(L"count", row_breaks_.size());	
				CP_XML_ATTR(L"manualBreakCount", row_breaks_.size());

				for (size_t i = 0; i < row_breaks_.size(); i++)
				{
					CP_XML_NODE(L"brk")
					{
						CP_XML_ATTR(L"id", row_breaks_[i]);	
						CP_XML_ATTR(L"max", 16383);	
						CP_XML_ATTR(L"man", 1);	
					}
				}
			}
		}
		if (false == column_breaks_.empty())
		{
			CP_XML_NODE(L"colBreaks")
			{
				CP_XML_ATTR(L"count", column_breaks_.size());	
				CP_XML_ATTR(L"manualBreakCount", column_breaks_.size());	
				
				for (size_t i = 0; i < column_breaks_.size(); i++)
				{
					CP_XML_NODE(L"brk")
					{
						CP_XML_ATTR(L"id", column_breaks_[i]);	
						CP_XML_ATTR(L"max", 1048575);	
						CP_XML_ATTR(L"man", 1);	
					}
				}			
			}
		}
	}
}
void xlsx_table_state::serialize_merge_cells(std::wostream & strm)
{
    return xlsx_merge_cells_.xlsx_serialize(strm);
}
void xlsx_table_state::serialize_ole_objects(std::wostream & strm)
{
    return xlsx_drawing_context_.get_drawings()->serialize_objects(strm);
}
void xlsx_table_state::serialize_controls(std::wostream & strm)
{
    return xlsx_drawing_context_.get_drawings()->serialize_controls(strm);
}
void xlsx_table_state::serialize_hyperlinks(std::wostream & strm)
{
    return xlsx_hyperlinks_.xlsx_serialize(strm);
}
void xlsx_table_state::serialize_condFormatting(std::wostream & strm)
{
    return xlsx_conditionalFormatting_context_.serialize(strm);
}
void xlsx_table_state::serialize_condFormattingEx(std::wostream& strm)
{
	return xlsx_conditionalFormatting_context_.serializeEx(strm);
}
void xlsx_table_state::dump_rels_hyperlinks(rels & Rels)
{
    return xlsx_hyperlinks_.dump_rels(Rels);
}
void xlsx_table_state::dump_rels_ole_objects(rels & Rels)
{
    return get_drawing_context().get_drawings()->dump_rels_sheet(Rels);
}

void xlsx_table_state::start_hyperlink()
{
}

 std::wstring xlsx_table_state::end_hyperlink(std::wstring const & ref, std::wstring const & href, std::wstring const & display)
{
    return xlsx_hyperlinks_.add(ref, href, display);
}




}
}
