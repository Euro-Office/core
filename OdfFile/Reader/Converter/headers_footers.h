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
#ifndef _CPDOCCORE_DOCX_HEADERS_FOOTERS_H_
#define _CPDOCCORE_DOCX_HEADERS_FOOTERS_H_

#include <string>
#include <set>
#include <boost/unordered_map.hpp>

#include "oox_rels.h"
namespace cpdoccore { 

namespace odf_reader 
{
	class header_footer_impl;
}

namespace oox {

//class rels;

class headers_footers
{
public:
    headers_footers() : size_(0)
    {
        for (int i = 0; i < none; i++) shown_[i] = false;
    }

    enum Type { header, footer, headerLeft, footerLeft, headerFirst, footerFirst, none };
    std::wstring add(const std::wstring & StyleName, const std::wstring & Content, Type type,rels &_rels);
   
	struct instance 
    {
        instance(const std::wstring & Id,
            const std::wstring & Content,
            Type _Type, const std::wstring & Name)
            : id_(Id), content_(Content), type_(_Type), name_(Name), empty_(false){}
        std::wstring id_;
        std::wstring content_;
        Type type_;
        std::wstring name_;
        bool empty_;	// written only to hide the header/footer of an earlier section
		rels rels_;
    };

    void dump_rels(rels & Rels) const;
    bool write_sectPr(const std::wstring & StyleName, bool next_page, std::wostream & _Wostream);

    // A section without w:headerReference (w:footerReference) shows the header (footer) of the section before it.
    // Sections of this style get an empty one instead where the style has none.
    void set_no_inherit(const std::wstring & StyleName) { no_inherit_.insert(StyleName); }

    typedef boost::shared_ptr<instance> instance_ptr;
    typedef std::vector<instance_ptr> instances_array;
    typedef boost::unordered_map<std::wstring, instances_array> instances_map; 
    
	const instances_map & instances() const { return instances_; }

private:
	std::wstring last_write_style_;
	std::set<std::wstring> no_inherit_;
	bool shown_[none];	// by Type: a section without a reference of that type would show an earlier section's

	static std::wstring create_id(size_t i);
	static std::wstring create_name(size_t i, Type _Type);
	instances_map instances_;
	size_t size_;
   
};

}
}


#endif

