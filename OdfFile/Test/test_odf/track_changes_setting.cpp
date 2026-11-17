// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// The "track changes" switch of a text document in both directions:
// config item RecordChanges in settings.xml (ODT) and w:trackRevisions in
// word/settings.xml (DOCX). Without it, a document opens with track changes
// off after each save, even when it was switched on for all editors.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// settings.xml with <config:config-item config:name="RecordChanges">true</...>.
TEST(OdtToDocx, RecordChangesBecomesTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/track_changes_on.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring settings = read_part(dir, L"word/settings.xml");
	ASSERT_FALSE(settings.empty()) << "word/settings.xml is missing";
	EXPECT_TRUE(is_well_formed(settings));
	EXPECT_NE(settings.find(L"<w:trackRevisions/>"), std::wstring::npos);
}

// A document without RecordChanges must not get w:trackRevisions.
TEST(OdtToDocx, NoRecordChangesNoTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/default_tab_stop_2cm.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring settings = read_part(dir, L"word/settings.xml");
	ASSERT_NE(settings.find(L"<w:defaultTabStop "), std::wstring::npos) << "settings were not converted";
	EXPECT_EQ(settings.find(L"w:trackRevisions"), std::wstring::npos);
}

// word/settings.xml with <w:trackRevisions/>.
TEST(DocxToOdt, TrackRevisionsBecomesRecordChanges)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/track_changes_on.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring settings = read_part(dir, L"settings.xml");
	ASSERT_FALSE(settings.empty()) << "settings.xml is missing";
	EXPECT_TRUE(is_well_formed(settings));
	EXPECT_NE(settings.find(L"config:name=\"RecordChanges\" config:type=\"boolean\">true<"), std::wstring::npos);
}

// A document without w:trackRevisions must not get RecordChanges.
TEST(DocxToOdt, NoTrackRevisionsNoRecordChanges)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/page_background_image_no_page_size.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	ASSERT_NE(read_part(dir, L"content.xml").find(L"BODY"), std::wstring::npos) << "the body was not converted";
	EXPECT_EQ(read_part(dir, L"settings.xml").find(L"RecordChanges"), std::wstring::npos);
}
