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

#include <vector>
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <boost/unordered_map.hpp>

#include "styles_lite_container.h"
#include "office_settings.h"
#include "../../Common/xml/simple_xml_writer.h"
#include "../../../DesktopEditor/common/SystemUtils.h"
#include "../../Common/utils.h"
#include "../../../OOXML/Base/Base.h"

namespace cpdoccore { 

namespace odf_reader {

struct container_element
{
    std::wstring		name;
    office_element_ptr	elm;
};

struct styles_lite_container::Impl
{
  
	std::vector<container_element>			array_;
	boost::unordered_map<std::wstring, int>	map_;
};

styles_lite_container::styles_lite_container(): impl_(new styles_lite_container::Impl() )
{
}

styles_lite_container::~styles_lite_container()
{
}

void styles_lite_container::add(const std::wstring & name, office_element_ptr content)
{
    container_element ref = {name, content};
    
	impl_->array_.push_back(ref);
    impl_->map_[name] = impl_->array_.size() - 1;    
}

office_element_ptr styles_lite_container::find_by_style_name(const std::wstring & name)
{
    boost::unordered_map<std::wstring, int>::const_iterator i = impl_->map_.find(name);
    
	if (i != impl_->map_.end())
    {
        return impl_->array_[i->second].elm;
    }
    else
    {
        return office_element_ptr();
    }
}

namespace
{
	std::wstring trimmed(const std::wstring & value)
	{
		const size_t first = value.find_first_not_of(L" \t\r\n");
		if (first == std::wstring::npos) return L"";
		return value.substr(first, value.find_last_not_of(L" \t\r\n") - first + 1);
	}

	// An ODF float, percentage or currency value as vt:r8 text: the shortest text that reads back as the
	// same number, with '.' as the decimal point. Empty when the value is no finite number.
	std::wstring vt_r8_number(const std::wstring & odf_value)
	{
		const std::wstring s = trimmed(odf_value);
		if (s.empty()) return L"";

		std::wistringstream in(s);
		in.imbue(std::locale::classic());
		double value = 0;
		if (!(in >> value) || !std::isfinite(value)) return L"";
		in >> std::ws;
		if (!in.eof()) return L"";
		// A number too small for a double reads as 0: that is not the value the file holds.
		if (value == 0 && s.substr(0, s.find_first_of(L"eE")).find_first_of(L"123456789") != std::wstring::npos)
			return L"";

		return _doc_props_utils_::shortest_number(value);
	}

	// An ODF boolean value as vt:bool text, empty when it is none.
	std::wstring vt_bool(const std::wstring & odf_value)
	{
		const std::wstring s = trimmed(odf_value);
		if (s == L"true" || s == L"1") return L"true";
		if (s == L"false" || s == L"0") return L"false";
		return L"";
	}
}

class doc_props_container::Impl
{
public:
	Impl() {}

