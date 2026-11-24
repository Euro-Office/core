// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// DOCX -> ODT: styles whose w:basedOn chain leads back to itself (A based on B, B based on A).

#include "gtest/gtest.h"

#include "common.h"
#include "DesktopEditor/common/Directory.h"

#include <string>

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
	// The address-space limit needs /proc (Linux); elsewhere only the timer applies, so keep it short.
	// A child stopped this way leaves its temporary folders behind; that only happens when the test fails.
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

	// Converts the fixture once in a limited child process and expects it to end normally.
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
}

// Paragraph styles "First" and "Second" are based on each other; "Itself" is based on itself.
TEST(DocxToOdtStyleParentCycle, ParagraphStyles)
{
	expect_conversion_ends(L"ExampleFiles/style_parent_cycle_paragraph.docx");
	if (HasFatalFailure() || IsSkipped())
		return;

	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/style_parent_cycle_paragraph.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";
	const std::wstring content = read_part(dir, L"content.xml");
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_TRUE(is_well_formed(read_part(dir, L"styles.xml")));
	EXPECT_NE(content.find(L"Loop start"), std::wstring::npos);
	EXPECT_NE(content.find(L"Loop end"), std::wstring::npos);
	EXPECT_NE(content.find(L"Own parent"), std::wstring::npos);
}

// Character styles "Strong1" and "Strong2" are based on each other.
TEST(DocxToOdtStyleParentCycle, CharacterStyles)
{
	expect_conversion_ends(L"ExampleFiles/style_parent_cycle_character.docx");
	if (HasFatalFailure() || IsSkipped())
		return;

	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/style_parent_cycle_character.docx");
	ASSERT_FALSE(dir.empty()) << "DOCX -> ODT conversion failed";
	const std::wstring content = read_part(dir, L"content.xml");
	EXPECT_TRUE(is_well_formed(content));
	EXPECT_TRUE(is_well_formed(read_part(dir, L"styles.xml")));
	EXPECT_NE(content.find(L"Plain"), std::wstring::npos);
	EXPECT_NE(content.find(L"Styled"), std::wstring::npos);
}
