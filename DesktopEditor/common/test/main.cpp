#include "gtest/gtest.h"

#include "../Directory.h"
#include "../File.h"

#include <algorithm>
#include <cstdio>
#ifndef _WIN32
#include <ftw.h>
#include <unistd.h>
#endif

// Builds a scratch tree next to the test binary. Nothing is committed to the
// repository: SetUp() creates
//
//   <root>/real.bin          regular file
//   <root>/sub/nested.bin    regular file
//
// and each test adds on top of that the symlink it needs.
//
class CDirectoryTest : public testing::Test
{
protected:
	std::wstring m_root;

	static bool Symlink(const std::string& sTarget, const std::wstring& wsLink)
	{
#ifdef _WIN32
		return false;
#else
		return 0 == symlink(sTarget.c_str(), U_TO_UTF8(wsLink).c_str());
#endif
	}

#ifndef _WIN32
	static int RemoveEntry(const char* pPath, const struct stat*, int, struct FTW*)
	{
		return remove(pPath);
	}
#endif

	// Cleanup deliberately does not go through NSDirectory: DeleteDirectory()
	// enumerates with GetFiles()/GetDirectories(), the very functions under test.
	// They drop symlinks today, so the tree would survive and leak state between
	// tests — which silently turned every case into a skip on the second run.
	void RemoveTree()
	{
#ifdef _WIN32
		NSDirectory::DeleteDirectory(m_root);
#else
		nftw(U_TO_UTF8(m_root).c_str(), RemoveEntry, 16, FTW_DEPTH | FTW_PHYS);
#endif
	}

	static bool Contains(const std::vector<std::wstring>& oArray, const std::wstring& wsName)
	{
		return std::any_of(oArray.begin(), oArray.end(),
			[&wsName](const std::wstring& ws) { return ws.size() >= wsName.size() &&
				0 == ws.compare(ws.size() - wsName.size(), wsName.size(), wsName); });
	}

	void SetUp() override
	{
		m_root = NSFile::GetProcessDirectory() + L"/directory_test_tree";
		RemoveTree();
		ASSERT_TRUE(NSDirectory::CreateDirectory(m_root));

		ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(m_root + L"/real.bin", L"x"));
		ASSERT_TRUE(NSDirectory::CreateDirectory(m_root + L"/sub"));
		ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(m_root + L"/sub/nested.bin", L"x"));
	}

	void TearDown() override
	{
		RemoveTree();
	}
};

// --- #128: entries that readdir reports as DT_LNK were dropped ---

