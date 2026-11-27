// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: documents without a part the converter looks things up in. A flat Word 2003 XML
// document has no package at all; a package may leave out its styles or theme part.

#include "gtest/gtest.h"

#include "common.h"
#include "DesktopEditor/common/Directory.h"

#include <string>
#include <vector>

#ifndef _WIN32
#include <fstream>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace
{
#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
	// Runs in the death-test child: limits time and address space, so a conversion that never ends
	// or keeps allocating fails the test instead of hanging the suite or exhausting the machine.
	void limit_child()
	{
		alarm(10);
		std::ifstream statm("/proc/self/statm");
		unsigned long long pages = 0;
		if (statm >> pages)
		{
			const unsigned long long room = 1024ULL * 1024 * 1024;
			const unsigned long long limit = pages * (unsigned long long)sysconf(_SC_PAGESIZE) + room;
			const struct rlimit rl = { (rlim_t)limit, (rlim_t)limit };
			setrlimit(RLIMIT_AS, &rl);
		}
	}
#endif

	// Converts the fixture once in a child process and expects it to end normally, so a crash
	// fails this test instead of the whole suite.
	void expect_conversion_ends(const std::wstring& fixture)
	{
#if GTEST_HAS_DEATH_TEST && !defined(_WIN32)
		ASSERT_EXIT(
			{
				limit_child();
				const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
				if (!dir.empty())
					NSDirectory::DeleteDirectory(dir);
				_exit(dir.empty() ? 1 : 0);
			},
			::testing::ExitedWithCode(0), "");
#else
		GTEST_SKIP() << "needs death tests";
#endif
	}

	// The fixture converts, and every listed text ends up in content.xml.
	void expect_converted_text(const std::wstring& fixture, const std::vector<std::wstring>& texts)
	{
		expect_conversion_ends(fixture);
		if (::testing::Test::HasFatalFailure() || ::testing::Test::IsSkipped())
			return;

		const std::wstring dir = convert_ooxml_to_odf_dir(fixture);
		ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";
		const std::wstring content = read_part(dir, L"content.xml");
		EXPECT_TRUE(is_well_formed(content));
		EXPECT_TRUE(is_well_formed(read_part(dir, L"styles.xml")));
		for (const std::wstring& text : texts)
			EXPECT_NE(content.find(text), std::wstring::npos) << "missing text";
	}
}

// A paragraph with a paragraph style.
TEST(DocxToOdtMissingParts, FlatParagraphStyle)
{
	expect_converted_text(L"ExampleFiles/flat_paragraph_style.xml", {L"Styled heading", L"Plain text"});
}

// A run with a theme font (w:asciiTheme); a flat document has no theme.
TEST(DocxToOdtMissingParts, FlatThemeFont)
{
	expect_converted_text(L"ExampleFiles/flat_theme_font.xml", {L"Theme font"});
}

// A run with a theme colour and no explicit colour value.
TEST(DocxToOdtMissingParts, FlatThemeColor)
{
	expect_converted_text(L"ExampleFiles/flat_theme_color.xml", {L"Theme colour"});
}

// A list level that uses a picture bullet (w:lvlPicBulletId).
TEST(DocxToOdtMissingParts, FlatPictureBullet)
{
	expect_converted_text(L"ExampleFiles/flat_picture_bullet.xml", {L"Picture bullet item"});
}

// A package without word/styles.xml whose paragraph names a style.
TEST(DocxToOdtMissingParts, PackageWithoutStylesPart)
{
	expect_converted_text(L"ExampleFiles/no_styles_part_paragraph_style.docx", {L"Styled heading", L"Plain text"});
}

// A package without a theme part whose run uses a theme colour.
TEST(DocxToOdtMissingParts, PackageWithoutThemePart)
{
	expect_converted_text(L"ExampleFiles/no_theme_part_theme_color.docx", {L"Theme colour"});
}
