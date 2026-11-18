// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: drawings in the header of a section that starts with a table.
// The header is converted where its section starts, so the table of the body
// is open at that moment. That table is not around the header: a drawing in
// a plain header paragraph is not in a table cell, a drawing in a table of
// the header is.
//
// header_drawing_section_starts_with_table.docx has four sections on their
// own pages: S0 (starts with a paragraph, header A), S1 (starts with a table,
// header A), S2 (starts with a table, header B) and the body section (starts
// with a paragraph, header B). Header A holds an anchored line with
// layoutInCell="1" and a w:object shape in plain paragraphs, header B holds
// the same two drawings in a table cell. The first cell of the table that
// starts S1 holds the anchored line once more, as a drawing of the body.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	const wchar_t* const kFixture = L"ExampleFiles/header_drawing_section_starts_with_table.docx";

	struct HeaderDrawings
	{
		std::wstring master_page;
		bool at_a_table = false;		// a table style of the body names the master page
		std::wstring line_anchor;
		std::wstring line_properties;	// style:graphic-properties of the line's style
		std::wstring shape_anchor;
		std::wstring shape_properties;
	};

	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = pos + key.size();
		const size_t end = tag.find(L'"', start);
		return end == std::wstring::npos ? L"" : tag.substr(start, end - start);
	}

	// The start tag of the first <name ...> element that begins in [from, to).
	std::wstring start_tag(const std::wstring& xml, const std::wstring& name, size_t from, size_t to)
	{
		const size_t pos = xml.find(L"<" + name + L" ", from);
		if (pos == std::wstring::npos || pos >= to)
			return L"";
		const size_t end = xml.find(L'>', pos);
		return end == std::wstring::npos ? L"" : xml.substr(pos, end - pos + 1);
	}

	// The start tag of the style with this name and family.
	std::wstring style_tag(const std::wstring& xml, const std::wstring& family, const std::wstring& name,
						   size_t* after)
	{
		size_t pos = 0;
		while ((pos = xml.find(L"<style:style ", pos)) != std::wstring::npos)
		{
			const size_t end = xml.find(L'>', pos);
			if (end == std::wstring::npos)
				break;
			const std::wstring tag = xml.substr(pos, end - pos + 1);
			if (attribute(tag, L"style:family") == family && attribute(tag, L"style:name") == name)
			{
				*after = end;
				return tag;
			}
			pos = end;
		}
		return L"";
	}

	std::wstring graphic_properties(const std::wstring& styles, const std::wstring& drawing_tag)
	{
		const std::wstring name = attribute(drawing_tag, L"draw:style-name");
		size_t after = 0;
		const std::wstring style = name.empty() ? L"" : style_tag(styles, L"graphic", name, &after);
		if (style.empty() || style[style.size() - 2] == L'/')
			return L"";
		return start_tag(styles, L"style:graphic-properties", after, styles.find(L"</style:style>", after));
	}

	bool named_by_a_table_style(const std::wstring& content, const std::wstring& master_page)
	{
		size_t pos = 0;
		while ((pos = content.find(L"<style:style ", pos)) != std::wstring::npos)
		{
			const size_t end = content.find(L'>', pos);
			if (end == std::wstring::npos)
				break;
			const std::wstring tag = content.substr(pos, end - pos + 1);
			if (attribute(tag, L"style:family") == L"table" && attribute(tag, L"style:master-page-name") == master_page)
				return true;
			pos = end;
		}
		return false;
	}

	// The drawings of every master page whose header holds the marker text.
	std::vector<HeaderDrawings> header_drawings(const std::wstring& dir, const std::wstring& marker)
	{
		std::vector<HeaderDrawings> result;
		const std::wstring styles = read_part(dir, L"styles.xml");
		const std::wstring content = read_part(dir, L"content.xml");
		if (!is_well_formed(styles) || !is_well_formed(content))
			return result;

		const std::wstring open = L"<style:master-page ";
		const std::wstring close = L"</style:master-page>";
		size_t pos = 0;
		while ((pos = styles.find(open, pos)) != std::wstring::npos)
		{
			const size_t end = styles.find(close, pos);
			if (end == std::wstring::npos)
				break;
			const size_t text = styles.find(marker, pos);
			if (text != std::wstring::npos && text < end)
			{
				HeaderDrawings page;
				page.master_page = attribute(start_tag(styles, L"style:master-page", pos, end), L"style:name");
				page.at_a_table = named_by_a_table_style(content, page.master_page);

				const std::wstring line = start_tag(styles, L"draw:line", pos, end);
				page.line_anchor = attribute(line, L"text:anchor-type");
				page.line_properties = graphic_properties(styles, line);

				const std::wstring shape = start_tag(styles, L"draw:rect", pos, end);
				page.shape_anchor = attribute(shape, L"text:anchor-type");
				page.shape_properties = graphic_properties(styles, shape);

				result.push_back(page);
			}
			pos = end;
		}
		return result;
	}
}

