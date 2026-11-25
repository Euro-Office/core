// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODF ShowRedlineChanges (ooo:view-settings) <-> Word w:revisionView w:insDel:
// a document saved with the markup of its tracked insertions and deletions hidden.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{

std::wstring docx_settings(const wchar_t* fixture)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(std::wstring(L"ExampleFiles/") + fixture);
    if (dir.empty())
        return L"";
    return read_part(dir, L"word/settings.xml");
}

std::wstring odt_settings(const wchar_t* fixture)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(std::wstring(L"ExampleFiles/") + fixture);
    if (dir.empty())
        return L"";
    return read_part(dir, L"settings.xml");
}
} // namespace

TEST(OdtToDocxShowRedlineChanges, FalseWritesRevisionViewInsDelOff)
{
    const std::wstring settings = docx_settings(L"show_redline_changes_false.odt");
    ASSERT_FALSE(settings.empty()) << "ODT -> DOCX conversion failed or word/settings.xml is missing";
    const size_t revisionView = settings.find(L"<w:revisionView ");
    ASSERT_NE(revisionView, std::wstring::npos);
    const size_t revisionViewEnd = settings.find(L"/>", revisionView);
    ASSERT_NE(revisionViewEnd, std::wstring::npos);
    // insDel and formatting off, as LibreOffice writes it: the document shows no change markup at all.
    const std::wstring element = settings.substr(revisionView, revisionViewEnd - revisionView);
    EXPECT_NE(element.find(L"w:insDel=\"0\""), std::wstring::npos);
    EXPECT_NE(element.find(L"w:formatting=\"0\""), std::wstring::npos);
    // CT_Settings order: w:revisionView comes before w:defaultTabStop and w:compat.
    const size_t compat = settings.find(L"<w:compat");
    ASSERT_NE(compat, std::wstring::npos);
    EXPECT_LT(revisionView, compat);
    const size_t defaultTabStop = settings.find(L"<w:defaultTabStop");
    if (defaultTabStop != std::wstring::npos)
        EXPECT_LT(revisionView, defaultTabStop);
}

TEST(OdtToDocxShowRedlineChanges, TrueWritesNoRevisionView)
{
    const std::wstring settings = docx_settings(L"show_redline_changes_true.odt");
    ASSERT_FALSE(settings.empty()) << "ODT -> DOCX conversion failed or word/settings.xml is missing";
    EXPECT_EQ(settings.find(L"w:revisionView"), std::wstring::npos);
}

TEST(DocxToOdtShowRedlineChanges, InsDelOffWritesViewSetting)
{
    const std::wstring settings = odt_settings(L"revision_view_ins_del_off.docx");
    ASSERT_FALSE(settings.empty()) << "DOCX -> ODT conversion failed or settings.xml is missing";
    ASSERT_TRUE(is_well_formed(settings));

    const size_t view = settings.find(L"config:name=\"ooo:view-settings\"");
    ASSERT_NE(view, std::wstring::npos) << "no ooo:view-settings";
    const size_t viewEnd = settings.find(L"</config:config-item-set>", view);
    ASSERT_NE(viewEnd, std::wstring::npos);
    const std::wstring viewSettings = settings.substr(view, viewEnd - view);

    // LibreOffice reads it at the top level of the view settings, not inside a Views entry.
    const size_t item = viewSettings.find(L"config:name=\"ShowRedlineChanges\"");
    ASSERT_NE(item, std::wstring::npos) << "no ShowRedlineChanges in ooo:view-settings";
    EXPECT_NE(viewSettings.find(L">false<", item), std::wstring::npos);
    const size_t views = viewSettings.find(L"config:name=\"Views\"");
    EXPECT_TRUE(views == std::wstring::npos || item < views);
}

TEST(DocxToOdtShowRedlineChanges, NoRevisionViewWritesNoSetting)
{
    const std::wstring settings = odt_settings(L"paragraph_spacing_no_compat_flag.docx");
    ASSERT_FALSE(settings.empty()) << "DOCX -> ODT conversion failed or settings.xml is missing";
    EXPECT_EQ(settings.find(L"ShowRedlineChanges"), std::wstring::npos);
}