TEST_F(CDirectoryTest, symlinked_file_is_listed)
{
	if (!Symlink("real.bin", m_root + L"/link.bin"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles = NSDirectory::GetFiles(m_root, false);

	EXPECT_TRUE(Contains(oFiles, L"real.bin"));
	EXPECT_TRUE(Contains(oFiles, L"link.bin"));
}

// Symlinked directories stay out on purpose. GetDirectories() feeds
// DeleteDirectory(), which recurses over its result: reporting a symlinked
// directory there would make DeleteDirectory() delete the link target's contents
// instead of just removing the link. This test pins that decision — if it ever
// fails, read the comment in Directory.cpp before "fixing" it.
TEST_F(CDirectoryTest, symlinked_directory_is_not_listed)
{
	if (!Symlink("sub", m_root + L"/sublink"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oDirs = NSDirectory::GetDirectories(m_root);

	EXPECT_TRUE(Contains(oDirs, L"sub"));
	EXPECT_FALSE(Contains(oDirs, L"sublink"));
}

// --- guards on the new behaviour (these pass before the fix as well) ---

// A symlinked directory passed in directly is scanned normally: opendir()
// follows the link. Only symlinked *subdirectories* discovered during recursion
// are skipped — so the common layout, a font directory that is itself a symlink,
// keeps working.
TEST_F(CDirectoryTest, symlinked_root_directory_is_scanned)
{
	if (!Symlink("sub", m_root + L"/sublink"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles = NSDirectory::GetFiles(m_root + L"/sublink", false);

	EXPECT_TRUE(Contains(oFiles, L"nested.bin"));
}

TEST_F(CDirectoryTest, dangling_symlink_is_skipped)
{
	if (!Symlink("missing.bin", m_root + L"/broken.bin"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles = NSDirectory::GetFiles(m_root, false);

	EXPECT_TRUE(Contains(oFiles, L"real.bin"));
	EXPECT_FALSE(Contains(oFiles, L"broken.bin"));
}

TEST_F(CDirectoryTest, self_referencing_symlink_does_not_recurse_forever)
{
	if (!Symlink(".", m_root + L"/loop"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles = NSDirectory::GetFiles(m_root, true);

	// The loop is never entered because a symlinked directory is not classified
	// as a directory, so the recursive walk has nothing to descend into. If that
	// ever changes, this scan would descend root/loop/loop/... until the path
	// exceeds PATH_MAX, reporting real.bin once per level — hence the bound.
	EXPECT_TRUE(Contains(oFiles, L"real.bin"));
	EXPECT_LT(oFiles.size(), (size_t)10);
}

// Following link subdirectories is only implemented for Linux now
// TODO: update the guard when implemented for MacOS
#ifdef __linux__

TEST_F(CDirectoryTest, follow_symlinks_lists_files_in_symlinked_subdirectory)
{
	if (!Symlink("sub", m_root + L"/sublink"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/real.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/sub/nested.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/sublink/nested.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)3);
}

// The target may live outside the scanned tree: the link decides what is
// reachable, not where the target is.
TEST_F(CDirectoryTest, follow_symlinks_reaches_directory_outside_the_root)
{
	ASSERT_TRUE(NSDirectory::CreateDirectory(m_root + L"/outside"));
	ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(m_root + L"/outside/linked.bin", L"x"));
	ASSERT_TRUE(NSDirectory::CreateDirectory(m_root + L"/scan"));
	ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(m_root + L"/scan/plain.bin", L"x"));
	if (!Symlink("../outside", m_root + L"/scan/link"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root + L"/scan", oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/scan/plain.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/scan/link/linked.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)2);
}

// Links are followed through several levels: a symlinked directory found inside
// another symlinked directory is walked too.
TEST_F(CDirectoryTest, follow_symlinks_follows_nested_symlinked_directories)
{
	ASSERT_TRUE(NSDirectory::CreateDirectory(m_root + L"/deep"));
	ASSERT_TRUE(NSFile::CFileBinary::SaveToFile(m_root + L"/deep/deep.bin", L"x"));
	if (!Symlink("../deep", m_root + L"/sub/deeplink") || !Symlink("sub", m_root + L"/sublink"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/sublink/deeplink/deep.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/sub/deeplink/deep.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/deep/deep.bin"));
}

// Self-reference ("loop -> ."): the link resolves to the directory being scanned,
// identified by (st_dev, st_ino), and is skipped — every file is listed once.
TEST_F(CDirectoryTest, follow_symlinks_skips_link_to_the_directory_itself)
{
	if (!Symlink(".", m_root + L"/loop"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/real.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/sub/nested.bin"));
	EXPECT_FALSE(Contains(oFiles, L"/loop/real.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)2);
}

// The self check compares identities, not spellings: "../sub" from inside sub
// is the same directory as ".".
TEST_F(CDirectoryTest, follow_symlinks_skips_self_link_spelled_through_parent)
{
	if (!Symlink("../sub", m_root + L"/sub/again"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/sub/nested.bin"));
	EXPECT_FALSE(Contains(oFiles, L"/again/nested.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)2);
}

TEST_F(CDirectoryTest, follow_symlinks_skips_dangling_directory_link)
{
	if (!Symlink("missing_dir", m_root + L"/broken"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/real.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/sub/nested.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)2);
}

// Hidden directories are not walked, and a symlink named like one is no
// exception.
TEST_F(CDirectoryTest, follow_symlinks_skips_hidden_symlinked_directory)
{
	if (!Symlink("sub", m_root + L"/.hidden"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_FALSE(Contains(oFiles, L"/.hidden/nested.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)2);
}

// Symlinked files are listed by both variants; following directories must not
// change that.
TEST_F(CDirectoryTest, follow_symlinks_still_lists_symlinked_file)
{
	if (!Symlink("real.bin", m_root + L"/link.bin"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, true);

	EXPECT_TRUE(Contains(oFiles, L"/real.bin"));
	EXPECT_TRUE(Contains(oFiles, L"/link.bin"));
}

#endif

// Following links is part of the recursive walk only: without bIsRecursion
// nothing below the root is listed, linked or not.
TEST_F(CDirectoryTest, follow_symlinks_without_recursion_lists_only_the_root)
{
	if (!Symlink("sub", m_root + L"/sublink"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles;
	NSDirectory::GetFilesFollowSymlinks(m_root, oFiles, false);

	EXPECT_TRUE(Contains(oFiles, L"/real.bin"));
	EXPECT_FALSE(Contains(oFiles, L"nested.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)1);
}

// The default listing must stay as it was: same tree, GetFiles() does not walk
// the link.
TEST_F(CDirectoryTest, get_files_still_does_not_follow_symlinked_subdirectory)
{
	if (!Symlink("sub", m_root + L"/sublink"))
		GTEST_SKIP() << "symlinks unavailable on this platform";

	std::vector<std::wstring> oFiles = NSDirectory::GetFiles(m_root, true);

	EXPECT_TRUE(Contains(oFiles, L"/sub/nested.bin"));
	EXPECT_FALSE(Contains(oFiles, L"/sublink/nested.bin"));
	EXPECT_EQ(oFiles.size(), (size_t)2);
}