	struct user_defined
	{
		std::wstring value;
		_CP_OPT(odf_types::office_value_type) value_type;
	};
	std::map<std::wstring, user_defined> map_user_defineds;
};

doc_props_container::doc_props_container() : impl_(new doc_props_container::Impl())
{
}

doc_props_container::~doc_props_container()
{
}
void doc_props_container::add_user_defined(const std::wstring & name, const std::wstring & value,
	_CP_OPT(odf_types::office_value_type) value_type)
{
	Impl::user_defined property;
	property.value = value;
	property.value_type = value_type;
	impl_->map_user_defineds.insert(std::make_pair(name, property));
}
std::wstring doc_props_container::get_user_defined(const std::wstring & name)
{
	std::map<std::wstring, Impl::user_defined>::iterator pFind = impl_->map_user_defineds.find(name);

	return pFind != impl_->map_user_defineds.end() ? pFind->second.value : L"";
}
std::wstring doc_props_container::dump_user_defined()
{
	std::wstringstream output;

	CP_XML_WRITER(output)
	{
		int pid = 2;
		for (std::map<std::wstring, Impl::user_defined>::iterator it = impl_->map_user_defineds.begin(); it != impl_->map_user_defineds.end(); ++it)
		{
			// Numbers, yes/no values and dates keep their type, as LibreOffice writes them. A time value
			// (a duration), and any value that does not parse as its type, stays text.
			std::wstring node = L"vt:lpwstr", value = it->second.value;
			if (it->second.value_type)
			{
				std::wstring typed;
				switch (it->second.value_type->get_type())
				{
				case odf_types::office_value_type::Float:
				case odf_types::office_value_type::Percentage:
				case odf_types::office_value_type::Currency:
					typed = vt_r8_number(value);
					if (!typed.empty()) node = L"vt:r8";
					break;
				case odf_types::office_value_type::Boolean:
					typed = vt_bool(value);
					if (!typed.empty()) node = L"vt:bool";
					break;
				case odf_types::office_value_type::Date:
					typed = _doc_props_utils_::w3cdtf_date(value);
					if (!typed.empty()) node = L"vt:filetime";
					break;
				default:
					break;
				}
				if (!typed.empty()) value = typed;
			}
			CP_XML_NODE(L"property")
			{
				CP_XML_ATTR(L"fmtid", L"{D5CDD505-2E9C-101B-9397-08002B2CF9AE}");
				CP_XML_ATTR(L"pid", pid++);
				CP_XML_ATTR(L"name", XmlUtils::EncodeXmlString(it->first));
				CP_XML_NODE(node)
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(value);
				}
			}
		}
	}
	return output.str();
}

std::wstring doc_props_container::dump_custom()
{
	std::wstring user_defined = dump_user_defined();
	if (user_defined.empty()) return L"";

	std::wstringstream output;
	CP_XML_WRITER(output)
	{
		CP_XML_NODE(L"Properties")
		{
			CP_XML_ATTR(L"xmlns", L"http://schemas.openxmlformats.org/officeDocument/2006/custom-properties");
			CP_XML_ATTR(L"xmlns:vt", L"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes");

			CP_XML_STREAM() << user_defined;
		}
	}
	return output.str();
}
std::wstring doc_props_container::dump_core()
{
	std::wstringstream output;
	
	CP_XML_WRITER(output)
	{
		CP_XML_NODE(L"cp:coreProperties")
		{
			CP_XML_ATTR(L"xmlns:cp", L"http://schemas.openxmlformats.org/package/2006/metadata/core-properties");
			CP_XML_ATTR(L"xmlns:xsi", L"http://www.w3.org/2001/XMLSchema-instance");
			CP_XML_ATTR(L"xmlns:dc", L"http://purl.org/dc/elements/1.1/");
			CP_XML_ATTR(L"xmlns:dcmitype", L"http://purl.org/dc/dcmitype/");
			CP_XML_ATTR(L"xmlns:dcterms", L"http://purl.org/dc/terms/");
			
			const std::wstring created = _doc_props_utils_::w3cdtf_date(creation_date_);
			if (!created.empty())
			{
				CP_XML_NODE(L"dcterms:created")
				{
					CP_XML_ATTR(L"xsi:type", L"dcterms:W3CDTF");
					CP_XML_STREAM() << created;
				}
			}
			const std::wstring modified = _doc_props_utils_::w3cdtf_date(dc_date_);
			if (!modified.empty())
			{
				CP_XML_NODE(L"dcterms:modified")
				{
					CP_XML_ATTR(L"xsi:type", L"dcterms:W3CDTF");
					CP_XML_STREAM() << modified;
				}
			}
			// OOXML dc:creator is the author who created the document (ODF meta:initial-creator),
			// cp:lastModifiedBy the one who saved it last (ODF dc:creator). Files without
			// meta:initial-creator, such as the ones older versions of this converter wrote, keep
			// dc:creator as the author too: otherwise the next save would lose the only name they have.
			const std::wstring & author = initial_creator_.empty() ? dc_creator_ : initial_creator_;
			if (!author.empty())
			{
				CP_XML_NODE(L"dc:creator")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(author);
				}
			}
			if (!dc_title_.empty())
			{
				CP_XML_NODE(L"dc:title")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(dc_title_);
				}
			}
			if (!dc_subject_.empty())
			{
				CP_XML_NODE(L"dc:subject")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(dc_subject_);
				}
			}
			if (!dc_description_.empty())
			{
				CP_XML_NODE(L"dc:description")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(dc_description_);
				}
			}
			if (!dc_language_.empty())
			{
				CP_XML_NODE(L"dc:language")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(dc_language_);
				}
			}
			// Only a person who saved the document: the application name here would be saved back to ODF
			// as the last author.
			if (!dc_creator_.empty())
			{
				CP_XML_NODE(L"cp:lastModifiedBy")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(dc_creator_);
				}
			}
			if (!keyword_.empty())
			{
				CP_XML_NODE(L"cp:keywords")
				{
					CP_XML_STREAM() << XmlUtils::EncodeXmlString(keyword_);
				}
			}
			if (revision_)
			{
				CP_XML_NODE(L"cp:revision")
				{
					CP_XML_STREAM() << *revision_;
				}
			}
		}
	}
	return output.str();
}

//----------------------------------------------------------------------------------
struct settings_value
{
    std::wstring name;
	std::wstring value;
};

struct settings_elm
{
	std::vector<settings_value>				array_;
    boost::unordered_map<std::wstring, int>	map_;
};

