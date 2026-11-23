// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: w:basedOn becomes style:parent-style-name only when it names a
// style of the same type. A w:basedOn naming a style of another type is
// ignored (ISO/IEC 29500-1 17.7.4.3), so the ODT style gets no parent.

#include "gtest/gtest.h"

#include "common.h"

#include <map>
#include <string>

namespace
{
	// Value of attribute `name` in the start tag `tag`, or "" when it is missing.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t at = tag.find(key);
		if (at == std::wstring::npos)
			return L"";
		const size_t begin = at + key.size();
		const size_t end = tag.find(L'"', begin);
		return end == std::wstring::npos ? L"" : tag.substr(begin, end - begin);
	}

	struct OdfStyle
	{
		std::wstring family;
		std::wstring parent;
	};

	// Every style:style of an ODF XML part by style:name.
	std::map<std::wstring, OdfStyle> styles_by_name(const std::wstring& xml)
	{
		std::map<std::wstring, OdfStyle> result;
		for (size_t at = xml.find(L"<style:style "); at != std::wstring::npos; at = xml.find(L"<style:style ", at + 1))
		{
			const size_t end = xml.find(L'>', at);
			if (end == std::wstring::npos)
				break;
			const std::wstring tag = xml.substr(at, end - at);

			OdfStyle style;
			style.family = attribute(tag, L"style:family");
			style.parent = attribute(tag, L"style:parent-style-name");
			result[attribute(tag, L"style:name")] = style;
		}
		return result;
	}
}

// Styles of the fixture: Normal (paragraph, default), Body (paragraph, based on
// Normal), Emph (character, based on the paragraph style Normal) and Indented
// (paragraph, based on the character style Emph).
TEST(DocxToOdtStyleBasedOn, ParentOfAnotherTypeIsIgnored)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/character_style_based_on_paragraph_style.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring stylesXml = read_part(dir, L"styles.xml");
	const std::wstring contentXml = read_part(dir, L"content.xml");
	ASSERT_TRUE(is_well_formed(stylesXml));
	ASSERT_TRUE(is_well_formed(contentXml));

	const std::map<std::wstring, OdfStyle> styles = styles_by_name(stylesXml);
	ASSERT_EQ(styles.count(L"Body"), 1u) << stylesXml;
	ASSERT_EQ(styles.count(L"Emph"), 1u) << stylesXml;
	ASSERT_EQ(styles.count(L"Indented"), 1u) << stylesXml;

	// Same type: the parent is kept.
	EXPECT_EQ(styles.at(L"Body").family, L"paragraph") << stylesXml;
	EXPECT_EQ(styles.at(L"Body").parent, L"Normal") << stylesXml;

	// Another type: no parent.
	EXPECT_EQ(styles.at(L"Emph").family, L"text") << stylesXml;
	EXPECT_EQ(styles.at(L"Emph").parent, L"") << stylesXml;
	EXPECT_EQ(styles.at(L"Indented").family, L"paragraph") << stylesXml;
	EXPECT_EQ(styles.at(L"Indented").parent, L"") << stylesXml;

	// Every parent, in styles.xml and in the automatic styles of content.xml,
	// names a style of the same family in styles.xml.
	const std::map<std::wstring, OdfStyle> automatic = styles_by_name(contentXml);
	for (const std::map<std::wstring, OdfStyle>* part : { &styles, &automatic })
	{
		for (const auto& entry : *part)
		{
			if (entry.second.parent.empty())
				continue;
			ASSERT_EQ(styles.count(entry.second.parent), 1u) << "unknown parent of " << entry.first;
			EXPECT_EQ(styles.at(entry.second.parent).family, entry.second.family) << "parent of " << entry.first;
		}
	}

	// The run still uses the character style.
	bool runUsesEmph = false;
	for (const auto& entry : automatic)
		if (entry.second.family == L"text" && entry.second.parent == L"Emph")
			runUsesEmph = true;
	EXPECT_TRUE(runUsesEmph) << contentXml;
}
