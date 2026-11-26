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

#include "headers_footers.h"
#include <boost/lexical_cast.hpp>

namespace cpdoccore { 
namespace oox {

std::wstring headers_footers::create_id(size_t i)
{
    return std::wstring(L"rHFId") + std::to_wstring(i); 
}

std::wstring headers_footers::create_name(size_t i, headers_footers::Type _Type)
{
    return ((_Type == header || _Type == headerLeft || _Type == headerFirst) ?  std::wstring(L"header") : std::wstring(L"footer") )
        + std::to_wstring(i) + L".xml";
}

std::wstring headers_footers::add(const std::wstring & StyleName,
                                  const std::wstring & Content,
                                  Type type,
								  rels &_rels
                                  )
{
	std::wstring id, name;
	if (type != headers_footers::none)
	{
		size_++;
		
		id = create_id(size_);
		name = create_name(size_, type);
	}
    instance_ptr inst = instance_ptr( new instance(id, Content, type, name) );
  
	std::vector<relationship> & rels = _rels.relationships();
	for (size_t i = 0; i < rels.size(); i++)
    {
		inst->rels_.add(rels[i]);
	}
    instances_[StyleName].push_back(inst);
	return id;
}

namespace 
{

std::wstring get_rel_type(headers_footers::Type _Type)
{
    if (_Type == headers_footers::header || _Type == headers_footers::headerLeft || _Type == headers_footers::headerFirst)
        return std::wstring(L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/header");
    else
        return std::wstring(L"http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer");
}
    
// A header or footer that shows nothing and takes next to no room: one paragraph, as low as a paragraph can be.
std::wstring empty_content(bool header)
{
	const std::wstring tag = header ? L"w:hdr" : L"w:ftr";
	return L"<" + tag + L" xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:p><w:pPr>"
		L"<w:spacing w:before=\"0\" w:after=\"0\" w:line=\"1\" w:lineRule=\"exact\"/>"
		L"<w:rPr><w:sz w:val=\"2\"/><w:szCs w:val=\"2\"/></w:rPr></w:pPr></w:p></" + tag + L">";
}

}

void headers_footers::dump_rels(rels & Rels) const//external rels
{
	for (boost::unordered_map<std::wstring, instances_array>::const_iterator it = instances_.begin(); it != instances_.end(); ++it)
    {
		for (size_t i = 0; i < it->second.size(); i++)
        {
			if (it->second[i]->type_ == none) continue;
            Rels.add( relationship( it->second[i]->id_,  get_rel_type(it->second[i]->type_),  it->second[i]->name_, L"" ) );
        }
    }        
}

bool headers_footers::write_sectPr(const std::wstring & StyleName, bool next_page/*not used*/, std::wostream & _Wostream)
{
    if (!instances_.count(StyleName))	return false;
	if (last_write_style_ == StyleName)	return true;

	last_write_style_ = StyleName;

	bool first	=	false, left	=	false;
	bool res	=	false;
	bool own[none], hidden[none];	// by Type: the style's own header/footer, an empty one in its place
	for (int i = 0; i < none; i++) own[i] = hidden[i] = false;

	instances_array & pFind = instances_.at(StyleName);
   
	for (size_t i = 0; i < pFind.size(); i++)
    {
		instance_ptr & inst = pFind[i];

		if (inst->type_ != none)
			(inst->empty_ ? hidden : own)[inst->type_] = true;

        std::wstring type = L"default";
		
		if ( inst->type_ == headerFirst || inst->type_ == footerFirst )
		{
			type = L"first";
			first = true;
		}
		else if ( inst->type_ == footerLeft || inst->type_ == headerLeft )
		{
			type = L"even";
			left = true;
		}

        std::wstring name;  
		if (inst->type_ == footer || inst->type_ == footerLeft  || inst->type_ == footerFirst ) 
			name =L"w:footerReference";
		else if (inst->type_ == header || inst->type_ == headerLeft  || inst->type_ == headerFirst ) 
			name =L"w:headerReference";

		if (!name.empty())
		{
			_Wostream << L"<" << name << L" r:id=\"" << inst->id_ << L"\" w:type=\"" << type << "\" />";
		}
		res=true;
	}

	if (no_inherit_.count(StyleName))
	{
		// The style has no header (footer) at all, an earlier section has one: the section would show that one.
		for (int footers = 0; footers < 2; footers++)
		{
			const Type			kinds[3] = { footers ? footer : header, footers ? footerLeft : headerLeft,
											 footers ? footerFirst : headerFirst };
			const wchar_t *		types[3] = { L"default", L"even", L"first" };

			if (own[kinds[0]] || own[kinds[1]] || own[kinds[2]]) continue;

			for (int i = 0; i < 3; i++)
			{
				if (!shown_[kinds[i]] || hidden[kinds[i]]) continue;
				if (kinds[i] == (footers ? footerFirst : headerFirst) && !first) continue; // not shown without w:titlePg

				rels no_rels;
				const std::wstring id = add(StyleName, empty_content(!footers), kinds[i], no_rels);
				instances_.at(StyleName).back()->empty_ = true;
				hidden[kinds[i]] = true;

				_Wostream << (footers ? L"<w:footerReference" : L"<w:headerReference") << L" r:id=\"" << id
					<< L"\" w:type=\"" << types[i] << L"\" />";
			}
		}
	}
	for (int i = 0; i < none; i++)
	{
		if (own[i])			shown_[i] = true;
		else if (hidden[i])	shown_[i] = false;
	}
	
	if (first)
	{
		_Wostream << L"<w:titlePg/>";
	}
	return res;
}

}
}
