/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// Reading text files with TxtFile: a UTF-16 or UTF-8 byte order mark decides
// the encoding, whatever encoding was asked for. The files are written by the
// tests into the temporary directory.

#include "gtest/gtest.h"

#include "../Source/TxtFormat/TxtFile.h"
#include "../../UnicodeConverter/UnicodeConverter_Encodings.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
	std::string Bytes(std::initializer_list<unsigned char> list)
	{
		return std::string(list.begin(), list.end());
	}

	// Writes the bytes to a temporary file that is removed with the object
	class TempFile
	{
	public:
		explicit TempFile(const std::string& bytes)
		{
			static int counter = 0;
			m_path = std::filesystem::temp_directory_path() / ("eo_txtfile_test_" + std::to_string(counter++) + ".txt");
			std::ofstream file(m_path, std::ios::binary);
			file.write(bytes.data(), (std::streamsize)bytes.size());
		}
		~TempFile()
		{
			std::error_code ec;
			std::filesystem::remove(m_path, ec);
		}
		std::wstring Path() const { return m_path.wstring(); }

	private:
		std::filesystem::path m_path;
	};

	int Utf8Index()
	{
		for (const NSUnicodeConverter::EncodindId& encoding : NSUnicodeConverter::Encodings)
		{
			if (65001 == encoding.WindowsCodePage)
				return encoding.Index;
		}
		return -1;
	}

	std::vector<std::string> ReadUtf8(const std::string& bytes, int encoding)
	{
		TempFile file(bytes);
		TxtFile txt(file.Path());
		return txt.readUtf8Lines(encoding);
	}

	std::vector<std::wstring> ReadUnicode(const std::string& bytes, int encoding)
	{
		TempFile file(bytes);
		TxtFile txt(file.Path());
		return txt.readUnicodeLines(encoding);
	}

	typedef std::vector<std::string> Lines;
	typedef std::vector<std::wstring> WLines;
}

TEST(TxtFileBom, Utf16LittleEndian)
{
	const std::string bytes = Bytes({0xFF, 0xFE, 'H', 0, 'i', 0, '\n', 0, 'W', 0, 'o', 0, 'r', 0, 'l', 0, 'd', 0, '\n', 0});
	EXPECT_EQ((Lines{"Hi", "World"}), ReadUtf8(bytes, Utf8Index()));
	EXPECT_EQ((WLines{L"Hi", L"World"}), ReadUnicode(bytes, Utf8Index()));
}

TEST(TxtFileBom, Utf16BigEndian)
{
	const std::string bytes = Bytes({0xFE, 0xFF, 0, 'H', 0, 'i', 0, '\n', 0, 'W', 0, 'o', 0, 'r', 0, 'l', 0, 'd', 0, '\n'});
	EXPECT_EQ((Lines{"Hi", "World"}), ReadUtf8(bytes, Utf8Index()));
	EXPECT_EQ((WLines{L"Hi", L"World"}), ReadUnicode(bytes, Utf8Index()));
}

TEST(TxtFileBom, Utf16WithAccentAndCharacterOutsideTheBmp)
{
	// "é" and U+1F600 (a surrogate pair)
	const std::string little = Bytes({0xFF, 0xFE, 0xE9, 0, '\n', 0, 0x3D, 0xD8, 0x00, 0xDE});
	const std::string big = Bytes({0xFE, 0xFF, 0, 0xE9, 0, '\n', 0xD8, 0x3D, 0xDE, 0x00});
	const Lines expected{"\xC3\xA9", "\xF0\x9F\x98\x80"};
	EXPECT_EQ(expected, ReadUtf8(little, Utf8Index()));
	EXPECT_EQ(expected, ReadUtf8(big, Utf8Index()));
}

TEST(TxtFileBom, TheBomWinsOverTheRequestedEncoding)
{
	const std::string bytes = Bytes({0xFF, 0xFE, 'H', 0, 'i', 0});
	for (int encoding : {-1, 0, Utf8Index(), 1000})
		EXPECT_EQ((Lines{"Hi"}), ReadUtf8(bytes, encoding)) << "encoding " << encoding;
}

TEST(TxtFileBom, OnlyTheBomGivesNoLines)
{
	EXPECT_TRUE(ReadUtf8(Bytes({0xFF, 0xFE}), Utf8Index()).empty());
	EXPECT_TRUE(ReadUtf8(Bytes({0xFE, 0xFF}), Utf8Index()).empty());
	EXPECT_TRUE(ReadUtf8(Bytes({0xEF, 0xBB, 0xBF}), Utf8Index()).empty());
}

TEST(TxtFileBom, Utf8WithBom)
{
	const std::string bytes = Bytes({0xEF, 0xBB, 0xBF, 'c', 'a', 'f', 0xC3, 0xA9, '\n', 'x'});
	EXPECT_EQ((Lines{"caf\xC3\xA9", "x"}), ReadUtf8(bytes, Utf8Index()));
}

TEST(TxtFileNoBom, Utf8WithCharacterOutsideTheBmp)
{
	// U+1F600 takes two UTF-16 code units, so it needs more room than one per character
	const Lines expected{"a\xF0\x9F\x98\x80" "b", "\xF0\x9F\x98\x80"};
	EXPECT_EQ(expected, ReadUtf8("a\xF0\x9F\x98\x80" "b\n\xF0\x9F\x98\x80\n", Utf8Index()));
}

TEST(TxtFileNoBom, Utf8ByIndexAndByDefault)
{
	const std::string bytes = "caf\xC3\xA9 \xE2\x82\xAC\n";
	const Lines expected{"caf\xC3\xA9 \xE2\x82\xAC"};
	EXPECT_EQ(expected, ReadUtf8(bytes, Utf8Index()));
	// an encoding that is not in the table falls back to UTF-8 by code page number
	EXPECT_EQ(expected, ReadUtf8(bytes, -1));
}
