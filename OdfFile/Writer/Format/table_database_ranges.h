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
#pragma once

#include <iosfwd>
#include <string>
#include <vector>
#include <CPOptional.h>
#include <xml/xmlelement.h>
#include <xml/nodetype.h>

#include "office_elements_create.h"

#include "../../DataTypes/bool.h"

namespace cpdoccore { 
namespace odf_writer {

/// \brief  one table:filter-condition of a database range filter
struct table_filter_condition_state
{
	int							field_number = 0;	// column offset in the range
	std::wstring				operator_;			// table:operator
	std::wstring				value;
	bool						is_number = false;	// table:data-type="number"
	std::vector<std::wstring>	set_items;			// table:filter-set-item values
};

/// \brief  the conditions of one filtered column, joined by and or by or
struct table_filter_column_state
{
	bool										is_or = false;
	std::vector<table_filter_condition_state>	conditions;
};

/// \brief  table:named-expressions
class table_database_ranges : public office_element_impl<table_database_ranges>
{
public:
    static const wchar_t * ns;
    static const wchar_t * name;

    static const ElementType type = typeTableDatabaseRanges;

    

	virtual void create_child_element(const std::wstring & Ns, const std::wstring & Name);
    virtual void add_child_element( const office_element_ptr & child_element);

    virtual void serialize(std::wostream & _Wostream);

    office_element_ptr_array database_ranges_;

};

CP_REGISTER_OFFICE_ELEMENT2(table_database_ranges);

/// \brief  table:named-range
class table_database_range: public office_element_impl<table_database_range>
{
public:
    static const wchar_t * ns;
    static const wchar_t * name;

    static const ElementType type = typeTableDatabaseRange;

    

	virtual void create_child_element(const std::wstring & Ns, const std::wstring & Name){}
	virtual void add_child_element( const office_element_ptr & child_element){}

    virtual void serialize(std::wostream & _Wostream);
	void serialize_filter_conditions(std::wostream & _Wostream,
		const std::vector<table_filter_condition_state> & conditions);

    _CP_OPT(std::wstring)		table_name_;
    _CP_OPT(std::wstring)		table_target_range_address_;

	_CP_OPT(odf_types::Bool)	table_display_filter_buttons_;

	// table:filter: the columns are joined by and
	std::vector<table_filter_column_state> filter_columns_;

};

CP_REGISTER_OFFICE_ELEMENT2(table_database_range);


}
}
