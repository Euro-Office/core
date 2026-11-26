/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// Word 2003 XML (flat WordprocessingML) documents that leave out an element the
// flat reader used to take for granted: w:styles, w:body, or w:compat in w:docPr
// when o:DocumentProperties names the Word version. A document that has w:compat
// keeps its own compat options next to the compatibilityMode setting, and its own
// compatibilityMode if it has one.
//
// Each case reads the fixture with OOX::CDocxFlat and converts it to DOCX and ODT
// in a child process, so a crash fails the test instead of the whole suite.
#include "../DocxFormat/DocxFlat.h"
#include "../DocxFormat/Document.h"
#include "../DocxFormat/Styles.h"
#include "../DocxFormat/Settings/Settings.h"
#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"
#include "common.h"

#include <gtest/gtest.h>

#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace
{
	const std::wstring kFixtures = L"ExampleFiles/docxflat/";

	// Runs in the death-test child: reads the fixture, runs check() on the result and
	// converts it to DOCX and ODT. With requireOutput, both conversions must succeed;
	// without it they only must not crash. Returns the child's exit code.
	int read_and_convert(const std::wstring& file, const std::wstring& work, bool (*check)(const OOX::CDocxFlat&),
						 bool requireOutput)
	{
		bool ok = true;
		{
			OOX::CDocxFlat flat{OOX::CPath(file)};
			ok = check(flat);
		}
		const int formats[] = {AVS_OFFICESTUDIO_FILE_DOCUMENT_DOCX, AVS_OFFICESTUDIO_FILE_DOCUMENT_ODT};
		const std::wstring extensions[] = {L"docx", L"odt"};
		for (int i = 0; i < 2; ++i)
		{
			const std::wstring to = work + FILE_SEPARATOR_STR + L"out." + extensions[i];
			const int code = ConvertFile(CreateParamsFile(file, to, work, AVS_OFFICESTUDIO_FILE_DOCUMENT_DOCX_FLAT, formats[i]));
			if (requireOutput && (code != 0 || !NSFile::CFileBinary::Exists(to)))
				ok = false;
		}
		return ok ? 0 : 2;
	}

	void expect_flat_document(const std::wstring& name, bool (*check)(const OOX::CDocxFlat&), bool requireOutput)
	{
#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
		const std::wstring file = kFixtures + name;
		ASSERT_TRUE(NSFile::CFileBinary::Exists(file)) << "missing fixture";
		const std::wstring work = GetWorkDir();
		ASSERT_EXIT(
			{
				alarm(60);
				_exit(read_and_convert(file, work, check, requireOutput));
			},
			::testing::ExitedWithCode(0), "");
#else
		GTEST_SKIP() << "needs death tests";
#endif
	}

	// The Word version from o:DocumentProperties ends up as the compatibilityMode setting.
	bool has_compatibility_mode(const OOX::CDocxFlat& flat)
	{
		if (!flat.m_pSettings.IsInit() || !flat.m_pSettings->m_oCompat.IsInit())
			return false;
		for (const OOX::Settings::CCompatSetting* setting : flat.m_pSettings->m_oCompat->m_arrCompatSettings)
		{
			if (setting && setting->m_sName.IsInit() && *setting->m_sName == L"compatibilityMode" &&
				setting->m_sVal.IsInit() && *setting->m_sVal == L"11.5606")
				return true;
		}
		return false;
	}
}

TEST(DocxFlatMissingElements, NoStyles)
{
	expect_flat_document(L"no_styles.xml", [](const OOX::CDocxFlat& flat) {
		return flat.m_pStyles.IsInit() && flat.m_pStyles->m_oDocDefaults.IsInit() && flat.m_pDocument.IsInit();
	}, true);
}

TEST(DocxFlatMissingElements, NoBody)
{
	expect_flat_document(L"no_body.xml", [](const OOX::CDocxFlat& flat) {
		return !flat.m_pDocument.IsInit() && flat.m_pStyles.IsInit();
	}, false);
}

TEST(DocxFlatMissingElements, VersionWithoutCompat)
{
	expect_flat_document(L"version_without_compat.xml", has_compatibility_mode, true);
}

TEST(DocxFlatMissingElements, VersionWithoutSettings)
{
	expect_flat_document(L"version_without_settings.xml", has_compatibility_mode, true);
}

TEST(DocxFlatMissingElements, VersionWithCompatKeepsOptions)
{
	expect_flat_document(L"version_with_compat.xml", [](const OOX::CDocxFlat& flat) {
		// w:breakWrappedTables in the file is read as doNotBreakWrappedTables="false".
		return has_compatibility_mode(flat) && flat.m_pSettings->m_oCompat->m_oDoNotBreakWrappedTables.IsInit() &&
			   !flat.m_pSettings->m_oCompat->m_oDoNotBreakWrappedTables->m_oVal.ToBool();
	}, true);
}

TEST(DocxFlatMissingElements, VersionWithOwnCompatibilityMode)
{
	expect_flat_document(L"version_with_compatibility_mode.xml", [](const OOX::CDocxFlat& flat) {
		// The document's own compatibilityMode wins over the Word version; there must be exactly one.
		if (!flat.m_pSettings.IsInit() || !flat.m_pSettings->m_oCompat.IsInit())
			return false;
		int count = 0;
		bool own = false;
		for (const OOX::Settings::CCompatSetting* setting : flat.m_pSettings->m_oCompat->m_arrCompatSettings)
		{
			if (setting && setting->m_sName.IsInit() && *setting->m_sName == L"compatibilityMode")
			{
				++count;
				own = setting->m_sVal.IsInit() && *setting->m_sVal == L"15";
			}
		}
		return count == 1 && own;
	}, true);
}
