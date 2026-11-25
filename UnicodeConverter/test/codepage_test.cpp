/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// CUnicodeConverter::toUnicode with a numeric code page. ICU cannot open most
// of these code pages by number, and on Linux iconv does not know the name
// CP<n> for ISO-8859-x, KOI8, EUC, GB18030 or UTF-7/8/16/32, so the converter
// has to open them by name. Without that, the input is widened byte by byte
// and the text comes out wrong (UTF-16 files with a BOM in the TXT reader).

#include "gtest/gtest.h"

#include "../UnicodeConverter.h"

#include <string>
#include <vector>

namespace
{
	struct CodePageCase
	{
		const char* name;
		int codePage;
		std::string bytes;
		std::wstring expected;
	};

	std::string Bytes(std::initializer_list<unsigned char> list)
	{
		return std::string(list.begin(), list.end());
	}

	std::vector<CodePageCase> Cases()
	{
		return {
			// Unicode
			{"UTF-16LE", 1200, Bytes({'H', 0, 'i', 0}), L"Hi"},
			{"UTF-16BE", 1201, Bytes({0, 'H', 0, 'i'}), L"Hi"},
			{"UTF-16LE surrogate pair", 1200, Bytes({'a', 0, 0x3D, 0xD8, 0x00, 0xDE}), L"a\U0001F600"},
			{"UTF-16BE surrogate pair", 1201, Bytes({0, 'a', 0xD8, 0x3D, 0xDE, 0x00}), L"a\U0001F600"},
			{"UTF-32LE", 12000, Bytes({'H', 0, 0, 0, 'i', 0, 0, 0}), L"Hi"},
			{"UTF-32BE", 12001, Bytes({0, 0, 0, 'H', 0, 0, 0, 'i'}), L"Hi"},
			{"UTF-32LE outside the BMP", 12000, Bytes({'a', 0, 0, 0, 0x00, 0xF6, 0x01, 0x00}), L"a\U0001F600"},
			{"UTF-32BE outside the BMP", 12001, Bytes({0, 0, 0, 'a', 0x00, 0x01, 0xF6, 0x00}), L"a\U0001F600"},
			{"UTF-8", 65001, Bytes({0xC3, 0xA9, 0xE2, 0x82, 0xAC}), L"é€"},
			{"UTF-7", 65000, "+AOk-", L"é"},

			// ISO-8859
			{"ISO-8859-1", 28591, Bytes({0xE9}), L"é"},
			{"ISO-8859-2", 28592, Bytes({0xA1}), L"Ą"},
			{"ISO-8859-3", 28593, Bytes({0xA1}), L"Ħ"},
			{"ISO-8859-4", 28594, Bytes({0xA1}), L"Ą"},
			{"ISO-8859-5", 28595, Bytes({0xB4, 0xD0}), L"Да"},
			{"ISO-8859-6", 28596, Bytes({0xC7}), L"ا"},
			{"ISO-8859-7", 28597, Bytes({0xE1}), L"α"},
			{"ISO-8859-8", 28598, Bytes({0xE0}), L"א"},
			{"ISO-8859-9", 28599, Bytes({0xFD}), L"ı"},
			{"ISO-8859-13", 28603, Bytes({0xE0}), L"ą"},
			{"ISO-8859-14", 28604, Bytes({0xA1}), L"Ḃ"},
			{"ISO-8859-15", 28605, Bytes({0xA4}), L"€"},

			// DOS Arabic
			{"DOS-720", 720, Bytes({0x82}), L"é"},

			// KOI8
			{"KOI8-R", 20866, Bytes({0xE4, 0xC1}), L"Да"},
			{"KOI8-U", 21866, Bytes({0xE4, 0xC1}), L"Да"},

			// East Asian
			{"EUC-JP", 20932, Bytes({0xA4, 0xA2}), L"あ"},
			{"EUC-KR", 51949, Bytes({0xB0, 0xA1}), L"가"},
			{"GB18030", 54936, Bytes({0xD6, 0xD0}), L"中"},

			// Windows code pages that already worked, so the change must keep them
			{"windows-1251", 1251, Bytes({0xC4, 0xE0}), L"Да"},
			{"windows-1252", 1252, Bytes({0xE9}), L"é"},
			{"Shift_JIS", 932, Bytes({0x82, 0xA0}), L"あ"},
		};
	}
}

TEST(UnicodeConverterCodePage, DecodesByCodePageNumber)
{
#if defined(_WIN32) || defined(_WIN64)
	GTEST_SKIP() << "Windows decodes numeric code pages with MultiByteToWideChar";
#else
	for (const CodePageCase& c : Cases())
	{
		NSUnicodeConverter::CUnicodeConverter converter;
		const std::wstring result = converter.toUnicode(c.bytes.c_str(), (unsigned int)c.bytes.size(), c.codePage);
		EXPECT_EQ(c.expected, result) << c.name << " (code page " << c.codePage << ")";
	}
#endif
}

TEST(UnicodeConverterCodePage, EmptyInputGivesEmptyString)
{
	NSUnicodeConverter::CUnicodeConverter converter;
	EXPECT_TRUE(converter.toUnicode("", 0, 1200).empty());
	EXPECT_TRUE(converter.toUnicode("", 0, 28595).empty());
}

TEST(UnicodeConverterCodePage, FromUnicodeKeepsCharactersOutsideTheBmp)
{
	// U+1F600 takes two UTF-16 code units, so the output needs more room than one per character
	NSUnicodeConverter::CUnicodeConverter converter;
	EXPECT_EQ("a\xF0\x9F\x98\x80" "b", converter.fromUnicode(std::wstring(L"a\U0001F600" L"b"), "UTF-8"));
	EXPECT_EQ(Bytes({'a', 0, 0x3D, 0xD8, 0x00, 0xDE}), converter.fromUnicode(std::wstring(L"a\U0001F600"), "UTF-16LE"));
}

TEST(UnicodeConverterCodePage, NonAsciiBytesAreNotSignExtended)
{
	// A code page this converter does not know falls back to widening the
	// bytes. They must stay in 0..255 and not turn into U+FFFFFFxx.
	NSUnicodeConverter::CUnicodeConverter converter;
	const std::string bytes = Bytes({0xE9, 'a'});
	const std::wstring result = converter.toUnicode(bytes.c_str(), (unsigned int)bytes.size(), 99999);
	ASSERT_EQ(2u, result.size());
	EXPECT_EQ((wchar_t)0xE9, result[0]);
	EXPECT_EQ(L'a', result[1]);
}
