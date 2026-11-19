/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// Drives the MorphData reader used for check boxes, option buttons and the
// other MS-OFORMS controls directly with a hand-packed buffer, and converts
// ExampleFiles/activex_groupname/groupname.docx, whose two option buttons share
// a compressed GroupName.
//
// MorphDataPropMask is a 64-bit mask and bit 32 selects fGroupName. Word
// stores the name compressed, so the character count has bit 31 set
// (0x80000006 for "Group1"). The name must be consumed whatever the top bit
// says, otherwise its bytes are taken for the TextProps that follow and the
// font name comes out of the wrong offset.

#include "../DocxFormat/Media/ActiveX.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace
{
	// MS-OFORMS 2.2.5.1 MorphDataPropMask: fSize is bit 8, fCaption bit 23,
	// fGroupName bit 32 (only reachable from a 64-bit value).
	const uint64_t kPropMask = (uint64_t(1) << 8) | (uint64_t(1) << 23) | (uint64_t(1) << 32);

	// Writes the little-endian fields of an MS-OFORMS 2.2.5 MorphData.
	struct MorphData
	{
		std::vector<unsigned char> bytes;

		void UInt8(unsigned char value) { bytes.push_back(value); }
		void UInt16(unsigned short value)
		{
			UInt8(value & 0xff);
			UInt8((value >> 8) & 0xff);
		}
		void UInt32(uint32_t value)
		{
			for (int i = 0; i < 4; ++i)
				UInt8((value >> (8 * i)) & 0xff);
		}
		void UInt64(uint64_t value)
		{
			for (int i = 0; i < 8; ++i)
				UInt8((value >> (8 * i)) & 0xff);
		}
		// CountOfCharsWithCompressionFlag, bit 31 set: one byte per character.
		void CountAndCompression(uint32_t count)
		{
			UInt32(0x80000000 | count);
		}
		// The characters of a string, padded to 4 as readString leaves them.
		void Bytes(const char* text, uint32_t count)
		{
			for (uint32_t i = 0; i < count; ++i)
				UInt8(text[i]);
			Align();
		}
		void Align()
		{
			while (bytes.size() % 4)
				UInt8(0);
		}
	};

	std::vector<unsigned char> BuildMorphData()
	{
		MorphData data;

		data.UInt8(0);               // MorphData.MinorVersion
		data.UInt8(0);               // MorphData.MajorVersion
		data.UInt16(0);              // MorphData.cbMorphData, patched with the size below
		data.UInt64(kPropMask);      // MorphData.PropMask

		// DataBlock, in mask order. Only fCaption comes before fGroupName; the
		// string sizes live here and the characters in the ExtraDataBlock.
		data.Align();                // the reader aligns before the string sizes
		data.CountAndCompression(7); // fCaption: 7 chars, compressed
		data.Align();                // the reader aligns before fGroupName
		data.CountAndCompression(6); // fGroupName: 6 chars, compressed

		// ExtraDataBlock: fSize first, then the strings in DataBlock order.
		data.UInt32(120);            // fSize.cx
		data.UInt32(45);             // fSize.cy
		data.Bytes("Option1", 7);    // fCaption characters
		data.Bytes("Group1", 6);     // fGroupName characters

		// StreamData: TextProps (MS-OFORMS 2.2.5.4). The font name can only be
		// read correctly when the group name above was consumed.
		data.UInt8(0);               // TextProps.MinorVersionText
		data.UInt8(0);               // TextProps.MajorVersionText
		data.UInt16(20);             // TextProps.cbTextProps, to the end of the record
		data.UInt32(1);              // TextProps.PropMask: fFontName
		data.CountAndCompression(5); // TextProps.fFontName: 5 chars, compressed
		data.Bytes("Arial", 5);      // TextProps.fFontName characters

		data.Align();

		// cbMorphData covers the whole structure and must not exceed the buffer.
		const size_t size = data.bytes.size();
		data.bytes[2] = size & 0xff;
		data.bytes[3] = (size >> 8) & 0xff;

		return data.bytes;
	}

	// ExampleFiles/activex_groupname/make_fixture.py authors this document: two
	// option buttons ({8BD21D50-...}) whose ActiveX binaries carry the same
	// compressed GroupName, "Group1".
	const wchar_t* const kGroupedDocument =
		L"ExampleFiles" FILE_SEPARATOR_STR L"activex_groupname" FILE_SEPARATOR_STR L"groupname.docx";

	std::wstring Part(const std::wstring& dir, const wchar_t* name)
	{
		return dir + FILE_SEPARATOR_STR + name;
	}

	std::vector<unsigned char> ReadBytes(const std::wstring& path)
	{
		BYTE* data = NULL;
		DWORD size = 0;
		std::vector<unsigned char> bytes;
		if (NSFile::CFileBinary::ReadAllBytes(path, &data, size) && data)
			bytes.assign(data, data + size);
		RELEASEARRAYOBJECTS(data);
		return bytes;
	}
}

TEST(ActiveXMorphData, ConsumesCompressedGroupName)
{
	std::vector<unsigned char> data = BuildMorphData();

	OOX::ActiveXObjectMorphData morph(SimpleTypes::Spreadsheet::objectRadio);
	morph.Parse(data.data(), static_cast<DWORD>(data.size()));

	// ExtraDataBlock fSize.
	ASSERT_TRUE(morph.m_oWidth.IsInit());
	EXPECT_EQ(*morph.m_oWidth, 120u);
	ASSERT_TRUE(morph.m_oHeight.IsInit());
	EXPECT_EQ(*morph.m_oHeight, 45u);

	// ExtraDataBlock fCaption.
	ASSERT_TRUE(morph.m_oCaption.IsInit());
	EXPECT_EQ(*morph.m_oCaption, std::wstring(L"Option1"));

	// StreamData TextProps: reached only after the group name was consumed.
	ASSERT_TRUE(morph.m_oFontName.IsInit())
		<< "TextProps started at the wrong offset: the group name was not consumed";
	EXPECT_EQ(*morph.m_oFontName, std::wstring(L"Arial"));
}