// Header A in S0 (converted at a paragraph) and in S1 (converted inside the
// first table of S1): its drawings are the same in both master pages.
TEST(DocxToOdtHeaderDrawingAnchor, SectionThatStartsWithATableKeepsTheDrawingsOfItsHeader)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(kFixture);
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::vector<HeaderDrawings> pages = header_drawings(dir, L"HEADER-A");
	ASSERT_GE(pages.size(), 2u);

	bool at_a_table = false, at_a_paragraph = false;
	for (size_t i = 0; i < pages.size(); ++i)
	{
		if (pages[i].at_a_table)
			at_a_table = true;
		else
			at_a_paragraph = true;

		EXPECT_EQ(pages[i].line_anchor, L"paragraph") << pages[i].master_page;
		EXPECT_EQ(attribute(pages[i].line_properties, L"style:vertical-rel"), L"paragraph") << pages[i].master_page;
		EXPECT_EQ(pages[i].line_properties, pages[0].line_properties) << pages[i].master_page;

		EXPECT_EQ(pages[i].shape_anchor, L"as-char") << pages[i].master_page;
		EXPECT_EQ(attribute(pages[i].shape_properties, L"style:run-through"), L"") << pages[i].master_page;
		EXPECT_EQ(pages[i].shape_properties, pages[0].shape_properties) << pages[i].master_page;
	}
	EXPECT_FALSE(pages[0].line_properties.empty());
	EXPECT_FALSE(pages[0].shape_properties.empty());
	EXPECT_TRUE(at_a_table) << "no master page with header A starts at a table";
	EXPECT_TRUE(at_a_paragraph) << "no master page with header A starts at a paragraph";
}

// Header B in S2 (converted inside the first table of S2) and in the body
// section: its drawings sit in a table of the header and stay cell drawings.
TEST(DocxToOdtHeaderDrawingAnchor, DrawingsInATableOfTheHeaderStayCellDrawings)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(kFixture);
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::vector<HeaderDrawings> pages = header_drawings(dir, L"HEADER-B");
	ASSERT_GE(pages.size(), 2u);

	bool at_a_table = false, at_a_paragraph = false;
	for (size_t i = 0; i < pages.size(); ++i)
	{
		if (pages[i].at_a_table)
			at_a_table = true;
		else
			at_a_paragraph = true;

		EXPECT_EQ(pages[i].line_anchor, L"char") << pages[i].master_page;
		EXPECT_EQ(attribute(pages[i].line_properties, L"style:vertical-rel"), L"paragraph-content")
			<< pages[i].master_page;
		EXPECT_EQ(pages[i].line_properties, pages[0].line_properties) << pages[i].master_page;

		EXPECT_EQ(pages[i].shape_anchor, L"as-char") << pages[i].master_page;
		EXPECT_EQ(attribute(pages[i].shape_properties, L"style:run-through"), L"background") << pages[i].master_page;
		EXPECT_EQ(pages[i].shape_properties, pages[0].shape_properties) << pages[i].master_page;
	}
	EXPECT_TRUE(at_a_table) << "no master page with header B starts at a table";
	EXPECT_TRUE(at_a_paragraph) << "no master page with header B starts at a paragraph";
}

// The header of S1 is converted from inside the first cell of S1's table. A
// drawing of the body in that cell, converted afterwards, is a cell drawing.
TEST(DocxToOdtHeaderDrawingAnchor, DrawingInTheTableOfTheBodyStaysACellDrawing)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(kFixture);
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_TRUE(is_well_formed(content));

	const size_t cell = content.find(L"<table:table-cell");
	ASSERT_NE(cell, std::wstring::npos);
	const std::wstring line = start_tag(content, L"draw:line", cell, content.find(L"</table:table-cell>", cell));
	ASSERT_EQ(attribute(line, L"draw:name"), L"Rule 3");

	EXPECT_EQ(attribute(line, L"text:anchor-type"), L"char");
	EXPECT_EQ(attribute(graphic_properties(content, line), L"style:vertical-rel"), L"paragraph-content");
}
