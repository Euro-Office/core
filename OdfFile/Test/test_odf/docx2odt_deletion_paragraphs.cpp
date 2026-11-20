// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: one tracked deletion written as several w:del with the same w:id
// (one per deleted paragraph) must become a single text:changed-region.

#include "gtest/gtest.h"

#include "common.h"

#include <string>
#include <vector>

namespace
{
	// Values of the given attribute on every element that starts with the given tag, in document order.
	std::vector<std::wstring> attribute_values(const std::wstring& xml, const std::wstring& tag, const std::wstring& attr)
	{
		std::vector<std::wstring> values;
		const std::wstring needle = attr + L"=\"";
		for (size_t pos = xml.find(tag); pos != std::wstring::npos; pos = xml.find(tag, pos + 1))
		{
			const size_t end = xml.find(L'>', pos);
			const size_t value = xml.find(needle, pos);
			if (end == std::wstring::npos || value == std::wstring::npos || value > end)
				continue;
			const size_t begin = value + needle.size();
			values.push_back(xml.substr(begin, xml.find(L'"', begin) - begin));
		}
		return values;
	}

	// The text:changed-region element with the given xml:id, or an empty string.
	std::wstring region(const std::wstring& xml, const std::wstring& id)
	{
		const size_t start = xml.find(L"<text:changed-region xml:id=\"" + id + L"\"");
		if (start == std::wstring::npos)
			return L"";
		const std::wstring close = L"</text:changed-region>";
		const size_t end = xml.find(close, start);
		return end == std::wstring::npos ? L"" : xml.substr(start, end + close.size() - start);
	}

	class DocxToOdtDeletionParagraphs : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/deletion_over_paragraph_break.docx");
			if (!dir.empty())
				content = read_part(dir, L"content.xml");
		}
		static std::wstring content;
	};
	std::wstring DocxToOdtDeletionParagraphs::content;
}

TEST_F(DocxToOdtDeletionParagraphs, OneRegionPerChange)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(content));

	const std::vector<std::wstring> ids = attribute_values(content, L"<text:changed-region", L"xml:id");
	EXPECT_EQ(ids, (std::vector<std::wstring>{L"ct5", L"ct7"}));
}

TEST_F(DocxToOdtDeletionParagraphs, OneChangeMarkPerChange)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::vector<std::wstring> marks = attribute_values(content, L"<text:change ", L"text:change-id");
	EXPECT_EQ(marks, (std::vector<std::wstring>{L"ct5", L"ct7"}));
}

TEST_F(DocxToOdtDeletionParagraphs, RegionKeepsEveryDeletedParagraph)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring merged = region(content, L"ct5");
	ASSERT_FALSE(merged.empty());
	const size_t one = merged.find(L"Deleted one.");
	const size_t two = merged.find(L"Deleted two.");
	ASSERT_NE(one, std::wstring::npos);
	ASSERT_NE(two, std::wstring::npos);
	EXPECT_LT(one, two);
	// the two deleted paragraphs and the empty one that ends the deletion
	size_t paragraphs = 0;
	for (size_t pos = merged.find(L"<text:p"); pos != std::wstring::npos; pos = merged.find(L"<text:p", pos + 1))
		++paragraphs;
	EXPECT_EQ(paragraphs, 3u);
}

TEST_F(DocxToOdtDeletionParagraphs, OtherDeletionUnchanged)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring other = region(content, L"ct7");
	ASSERT_FALSE(other.empty());
	EXPECT_NE(other.find(L"gone"), std::wstring::npos);
	EXPECT_EQ(other.find(L"Deleted"), std::wstring::npos);
}

// The same deletion with a footnote inside the deleted text: the note's own paragraph
// must not end the deletion of the paragraph around it.
namespace
{
	class DocxToOdtDeletionAroundNote : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/deletion_around_note.docx");
			if (!dir.empty())
				content = read_part(dir, L"content.xml");
		}
		static std::wstring content;
	};
	std::wstring DocxToOdtDeletionAroundNote::content;
}

TEST_F(DocxToOdtDeletionAroundNote, OneRegionPerChange)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(content));

	const std::vector<std::wstring> ids = attribute_values(content, L"<text:changed-region", L"xml:id");
	EXPECT_EQ(ids, (std::vector<std::wstring>{L"ct5", L"ct7"}));
	const std::vector<std::wstring> marks = attribute_values(content, L"<text:change ", L"text:change-id");
	EXPECT_EQ(marks, (std::vector<std::wstring>{L"ct5", L"ct7"}));
}

TEST_F(DocxToOdtDeletionAroundNote, RegionKeepsTheNoteAndEveryDeletedParagraph)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring merged = region(content, L"ct5");
	ASSERT_FALSE(merged.empty());
	const size_t one = merged.find(L"Deleted one.");
	const size_t note = merged.find(L"Note text.");
	const size_t two = merged.find(L"Deleted two.");
	ASSERT_NE(one, std::wstring::npos);
	ASSERT_NE(note, std::wstring::npos);
	ASSERT_NE(two, std::wstring::npos);
	EXPECT_LT(one, note);
	EXPECT_LT(note, two);
}

// Two deletions apart in one paragraph that reuse one w:id ("Start [AAA] keep [BBB]
// end.") are not joined: each keeps its own region and change mark at its own place,
// under ids that differ, so rejecting them restores each text where it was.
namespace
{
	class DocxToOdtDeletionSameIdApart : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/deletion_same_id_apart.docx");
			if (!dir.empty())
				content = read_part(dir, L"content.xml");
		}
		static std::wstring content;
	};
	std::wstring DocxToOdtDeletionSameIdApart::content;
}

TEST_F(DocxToOdtDeletionSameIdApart, OneRegionPerDeletion)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";
	EXPECT_TRUE(is_well_formed(content));

	const std::vector<std::wstring> ids = attribute_values(content, L"<text:changed-region", L"xml:id");
	ASSERT_EQ(ids.size(), 2u);
	EXPECT_EQ(ids[0], L"ct5");
	EXPECT_NE(ids[1], ids[0]);
	const std::vector<std::wstring> marks = attribute_values(content, L"<text:change ", L"text:change-id");
	EXPECT_EQ(marks, ids);

	const std::wstring first = region(content, ids[0]);
	const std::wstring second = region(content, ids[1]);
	EXPECT_NE(first.find(L"AAA"), std::wstring::npos) << first;
	EXPECT_EQ(first.find(L"BBB"), std::wstring::npos) << first;
	EXPECT_NE(second.find(L"BBB"), std::wstring::npos) << second;
}

TEST_F(DocxToOdtDeletionSameIdApart, MarksStayInPlace)
{
	ASSERT_FALSE(content.empty()) << "DOCX -> ODT conversion failed";

	const size_t body = content.find(L"<office:text");
	ASSERT_NE(body, std::wstring::npos);
	const size_t first = content.find(L"<text:change text:change-id=", body);
	const size_t keep = content.find(L" keep ", body);
	ASSERT_NE(first, std::wstring::npos);
	ASSERT_NE(keep, std::wstring::npos);
	const size_t second = content.find(L"<text:change text:change-id=", first + 1);
	ASSERT_NE(second, std::wstring::npos);
	EXPECT_LT(first, keep);
	EXPECT_LT(keep, second);
}
