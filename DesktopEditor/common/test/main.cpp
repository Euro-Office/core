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
