/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// docx -> Editor.bin -> docx. Tracked changes (w:ins, w:del) inside a legacy
// VML text box (w:pict > v:shape > v:textbox > w:txbxContent, without
// mc:AlternateContent) must survive: the node based reader used for these text
// boxes skipped w:ins and w:del together with their text, so both the inserted
// and the deleted text were gone after opening and saving the document.
//
// The fixture has one tracked insertion and one tracked deletion in the body
// (control: these always worked) and two insertions and two deletions in two
// legacy text boxes. Every revision carries a unique word.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace
{
	class DocxVmlTextBoxTrackedChanges : public ::testing::Test
	{
	public:
		static void SetUpTestSuite()
		{
			std::wstring workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"docx" FILE_SEPARATOR_STR L"vml_textbox_tracked_changes.docx";
			std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
			std::wstring result = workDir + FILE_SEPARATOR_STR + L"result.docx";

			// One x2t temp dir per step: x2t does not clean a temp dir it is given.
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
			code = ConvertFile(CreateParamsFile(bin, result, saveTempDir, AVS_OFFICESTUDIO_FILE_CANVAS_WORD,
												AVS_OFFICESTUDIO_FILE_DOCUMENT_DOCX));
			if (code != 0)
			{
				error = "Editor.bin -> docx: x2t returned " + std::to_string(code);
				return;
			}
			std::wstring resultDir = workDir + FILE_SEPARATOR_STR + L"result";
			if (!Unpack(result, resultDir, error))
				return;

			std::ifstream file(std::filesystem::path(resultDir + FILE_SEPARATOR_STR + L"word" + FILE_SEPARATOR_STR + L"document.xml"),
							   std::ios::binary);
			if (!file)
			{
				error = "word/document.xml is missing in the result";
				return;
			}
			document.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		}

		void SetUp() override
		{
			ASSERT_TRUE(error.empty()) << error;
		}

		static bool Contains(const char* text)
		{
			return document.find(text) != std::string::npos;
		}

		static std::string document; // word/document.xml of the round-tripped file
		static std::string error;
	};

	std::string DocxVmlTextBoxTrackedChanges::document;
	std::string DocxVmlTextBoxTrackedChanges::error;
}

// Control: tracked changes in the body never went through the node based reader.
TEST_F(DocxVmlTextBoxTrackedChanges, BodyRevisionsAreKept)
{
	EXPECT_TRUE(Contains("BodyInsertedAlpha")) << "inserted text in the body is lost";
	EXPECT_TRUE(Contains("BodyDeletedBeta")) << "deleted text in the body is lost";
}

TEST_F(DocxVmlTextBoxTrackedChanges, InsertedTextInTextBoxIsKept)
{
	EXPECT_TRUE(Contains("BoxInsertedGamma")) << "inserted text in the first text box is lost";
	EXPECT_TRUE(Contains("BoxInsertedEpsilon")) << "inserted text in the second text box is lost";
}

TEST_F(DocxVmlTextBoxTrackedChanges, DeletedTextInTextBoxIsKept)
{
	EXPECT_TRUE(Contains("BoxDeletedDelta")) << "deleted text in the first text box is lost";
	EXPECT_TRUE(Contains("BoxDeletedZeta")) << "deleted text in the second text box is lost";
}

// The text must come back as revisions, not as plain text. A text box may be
// written twice (wps:txbx and its VML fallback), hence "at least".
TEST_F(DocxVmlTextBoxTrackedChanges, RevisionMarksInTextBoxAreKept)
{
	auto count = [](const char* text) {
		size_t n = 0;
		for (size_t pos = document.find(text); pos != std::string::npos; pos = document.find(text, pos + 1))
			++n;
		return n;
	};
	// One of each in the body, two of each in the text boxes.
	EXPECT_GE(count("<w:ins "), 3u) << "expected at least 3 w:ins in word/document.xml";
	EXPECT_GE(count("<w:del "), 3u) << "expected at least 3 w:del in word/document.xml";
	EXPECT_GE(count("<w:delText"), 3u) << "expected at least 3 w:delText in word/document.xml";
}
