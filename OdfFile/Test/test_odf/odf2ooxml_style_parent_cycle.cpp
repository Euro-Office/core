// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX and ODT -> DOCX: styles whose style:parent-style-name chain leads back to itself
// (A -> B -> A). The conversion must end, keep the content and write no cycle into w:basedOn.

#include "gtest/gtest.h"

#include "common.h"
#include "DesktopEditor/common/Directory.h"

#include <map>
#include <set>
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
	// Same limits as in docx2odt_style_parent_cycle.cpp.
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
				const std::wstring dir = convert_odf_to_ooxml_dir(fixture);
				if (!dir.empty())
					NSDirectory::DeleteDirectory(dir);
				_exit(dir.empty() ? 1 : 0);
			},
			::testing::ExitedWithCode(0), "");
#else
		GTEST_SKIP() << "needs death tests";
#endif
	}

	// The value of attribute `name` inside `element`, or "".
	std::wstring attribute(const std::wstring& element, const std::wstring& name)
	{
		const std::wstring key = name + L"=\"";
		const size_t at = element.find(key);
		if (at == std::wstring::npos)
			return L"";
		const size_t begin = at + key.size();
		const size_t end = element.find(L'"', begin);
		return end == std::wstring::npos ? L"" : element.substr(begin, end - begin);
	}

	// True if some w:basedOn chain of word/styles.xml comes back to a style it has passed.
	bool has_based_on_cycle(const std::wstring& styles)
	{
		std::map<std::wstring, std::wstring> based_on;
		for (size_t at = styles.find(L"<w:style "); at != std::wstring::npos; at = styles.find(L"<w:style ", at + 1))
		{
			const size_t end = styles.find(L"</w:style>", at);
			const std::wstring style = styles.substr(at, end == std::wstring::npos ? std::wstring::npos : end - at);
			const size_t link = style.find(L"<w:basedOn ");
			if (link != std::wstring::npos)
				based_on[attribute(style, L"w:styleId")] = attribute(style.substr(link), L"w:val");
		}
		for (const auto& start : based_on)
		{
			std::set<std::wstring> seen;
			for (std::wstring id = start.first; based_on.count(id); id = based_on[id])
			{
				if (!seen.insert(id).second)
					return true;
			}
		}
		return false;
	}
}

// Cell styles: A and B are each other's parent, C -> D -> E -> C, and the automatic style ce1 has A as parent.
TEST(OdfToOoxmlStyleParentCycle, CellStyles)
{
	expect_conversion_ends(L"ExampleFiles/style_parent_cycle_cell.ods");
	if (HasFatalFailure() || IsSkipped())
		return;

	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/style_parent_cycle_cell.ods");
	ASSERT_FALSE(dir.empty()) << "ODS -> XLSX conversion failed";
	const std::wstring sheet = read_part(dir, L"xl/worksheets/sheet1.xml");
	EXPECT_TRUE(is_well_formed(sheet));
	const std::wstring styles = read_part(dir, L"xl/styles.xml");
	EXPECT_TRUE(is_well_formed(styles));
	// The cycle styles A and C are bold: their own properties are kept.
	EXPECT_NE(styles.find(L"<b val=\"1\"/>"), std::wstring::npos) << styles;
	EXPECT_NE(sheet.find(L"<v>1.5</v>"), std::wstring::npos) << sheet;
	EXPECT_NE(sheet.find(L"<v>2.5</v>"), std::wstring::npos) << sheet;
	EXPECT_NE(sheet.find(L"<v>3.5</v>"), std::wstring::npos) << sheet;
	NSDirectory::DeleteDirectory(dir);
}

// Paragraph styles P1 and P2 are each other's parent, and so are the text styles T1 and T2.
TEST(OdfToOoxmlStyleParentCycle, ParagraphAndTextStyles)
{
	expect_conversion_ends(L"ExampleFiles/style_parent_cycle_paragraph.odt");
	if (HasFatalFailure() || IsSkipped())
		return;

	const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/style_parent_cycle_paragraph.odt");
	ASSERT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
	const std::wstring document = read_part(dir, L"word/document.xml");
	const std::wstring styles = read_part(dir, L"word/styles.xml");
	EXPECT_TRUE(is_well_formed(document));
	EXPECT_TRUE(is_well_formed(styles));
	EXPECT_NE(document.find(L"Loop start"), std::wstring::npos);
	EXPECT_NE(document.find(L"Loop end"), std::wstring::npos);
	EXPECT_NE(document.find(L"in a span"), std::wstring::npos);
	EXPECT_NE(styles.find(L"<w:name w:val=\"P1\"/>"), std::wstring::npos) << styles;
	EXPECT_NE(styles.find(L"<w:name w:val=\"P2\"/>"), std::wstring::npos) << styles;
	EXPECT_FALSE(has_based_on_cycle(styles)) << styles;
	NSDirectory::DeleteDirectory(dir);
}
