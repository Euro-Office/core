/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// A VML shape may omit width/height and give its size only through
// margin-right/margin-bottom (CDrawingConverter::GetDrawingMainProps falls back
// to those). The shape must not end up with a zero extent.
//
// DOCX -> ODT is used because the ODT writer turns the converted VML shape into
// a draw:rect whose svg:width/svg:height show the size that was computed.

#include "common.h"

#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <string>

namespace
{
	std::string ReadText(const std::wstring& path)
	{
		std::string text;
		NSFile::CFileBinary::ReadAllTextUtf8A(path, text);
		return text;
	}

	std::string Attribute(const std::string& tag, const std::string& name)
	{
		std::string open = " " + name + "=\"";
		size_t begin = tag.find(open);
		if (begin == std::string::npos)
			return "";
		begin += open.size();
		return tag.substr(begin, tag.find('"', begin) - begin);
	}

	TEST(VmlShapeSize, MarginFallback)
	{
		std::wstring workDir = GetWorkDir();
		std::wstring odt = workDir + FILE_SEPARATOR_STR + L"vml_margin.odt";
		std::string error;
		ASSERT_TRUE(ConvertTo(L"ExampleFiles" FILE_SEPARATOR_STR L"docx2odt"
							  FILE_SEPARATOR_STR L"vml_margin.docx", odt, workDir, error)) << error;

		std::wstring unpacked = workDir + FILE_SEPARATOR_STR + L"unpacked";
		ASSERT_TRUE(Unpack(odt, unpacked, error)) << error;

		std::string content = ReadText(unpacked + FILE_SEPARATOR_STR + L"content.xml");
		size_t begin = content.find("<draw:rect");
		ASSERT_NE(std::string::npos, begin) << content;
		std::string tag = content.substr(begin, content.find('>', begin) - begin);

		// margin-right: 100pt - margin-left: 10pt = 90pt = 3.175cm
		EXPECT_EQ("3.175cm", Attribute(tag, "svg:width")) << tag;
		// margin-bottom: 100pt - margin-top: 10pt = 90pt = 3.175cm
		EXPECT_EQ("3.175cm", Attribute(tag, "svg:height")) << tag;
	}
}
