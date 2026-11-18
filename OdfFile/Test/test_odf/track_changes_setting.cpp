// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// The "track changes" (record changes) switch of a text document in both
// directions. ODT keeps it in the text:track-changes attribute of
// <text:tracked-changes> in content.xml, where LibreOffice reads and writes it:
// an element without the attribute means on, no element means off. DOCX keeps
// it in w:trackRevisions in word/settings.xml. Without it, a document opens with
// track changes off after each save, even when it was switched on for all editors.
//
// The track_changes_lo_*.odt fixtures were written by LibreOffice 25.2.3.2
// (headless --convert-to odt) from track_changes_on.docx and
// track_changes_insert_off.docx. track_changes_no_attr.odt is
// track_changes_lo_off.odt with the text:track-changes attribute removed.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	const std::wstring TrackRevisions = L"<w:trackRevisions/>";
}

// LibreOffice, recording on: <text:tracked-changes text:track-changes="true"/>.
TEST(OdtToDocx, LibreOfficeRecordingOnBecomesTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/track_changes_lo_on.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring settings = read_part(dir, L"word/settings.xml");
	ASSERT_FALSE(settings.empty()) << "word/settings.xml is missing";
	EXPECT_TRUE(is_well_formed(settings));
	EXPECT_NE(settings.find(TrackRevisions), std::wstring::npos);
}

// LibreOffice, recording off, one tracked insertion: text:track-changes="false".
TEST(OdtToDocx, LibreOfficeRecordingOffNoTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/track_changes_lo_off.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	ASSERT_NE(read_part(dir, L"word/document.xml").find(L"<w:ins "), std::wstring::npos) << "the tracked insertion was not converted";
	EXPECT_EQ(read_part(dir, L"word/settings.xml").find(L"w:trackRevisions"), std::wstring::npos);
}

// <text:tracked-changes> without the attribute: recording is on (the ODF default).
TEST(OdtToDocx, TrackedChangesWithoutAttributeBecomesTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/track_changes_no_attr.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	ASSERT_NE(read_part(dir, L"word/document.xml").find(L"<w:ins "), std::wstring::npos) << "the tracked insertion was not converted";
	EXPECT_NE(read_part(dir, L"word/settings.xml").find(TrackRevisions), std::wstring::npos);
}

// No <text:tracked-changes>, but the config item RecordChanges in settings.xml.
TEST(OdtToDocx, RecordChangesBecomesTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/track_changes_on.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring settings = read_part(dir, L"word/settings.xml");
	ASSERT_FALSE(settings.empty()) << "word/settings.xml is missing";
	EXPECT_TRUE(is_well_formed(settings));
	EXPECT_NE(settings.find(TrackRevisions), std::wstring::npos);
}

// No <text:tracked-changes> and no RecordChanges: no w:trackRevisions.
TEST(OdtToDocx, NoRecordChangesNoTrackRevisions)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/default_tab_stop_2cm.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";

	const std::wstring settings = read_part(dir, L"word/settings.xml");
	ASSERT_NE(settings.find(L"<w:defaultTabStop "), std::wstring::npos) << "settings were not converted";
	EXPECT_EQ(settings.find(L"w:trackRevisions"), std::wstring::npos);
}

// <w:trackRevisions/> and no changes: the element is written with the attribute "true".
TEST(DocxToOdt, TrackRevisionsBecomesTrackChangesTrue)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/track_changes_on.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_FALSE(content.empty()) << "content.xml is missing";
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_NE(content.find(L"<text:tracked-changes text:track-changes=\"true\""), std::wstring::npos);
}

// A tracked insertion without w:trackRevisions: the attribute must say "false",
// because an element without it means that recording is on.
TEST(DocxToOdt, ChangesWithoutTrackRevisionsBecomeTrackChangesFalse)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/track_changes_insert_off.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_NE(content.find(L"<text:changed-region"), std::wstring::npos) << "the tracked insertion was not converted";
	EXPECT_NE(content.find(L"<text:tracked-changes text:track-changes=\"false\""), std::wstring::npos);
}

// No w:trackRevisions and no changes: no <text:tracked-changes>.
TEST(DocxToOdt, NoTrackRevisionsNoTrackedChanges)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/page_background_image_no_page_size.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";

	const std::wstring content = read_part(dir, L"content.xml");
	ASSERT_NE(content.find(L"BODY"), std::wstring::npos) << "the body was not converted";
	EXPECT_EQ(content.find(L"text:tracked-changes"), std::wstring::npos);
	EXPECT_EQ(read_part(dir, L"settings.xml").find(L"RecordChanges"), std::wstring::npos);
}
