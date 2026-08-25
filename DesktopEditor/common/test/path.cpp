#include "gtest/gtest.h"

#include "../Path.h"

// ShortenPath resolves "." and ".." lexically, without touching the filesystem.
// Its result is used as a containment check by every one of its callers: the
// ZipSlip guard in OfficeUtils/src/ZipUtilsCP.cpp, the "../" rejection in
// EpubFile/src/CEpubFile.cpp, and several starts_with(root) checks in HtmlFile2,
// OFDFile and the SVG image loader. It therefore decides whether a path taken
// from a document is accepted, which is why it is worth pinning precisely.

TEST(ShortenPath, keeps_a_plain_relative_path)
{
	EXPECT_EQ(L"a/b/c", NSSystemPath::ShortenPath(L"a/b/c"));
}

TEST(ShortenPath, resolves_a_parent_reference_in_the_middle)
{
	EXPECT_EQ(L"b", NSSystemPath::ShortenPath(L"a/../b"));
}

// --- defects reproduced below ---

TEST(ShortenPath, resolves_a_trailing_parent_reference)
{
	EXPECT_EQ(L"", NSSystemPath::ShortenPath(L"a/.."));
}

TEST(ShortenPath, keeps_consecutive_parent_references)
{
	EXPECT_EQ(L"../..", NSSystemPath::ShortenPath(L"../.."));
}

TEST(ShortenPath, does_not_crash_on_bare_parent_when_removing_external_path)
{
	EXPECT_EQ(L"", NSSystemPath::ShortenPath(L"..", true));
}

// --- guards: behaviour that must not change (these pass before the fix too) ---

TEST(ShortenPath, keeps_a_leading_parent_chain)
{
	EXPECT_EQ(L"../../etc/passwd", NSSystemPath::ShortenPath(L"../../etc/passwd"));
}

TEST(ShortenPath, keeps_an_absolute_path)
{
	EXPECT_EQ(L"/tmp/evil.txt", NSSystemPath::ShortenPath(L"/tmp/evil.txt"));
}

TEST(ShortenPath, returns_empty_for_empty_input)
{
	EXPECT_EQ(L"", NSSystemPath::ShortenPath(L""));
}

TEST(ShortenPath, drops_current_directory_segments)
{
	EXPECT_EQ(L"a/b", NSSystemPath::ShortenPath(L"./a/./b"));
}

// Archive entries and HTML sources routinely use backslashes; the callers rely on
// them being treated as separators rather than as part of a name.
TEST(ShortenPath, treats_a_backslash_as_a_separator)
{
	EXPECT_EQ(L"b", NSSystemPath::ShortenPath(L"a\\..\\b"));
}

TEST(ShortenPath, collapses_repeated_and_trailing_separators)
{
	EXPECT_EQ(L"a/b", NSSystemPath::ShortenPath(L"a//b"));
	EXPECT_EQ(L"a/b", NSSystemPath::ShortenPath(L"a/b/"));
}

TEST(ShortenPath, drops_the_external_prefix_when_asked)
{
	EXPECT_EQ(L"a", NSSystemPath::ShortenPath(L"../a", true));
}
