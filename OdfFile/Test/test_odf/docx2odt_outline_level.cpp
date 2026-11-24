// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: outline levels of paragraph styles and paragraphs. w:outlineLvl is
// zero-based and 9 means body text; ODF levels start at 1. A paragraph without its
// own w:outlineLvl takes the level of its style chain, like in Word.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// "h<level>" for the text:h, "p" for the text:p whose text is exactly `text`,
	// or "" if there is none. Fixture paragraphs hold a single run of plain text.
	std::wstring paragraph_kind(const std::wstring& content, const std::wstring& text)
	{
		const size_t pos = content.find(L">" + text + L"</");
		if (pos == std::wstring::npos)
			return L"";
		const size_t start = content.rfind(L'<', pos);
		size_t elem = content.rfind(L"<text:h", start);
		const size_t para = content.rfind(L"<text:p", start);
		if (elem == std::wstring::npos || (para != std::wstring::npos && para > elem))
			return para == std::wstring::npos ? L"" : L"p";
		const size_t end = content.find(L'>', elem);
		const std::wstring tag = content.substr(elem, end - elem);
		const std::wstring attr = L"text:outline-level=\"";
		const size_t lvl = tag.find(attr);
		if (lvl == std::wstring::npos)
			return L"h";
		return L"h" + tag.substr(lvl + attr.size(), tag.find(L'"', lvl + attr.size()) - lvl - attr.size());
	}

	// The style:default-outline-level of the paragraph style `name`, "-" if the
	// attribute is missing, "?" if the style is missing.
	std::wstring default_outline_level(const std::wstring& styles, const std::wstring& name)
	{
		const size_t pos = styles.find(L"style:name=\"" + name + L"\"");
		if (pos == std::wstring::npos)
			return L"?";
		const size_t start = styles.rfind(L'<', pos);
		const std::wstring tag = styles.substr(start, styles.find(L'>', pos) - start);
		const std::wstring attr = L"style:default-outline-level=\"";
		const size_t lvl = tag.find(attr);
		if (lvl == std::wstring::npos)
			return L"-";
		return tag.substr(lvl + attr.size(), tag.find(L'"', lvl + attr.size()) - lvl - attr.size());
	}

	// The fixture is converted once; every test reads the same parts.
	struct Converted
	{
		std::wstring content;
		std::wstring styles;
	};
	const Converted& converted()
	{
		static const Converted parts = []
		{
			Converted result;
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/outline_levels_from_styles.docx");
			if (!dir.empty())
			{
				result.content = read_part(dir, L"content.xml");
				result.styles = read_part(dir, L"styles.xml");
			}
			return result;
		}();
		return parts;
	}

	class DocxToOdtOutlineLevel : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed or content.xml is missing";
			ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
		}
		const std::wstring& content = converted().content;
		const std::wstring& styles = converted().styles;
	};
}

TEST_F(DocxToOdtOutlineLevel, PartsAreWellFormed)
{
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_TRUE(is_well_formed(styles));
}

// Heading 1 (w:outlineLvl 0) and Heading 2 (1) become ODF levels 1 and 2; a style without
// its own level inherits nothing into the attribute; w:outlineLvl 9 (body text) gives an
// empty value, which resets the level inherited from the parent heading style.
TEST_F(DocxToOdtOutlineLevel, StylesGetDefaultOutlineLevel)
{
	EXPECT_EQ(default_outline_level(styles, L"Heading1"), L"1");
	EXPECT_EQ(default_outline_level(styles, L"Heading2"), L"2");
	EXPECT_EQ(default_outline_level(styles, L"HeadingInherited"), L"-");
	EXPECT_EQ(default_outline_level(styles, L"BodyFromHeading"), L"");
	EXPECT_EQ(default_outline_level(styles, L"Caption5"), L"5");
	EXPECT_EQ(default_outline_level(styles, L"Normal"), L"-");
}

// Paragraphs that only have a heading style are headings with the style chain's level.
TEST_F(DocxToOdtOutlineLevel, StyleLevelMakesHeading)
{
	EXPECT_EQ(paragraph_kind(content, L"Heading one"), L"h1");
	EXPECT_EQ(paragraph_kind(content, L"Heading two"), L"h2");
	EXPECT_EQ(paragraph_kind(content, L"Inherited two"), L"h2");
	EXPECT_EQ(paragraph_kind(content, L"Body style"), L"p");
}

TEST_F(DocxToOdtOutlineLevel, DirectLevelWins)
{
	EXPECT_EQ(paragraph_kind(content, L"Direct three"), L"h3");
	EXPECT_EQ(paragraph_kind(content, L"Direct body"), L"p");
}

// w:outlineLvl 9 is body text, not a tenth level; values outside 0..9 are ignored.
TEST_F(DocxToOdtOutlineLevel, LevelNineIsBodyText)
{
	EXPECT_EQ(paragraph_kind(content, L"Body nine"), L"p");
	EXPECT_EQ(paragraph_kind(content, L"Bad level"), L"h2");
}

// A level of a style (the last one in styles.xml has one) does not reach body paragraphs.
TEST_F(DocxToOdtOutlineLevel, PlainParagraphsStayParagraphs)
{
	EXPECT_EQ(paragraph_kind(content, L"Plain first"), L"p");
	EXPECT_EQ(paragraph_kind(content, L"Plain last"), L"p");
}

// A tracked change from a heading style keeps the old style only in the change record;
// the paragraph and the one after it are body text.
TEST_F(DocxToOdtOutlineLevel, OldStyleOfTrackedChangeIsIgnored)
{
	EXPECT_EQ(paragraph_kind(content, L"Was heading"), L"p");
	EXPECT_EQ(paragraph_kind(content, L"After change"), L"p");
}

// ODF allows headings in a text box, but not in the text of other shapes or in comments.
TEST_F(DocxToOdtOutlineLevel, HeadingOnlyWhereOdfAllowsIt)
{
	EXPECT_EQ(paragraph_kind(content, L"Box heading"), L"h1");
	EXPECT_EQ(paragraph_kind(content, L"Shape heading"), L"p");
	EXPECT_EQ(paragraph_kind(content, L"WordArt heading"), L"p");
	EXPECT_EQ(paragraph_kind(content, L"Comment heading"), L"p");
}
