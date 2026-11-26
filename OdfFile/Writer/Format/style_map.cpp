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

#include "style_map.h"

#include <xml/simple_xml_writer.h>
#include <xml/xmlchar.h>
#include <xml/attributes.h>


namespace cpdoccore { 

namespace odf_writer {

// style:map
//////////////////////////////////////////////////////////////////////////////////////////////////
const wchar_t * style_map::ns = L"style";
const wchar_t * style_map::name = L"map";

void style_map::create_child_element(  const std::wstring & Ns, const std::wstring & Name)
{
    CP_NOT_APPLICABLE_ELM();
}
void style_map::serialize(std::wostream & strm)
{
	CP_XML_WRITER(strm)
	{
		CP_XML_NODE_SIMPLE()
		{
			CP_XML_ATTR(L"style:condition", style_condition_);
			CP_XML_ATTR(L"style:apply-style-name", style_apply_style_name_);
			if (false == style_base_cell_address_.empty())
				CP_XML_ATTR(L"style:base-cell-address", style_base_cell_address_);
		}
	}
}

}
}
