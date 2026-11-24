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

#include "office_text.h"
#include "office_meta.h"

#include <sstream>
#include <vector>

#include <xml/xmlchar.h>
#include <xml/utils.h>
#include "odf_document.h"

#include "serialize_elements.h"
#include "odfcontext.h"
#include "style_paragraph_properties.h"
#include "styles.h"
#include "text_elements.h"
#include "list.h"
#include "documentcontext.h"

namespace cpdoccore { 
namespace odf_reader {

namespace
{
// The body element after content[i], or NULL at the end. Soft page breaks between the two do not count.
const office_element * element_after(const office_element_ptr_array & content, size_t i)
{
	for (++i; i < content.size(); ++i)
	{
		if (content[i] && content[i]->get_type() != typeTextSoftPageBreak)
			return content[i].get();
	}
	return NULL;
}
}

// office:text
//////////////////////////////////////////////////////////////////////////////////////////////////
const wchar_t * office_text::ns		= L"office";
const wchar_t * office_text::name	= L"text";


office_text::office_text()
{}

const cpdoccore::odf_reader::office_element_ptr_array& office_text::get_content()
{
	return content_;
}

void office_text::add_attributes( const xml::attributes_wc_ptr & Attributes )
{
    CP_APPLY_ATTR(L"text:global", text_global_, false);
}
 
bool is_text_content(const std::wstring & ns, const std::wstring & name)
{
    if (ns == L"text")
    {
        return (
            name == L"h" ||
            name == L"p" ||
            name == L"list" ||
            name == L"numbered-paragraph" ||
            name == L"section" ||
            name == L"page-sequence" ||
            name == L"soft-page-break" ||

            name == L"table-of-content" ||
            name == L"illustration-index" ||
            name == L"table-index" ||
            name == L"object-index" ||
            name == L"user-index" ||
            name == L"alphabetical-index" ||
            name == L"bibliography" ||
			name == L"alphabetical-index-auto-mark-file" ||

            name == L"change" ||
            name == L"change-start" ||
            name == L"change-end"

            );
    }
    else if (ns == L"table")
    {
        return name == L"table";
    }
    else if (ns == L"draw" || ns == L"dr3d")
    {
        return true; // all shapes // 
    }
	else if (ns == L"office" && name == L"forms") 
	{
		return true;
	}

    return false;
}

void office_text::add_child_element( xml::sax * Reader, const std::wstring & Ns, const std::wstring & Name)
{
	if CP_CHECK_NAME(L"text", L"tracked-changes") 
	{
		CP_CREATE_ELEMENT(tracked_changes_);
	}
	else if CP_CHECK_NAME(L"table", L"content-validations")
	{
		CP_CREATE_ELEMENT(table_content_validations_);
	}
	else if CP_CHECK_NAME(L"text", L"user-field-decls")
	{
		CP_CREATE_ELEMENT(user_fields_);
	}
	else if CP_CHECK_NAME(L"text", L"sequence-decls")
	{
		CP_CREATE_ELEMENT(sequences_);
	}
	else if CP_CHECK_NAME(L"text", L"variable-decls")
	{
		CP_CREATE_ELEMENT(variables_);
	}
	else if (is_text_content(Ns, Name))
    {
		// elements without a reader class are skipped and add nothing to content_
		if (CP_CREATE_ELEMENT(content_) && !first_element_style_name &&
			(content_.back()->get_type() == typeTextP || content_.back()->get_type() == typeTextH))
		{//bus-modern_l.ott
			if (content_.back()->element_style_name)
				first_element_style_name = content_.back()->element_style_name;
			else
				first_element_style_name = L""; //default

		}
    }
    else
        CP_NOT_APPLICABLE_ELM();
}

namespace
{
// An endnote is printed at the end of the document unless a section around it collects its endnotes.
bool has_endnote_at_document_end(oox::docx_conversion_context & Context, const document_context * context)
{
	if (!context)
		return false;

	const std::vector<document_context::section_entry> & sections = context->sections;
	// for each section: 0 not known yet, 1 it or a section around it collects endnotes, 2 none does
	std::vector<char> collected(sections.size(), 0);
	for (const int innermost : context->endnote_sections)
	{
		if (innermost < 0 || innermost >= static_cast<int>(sections.size()))
			return true; // outside every section

		std::vector<int> path;
		char result = 2;
		// a parent index is always lower than the index of its section: the walk ends
		for (int s = innermost; s >= 0; s = sections[s].parent)
		{
			if (collected[s] != 0)
			{
				result = collected[s];
				break;
			}
			path.push_back(s);

			const style_instance * style = Context.root()->odf_context().styleContainer().style_by_name(
				sections[s].style_name, odf_types::style_family::Section, false);
			const style_section_properties * properties =
				(style && style->content()) ? style->content()->get_style_section_properties() : NULL;
			if (properties && properties->collects_endnotes())
			{
				result = 1;
				break;
			}
		}
		for (size_t i = 0; i < path.size(); i++)
			collected[path[i]] = result;

		if (result == 2)
			return true;
	}
	return false;
}

// The paragraph that ends the body, when the body ends with a paragraph or a list. Not a heading:
// a heading writes the section properties into an empty paragraph of its own before it, so the
// heading would move to the endnote page. Without such a paragraph docx_convert adds one.
const text::paragraph * last_body_paragraph(const office_element_ptr_array & content)
{
	const office_element * element = content.empty() ? NULL : content.back().get();
	while (element)
	{
		if (const text::p * last = dynamic_cast<const text::p *>(element))
			return &last->paragraph_;

		const text::list * last_list = dynamic_cast<const text::list *>(element);
		if (!last_list || last_list->items().empty())
			return NULL;

		const text::list_item * item = dynamic_cast<const text::list_item *>(last_list->items().back().get());
		element = (item && !item->content().empty()) ? item->content().back().get() : NULL;
	}
	return NULL;
}
} // namespace

void office_text::docx_convert(oox::docx_conversion_context & Context)
{
	if (sequences_)
		sequences_->docx_convert(Context);

	if (user_fields_)
		user_fields_->docx_convert(Context);
	
	if (variables_)
		variables_->docx_convert(Context);
	
	if (sequences_)
		sequences_->docx_convert(Context);
	
	if (tracked_changes_)
		tracked_changes_->docx_convert(Context);

 	//if (forms_)
		//forms_->docx_convert(Context);

	Context.start_office_text();

	// LibreOffice prints the endnotes that no section collects on a page of their own after the
	// body, with the page style of the endnote configuration. Word and the editors print them
	// after the last paragraph, so the body ends with a section for that page.
	// With ContinuousEndnotes (set by LibreOffice's import of Word 97 files) LibreOffice prints them
	// right after the text instead.
	const _CP_OPT(std::wstring) continuous_endnotes =
		Context.root()->odf_context().Settings().find_by_name(L"ContinuousEndnotes");
	const bool continuous = continuous_endnotes && *continuous_endnotes == L"true";
	const bool endnote_page = !continuous && has_endnote_at_document_end(Context, getContext());
	const text::paragraph * last_paragraph = endnote_page ? last_body_paragraph(content_) : NULL;

	Context.endnote_page_paragraph_ = last_paragraph;

	_CP_OPT(std::wstring) masterPageName_first;

	if ((first_element_style_name) && (!first_element_style_name->empty()))
	{
		std::wstring style_name = *first_element_style_name;

		masterPageName_first = Context.root()->odf_context().styleContainer().master_page_name_by_name(style_name);
	   
		if (masterPageName_first)
		{				
			std::wstring masterPageNameLayout = Context.root()->odf_context().pageLayoutContainer().page_layout_name_by_style(*masterPageName_first);

			if (false == masterPageNameLayout.empty())
			{
				if (true == Context.set_master_page_name(*masterPageName_first))
				{
					Context.remove_page_properties();
					Context.add_page_properties(masterPageNameLayout);
				}
			}
		}  
	}
	for (size_t i = 0; i < content_.size(); i++)
    {
		if (content_[i]->element_style_name)
		{
			std::wstring style_name = *content_[i]->element_style_name;

			_CP_OPT(std::wstring) masterPageName = Context.root()->odf_context().styleContainer().master_page_name_by_name(*content_[i]->element_style_name);
		   
			if (!masterPageName)
			{
				style_instance* curr_style_instance = Context.root()->odf_context().styleContainer().style_by_name(style_name, odf_types::style_family::Paragraph, false);
				
				if (curr_style_instance && curr_style_instance->content()->get_style_paragraph_properties() &&
					curr_style_instance->content()->get_style_paragraph_properties()->content_.fo_break_before_)
				{
					if (curr_style_instance && false == curr_style_instance->parent_name().empty())
					{
						masterPageName = Context.root()->odf_context().styleContainer().master_page_name_by_name(curr_style_instance->parent_name());
					}
					if (!masterPageName && (style_name == L"Standard" || (curr_style_instance && curr_style_instance->parent_name() == L"Standard")))
					{
						masterPageName = L"Standard";
					}
				}
			}

			_CP_OPT(std::wstring) next_master_name = Context.get_next_master_page_name();
			if (next_master_name)
			{
				masterPageName = *next_master_name;
				Context.set_next_master_page_name(boost::none);
			}

			if (masterPageName)
			{
				std::wstring masterPageNameLayout = Context.root()->odf_context().pageLayoutContainer().page_layout_name_by_style(*masterPageName);

				if (false == masterPageNameLayout.empty())
				{
					if (true == Context.set_master_page_name(*masterPageName))
					{
						Context.remove_page_properties();
						Context.add_page_properties(masterPageNameLayout);
					}
				}
			}
		}
		if (content_[i]->next_element_style_name)
		{
			std::wstring text___ = *content_[i]->next_element_style_name;
			// Check whether the page properties will change.
			// if yes, set the context flag to what is needed in the current paragraph
			// write section properties
			//check... if it is the current one - a duplicate may get registered - and then an unnecessary gap will be drawn
			const _CP_OPT(std::wstring) next_masterPageName	= Context.root()->odf_context().styleContainer().master_page_name_by_name(*content_[i]->next_element_style_name);

			if ((next_masterPageName)  && (Context.get_master_page_name() != *next_masterPageName))
			{
				if (false == Context.root()->odf_context().pageLayoutContainer().compare_page_properties(Context.get_master_page_name(), *next_masterPageName))
				{
					Context.next_dump_page_properties(true);
					//is_empty = false;
				}
			}

			style_instance* next_style_instance = Context.root()->odf_context().styleContainer().style_by_name(text___, odf_types::style_family::Paragraph, false);
			if (next_style_instance)
			{
				style_content* next_style_content = next_style_instance->content();
				if (next_style_content)
				{
					style_paragraph_properties* next_para_props = next_style_content->get_style_paragraph_properties();
					if (next_para_props)
					{
						if (next_para_props->content_.fo_break_before_ && next_para_props->content_.fo_break_before_->get_type() == odf_types::fo_break::Page)
						{
							std::wstring currentMasterPageName = Context.get_master_page_name();
							style_master_page* masterPage = Context.root()->odf_context().pageLayoutContainer().master_page_by_name(currentMasterPageName);

							if (masterPage && masterPage->attlist_.style_next_style_name_)
								Context.set_next_master_page_name(*masterPage->attlist_.style_next_style_name_);

							Context.next_dump_page_properties(true);
						}
					}
				}
			}
		} 
		// A table never gets here through next_element_style_name: the paragraph that knows the style of
		// the element after the table is the last one inside the table. So when that element names another
		// page style, ask for the section properties of the page style that ends before the table is
		// converted: the table writes them into an empty paragraph after itself.
		if (content_[i]->get_type() == typeTableTable && false == Context.get_paragraph_state())
		{
			const office_element * next = element_after(content_, i);
			if (next && next->element_style_name)
			{
				odf_read_context & odfContext = Context.root()->odf_context();
				const _CP_OPT(std::wstring) next_masterPageName =
					odfContext.styleContainer().master_page_name_by_name(*next->element_style_name);

				if ((next_masterPageName) && (Context.get_master_page_name() != *next_masterPageName) &&
					false == odfContext.pageLayoutContainer().compare_page_properties(
						Context.get_master_page_name(), *next_masterPageName))
				{
					Context.next_dump_page_properties(true);
				}
			}
		}
		content_[i]->docx_convert(Context);
    }
	// An empty paragraph of minimal height (the endnote page section and the save in DocxConverter
	// know it by these properties).
	static const wchar_t * minimal_paragraph_properties =
		L"<w:spacing w:before=\"0\" w:after=\"0\" w:line=\"20\" w:lineRule=\"exact\"/>"
		L"<w:rPr><w:sz w:val=\"2\"/><w:szCs w:val=\"2\"/></w:rPr>";

	bool endnote_section = false;
	if (last_paragraph)
	{
		endnote_section = !Context.endnote_page_paragraph_ && Context.get_section_context().dump_.empty();
	}
	else if (endnote_page && Context.get_section_context().dump_.empty() && !Context.get_table_context().in_table())
	{
		// The body ends with a table, a heading or a section: no paragraph of the body can take the
		// section properties of the page style in force, so an empty paragraph of minimal height
		// takes them.
		Context.next_dump_page_properties(true);
		std::wstringstream section_properties;
		if (Context.process_page_properties(section_properties))
		{
			Context.output_stream() << L"<w:p><w:pPr>" << minimal_paragraph_properties
				<< section_properties.str() << L"</w:pPr></w:p>";
			endnote_section = true;
		}
	}
	if (endnote_section)
	{
		// The last paragraph (or the empty one above) took the section properties of the page style
		// in force, so the endnote page gets a section of its own that starts on a new page. Word
		// wants a paragraph in it: an empty one of minimal height, the endnotes follow it. The page
		// style is the one the endnote configuration names, "Endnote" by default as in LibreOffice;
		// a document without that page style keeps the one in force.
		const text_notes_configuration * endnotes =
			Context.root()->odf_context().noteConfiguration().getConfiguration(odf_types::noteclass::Endnote);

		const std::wstring master_page =
			(endnotes && endnotes->text_master_page_name_) ? *endnotes->text_master_page_name_ : L"Endnote";
		const std::wstring layout =
			Context.root()->odf_context().pageLayoutContainer().page_layout_name_by_style(master_page);

		if (false == layout.empty() && Context.set_master_page_name(master_page))
		{
			Context.remove_page_properties();
			Context.add_page_properties(layout);
		}
		Context.output_stream() << L"<w:p><w:pPr>" << minimal_paragraph_properties << L"</w:pPr></w:p>";

		Context.next_dump_page_properties(true);
		Context.endnote_page_section_ = true;
	}
	Context.endnote_page_paragraph_ = NULL;

    Context.end_office_text();
}

void office_text::xlsx_convert(oox::xlsx_conversion_context & Context)
{
    //Context.start_office_text();
    for (size_t i = 0; i < content_.size(); i++)
    {
        content_[i]->xlsx_convert(Context);
    }
    //Context.end_office_text();
}

void office_text::pptx_convert(oox::pptx_conversion_context & Context)
{
    for (size_t i = 0; i < content_.size(); i++)
    {
        content_[i]->pptx_convert(Context);
    }
}

// office:change-info
//-------------------------------------------------------------------------------------------------------------------
const wchar_t * office_change_info::ns		= L"office";
const wchar_t * office_change_info::name	= L"change-info";

void office_change_info::add_attributes( const xml::attributes_wc_ptr & Attributes )
{
	int count = Attributes->size();
	
	CP_APPLY_ATTR(L"office:chg-author", office_chg_author_);
	CP_APPLY_ATTR(L"office:chg-date-time", office_chg_date_time_);
}

void office_change_info::add_child_element( xml::sax * Reader, const std::wstring & Ns, const std::wstring & Name)
{
	if CP_CHECK_NAME(L"dc", L"date")
	{
		CP_CREATE_ELEMENT(dc_date_);
	}
	else if CP_CHECK_NAME(L"dc", L"creator")
	{
		CP_CREATE_ELEMENT(dc_creator_);
	}
	else
		CP_NOT_APPLICABLE_ELM();
}

void office_change_info::docx_convert(oox::docx_conversion_context & Context)
{
	std::wstring date;
 	std::wstring author;
	
	if (dc_date_)
	{
		date = XmlUtils::EncodeXmlString(dynamic_cast<dc_date * >(dc_date_.get())->content_);
	}
	else if (office_chg_date_time_)
	{
		date = *office_chg_date_time_;
	}
	if (dc_creator_)
	{
		author = XmlUtils::EncodeXmlString(dynamic_cast<dc_creator * >(dc_creator_.get())->content_);
	}
	else if (office_chg_author_)
	{
		author = *office_chg_author_;
	}
	
	Context.get_text_tracked_context().set_user_info(author, date);
}

}
}
