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

#include <set>
#include <string>
#include <vector>

namespace cpdoccore { 
namespace odf_reader {

class office_element;

class document_context
{
public:
    document_context();
    virtual ~document_context();
				
	office_element* get_last_paragraph();
	office_element* get_last_element();

	std::wstring office_class_; //openoffice xml 1.0
	bool is_old_version;  //openoffice xml 1.0

	std::vector<office_element*> levels;
	office_element* last_paragraph;

	// every text:section of the document in reading order (section::add_attributes): the index of the
	// section around it (-1 at the top, always lower than its own index) and its style name. Indexes,
	// not pointers: a duplicate child element replaces and frees the subtree that was read before it.
	struct section_entry
	{
		int parent;
		std::wstring style_name;
	};
	std::vector<section_entry> sections;
	// the innermost text:section around each endnote of the document as an index into sections, -1 for
	// an endnote outside every section: one entry per section, not per endnote (office_text::docx_convert)
	std::set<int> endnote_sections;
};

}
}
