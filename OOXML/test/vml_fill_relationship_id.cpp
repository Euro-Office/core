/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// A VML fill names its picture with r:id (Word writes the page background as
// <w:background><v:background><v:fill r:id="..." type="frame"/>, and so does
// x2t when it saves one). The drawing converter must resolve that id, or the
// picture is dropped when the document is opened (docx -> Editor.bin) and the
// next save writes the background without it.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
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

	// Saving: the background comes back with its picture.
	TEST_F(VmlFillRelationshipId, SaveKeepsThePageBackgroundPicture)
	{
		std::wstring word = resultDir + FILE_SEPARATOR_STR + L"word";
		EXPECT_EQ(1u, NSDirectory::GetFiles(word + FILE_SEPARATOR_STR + L"media").size())
			<< "pictures in word/media of the saved docx";

		std::string document = ReadText(word + FILE_SEPARATOR_STR + L"document.xml");
		size_t begin = document.find("<w:background");
		ASSERT_NE(std::string::npos, begin) << document;
		std::string background = document.substr(begin, document.find("</w:background>", begin) - begin);
		EXPECT_NE(std::string::npos, background.find("<v:fill r:id=\"")) << background;
	}
}