struct view_elm : settings_elm
{
	boost::unordered_map<std::wstring, int>	map_tables;
	std::vector<settings_elm>				tables;
};
//------------------------------------------------------------------------------------
class settings_container::Impl
{
public:
    Impl() : inViewTable(false), inView(false) {}

	settings_elm				common; 
    settings_elm				common_view;

	std::vector<view_elm>		views;

	bool inViewTable;
	bool inView;

	_CP_OPT(double) tab_distance_;

	std::map<std::wstring, std::wstring> map_user_defineds;
};

settings_container::settings_container(): impl_(new settings_container::Impl() )
{
}

settings_container::~settings_container()
{
}

void settings_container::add(const std::wstring & name, const std::wstring & value)
{
    settings_value ref = {name, value};
    
	impl_->common.array_.push_back(ref);
    impl_->common.map_[name] = impl_->common.array_.size() - 1;    
}

void settings_container::start_view ()
{
	impl_->inView = true;

	view_elm elm;
	impl_->views.push_back(elm);
}

void settings_container::end_view ()
{
	impl_->inView = false;
}

void settings_container::start_table_view (const std::wstring & name)
{
	if (name.empty()) return;

	impl_->inViewTable = true;
	
	settings_elm elm;

	impl_->views.back().tables.push_back(elm);
    impl_->views.back().map_tables[name] = impl_->views.back().tables.size() - 1;    
}

void settings_container::end_table_view ()
{
	impl_->inViewTable = false;
}

int settings_container::get_views_count()
{
	return impl_->views.size();
}

int settings_container::get_table_view_count(int ind, std::wstring name)
{
	if (ind < 0 || ind >= (int)impl_->views.size()) return 0;
	
    boost::unordered_map<std::wstring, int>::const_iterator i = impl_->views[ind].map_tables.find(name);
	
	if (i != impl_->views[ind].map_tables.end())
	{
		return impl_->views[ind].tables[i->second].array_.size();
	}
	
	return 0;
}

void settings_container::add_view(const std::wstring & name, const std::wstring & value)
{
    settings_value ref = {name, value};
    
	if (impl_->inView)
	{
		if (impl_->inViewTable)
		{
			impl_->views.back().tables.back().array_.push_back(ref);
			impl_->views.back().tables.back().map_[name] = impl_->views.back().tables.back().array_.size() - 1;   
		}
		else
		{
			impl_->views.back().array_.push_back(ref);
			impl_->views.back().map_[name] = impl_->views.back().array_.size() - 1;   
		}
	}
	else
	{
		impl_->common_view.array_.push_back(ref);
		impl_->common_view.map_[name] = impl_->common_view.array_.size() - 1;    
	}
}

void settings_container::set_tab_distance(double pt)
{
	impl_->tab_distance_ = pt * 20;
}

_CP_OPT(double) settings_container::get_tab_distance()
{
	return impl_->tab_distance_;
}

std::pair<std::wstring, std::wstring> settings_container::get_table_view(int index_view, const std::wstring & table_name, int index)
{
	std::pair<std::wstring, std::wstring> value;
	
	if (index		< 0) return value;
	if (index_view	< 0 || 
		index_view	>= (int)impl_->views.size()) return value;
	
    boost::unordered_map<std::wstring, int>::const_iterator i = impl_->views[index_view].map_tables.find(table_name);
	
	if (i == impl_->views[index_view].map_tables.end()) return value;
		
	if (index < (int)impl_->views[index_view].tables[i->second].array_.size())
	{
		value.first		= impl_->views[index_view].tables[i->second].array_[index].name;
		value.second	= impl_->views[index_view].tables[i->second].array_[index].value;
	}

	return value;
}
_CP_OPT(std::wstring) settings_container::find_by_name(const std::wstring & name)
{
	_CP_OPT(std::wstring) value;
    boost::unordered_map<std::wstring, int>::const_iterator i = impl_->common.map_.find(name);
    
	if (i != impl_->common.map_.end())
    {
		value = impl_->common.array_[i->second].value;
    }

	return value;
}

_CP_OPT(std::wstring) settings_container::find_view_by_name(const std::wstring & name, int ind)
{
	_CP_OPT(std::wstring) value;

	if (ind < 0 || ind >= (int)impl_->views.size())
	{
		boost::unordered_map<std::wstring, int>::const_iterator i = impl_->common_view.map_.find(name);
	    
		if (i != impl_->common_view.map_.end())
		{
			value = impl_->common_view.array_[i->second].value;
		}
	}
	else
	{
		boost::unordered_map<std::wstring, int>::const_iterator i = impl_->views[ind].map_.find(name);
	    
		if (i != impl_->views[ind].map_.end())
		{
			value = impl_->views[ind].array_[i->second].value;
		}
		else
		{
			return find_view_by_name(name);
		}
	}
	return value;
}

}
}
