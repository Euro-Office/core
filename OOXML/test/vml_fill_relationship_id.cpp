/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// A VML fill names its picture with r:id (a picture page background is stored as
// <w:background><v:background><v:fill r:id="..." type="frame"/>; LibreOffice
// writes it this way, and so does x2t when it saves one). The drawing converter
// must resolve that id, or the picture is dropped when the document is opened
// (docx -> Editor.bin) and the next save writes the background without it.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
	std::string ReadText(const std::wstring& path)
	{
		std::string text;
		NSFile::CFileBinary::ReadAllTextUtf8A(path, text);
		return text;
	}

	// The tag of text that starts at pos, up to and including its '>', or "" if there is none.
	std::string TagAt(const std::string& text, size_t pos)
	{
		size_t end = std::string::npos == pos ? pos : text.find('>', pos);
		return std::string::npos == end ? std::string() : text.substr(pos, end + 1 - pos);
	}

	// The value of the attribute name in tag, or "" if the tag has none.
	std::string AttributeValue(const std::string& tag, const std::string& name)
	{
		std::string key = " " + name + "=\"";
		size_t begin = tag.find(key);
		if (std::string::npos == begin)
			return std::string();
		begin += key.size();
		size_t end = tag.find('"', begin);
		return std::string::npos == end ? std::string() : tag.substr(begin, end - begin);
	}

	// The first tag of text that starts with open and has the attribute name = value, or "".
	std::string FindTag(const std::string& text, const std::string& open, const std::string& name, const std::string& value)
	{
		for (size_t pos = text.find(open); std::string::npos != pos; pos = text.find(open, pos + 1))
		{
			std::string tag = TagAt(text, pos);
			if (AttributeValue(tag, name) == value)
				return tag;
		}
		return std::string();
	}

	class VmlFillRelationshipId : public ::testing::Test
	{
	public:
		static void SetUpTestSuite()
		{
			workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"docx2bin" FILE_SEPARATOR_STR
								  L"page_background_picture.docx";
			std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
			std::wstring result = workDir + FILE_SEPARATOR_STR + L"result.docx";

			// One x2t temp dir per step, so the save step cannot pick up parts the open step unpacked.
			std::wstring openTempDir = workDir + FILE_SEPARATOR_STR + L"open";
			std::wstring saveTempDir = workDir + FILE_SEPARATOR_STR + L"save";
			NSDirectory::CreateDirectory(openTempDir);
			NSDirectory::CreateDirectory(saveTempDir);

			if (!NSFile::CFileBinary::Exists(source))
			{
				error = "missing input " + U_TO_UTF8(source);
				return;
			}
			int code = ConvertFile(CreateParamsFile(source, bin, openTempDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_WORD));
			if (code != 0)
			{
				error = "docx -> Editor.bin: x2t returned " + std::to_string(code);
				return;
			}
			mediaDir = workDir + FILE_SEPARATOR_STR + L"media";
			code = ConvertFile(CreateParamsFile(bin, result, saveTempDir, AVS_OFFICESTUDIO_FILE_CANVAS_WORD,
												AVS_OFFICESTUDIO_FILE_DOCUMENT_DOCX));
			if (code != 0)
			{
				error = "Editor.bin -> docx: x2t returned " + std::to_string(code);
				return;
			}
			resultDir = workDir + FILE_SEPARATOR_STR + L"result";
			Unpack(result, resultDir, error);
		}

		void SetUp() override
		{
			ASSERT_TRUE(error.empty()) << error;
		}

		static std::wstring workDir;
		static std::wstring mediaDir;
		static std::wstring resultDir;
		static std::string error;
	};

	std::wstring VmlFillRelationshipId::workDir;
	std::wstring VmlFillRelationshipId::mediaDir;
	std::wstring VmlFillRelationshipId::resultDir;
	std::string VmlFillRelationshipId::error;

	// Opening: the background picture is extracted next to Editor.bin like any other picture.
	TEST_F(VmlFillRelationshipId, OpenKeepsThePageBackgroundPicture)
	{
		EXPECT_EQ(1u, NSDirectory::GetFiles(mediaDir).size()) << "pictures in " << U_TO_UTF8(mediaDir);
	}

	// Saving: the background comes back with its picture, and its fill points to that picture.
	TEST_F(VmlFillRelationshipId, SaveKeepsThePageBackgroundPicture)
	{
		std::wstring word = resultDir + FILE_SEPARATOR_STR + L"word";
		std::vector<std::wstring> media = NSDirectory::GetFiles(word + FILE_SEPARATOR_STR + L"media");
		ASSERT_EQ(1u, media.size()) << "pictures in word/media of the saved docx";
		std::wstring name = media[0].substr(media[0].find_last_of(L"/\\") + 1);
		std::string picture = "media/" + U_TO_UTF8(name);

		std::string document = ReadText(word + FILE_SEPARATOR_STR + L"document.xml");
		size_t begin = document.find("<w:background");
		ASSERT_NE(std::string::npos, begin) << document;
		size_t end = document.find("</w:background>", begin);
		ASSERT_NE(std::string::npos, end) << document.substr(begin, 300);
		std::string background = document.substr(begin, end - begin);

		std::string fill = TagAt(background, background.find("<v:fill "));
		ASSERT_FALSE(fill.empty()) << background;
		std::string id = AttributeValue(fill, "r:id");
		ASSERT_FALSE(id.empty()) << fill;

		std::string rels = ReadText(word + FILE_SEPARATOR_STR + L"_rels" + FILE_SEPARATOR_STR + L"document.xml.rels");
		std::string relationship = FindTag(rels, "<Relationship ", "Id", id);
		ASSERT_FALSE(relationship.empty()) << "no relationship " << id << " in " << rels;
		std::string target = AttributeValue(relationship, "Target");
		ASSERT_GE(target.size(), picture.size()) << relationship;
		EXPECT_EQ(picture, target.substr(target.size() - picture.size())) << relationship;
	}
}
