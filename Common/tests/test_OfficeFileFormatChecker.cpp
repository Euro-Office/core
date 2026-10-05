/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "gtest/gtest.h"

#include "../OfficeFileFormatChecker.h"
#include "../OfficeFileFormats.h"
#include "../../DesktopEditor/common/File.h"

#include <string>

namespace
{
	// Writes raw bytes to a temp file and returns its path. The caller is
	// responsible for deleting the file (tests do so in TearDown).
	std::wstring WriteTempFile(const std::wstring& nameHint, const std::string& bytes)
	{
		static int counter = 0;
		std::wstring path = NSFile::CFileBinary::GetTempPath() + L"/eo_fmt_test_" + std::to_wstring(++counter) + nameHint;

		NSFile::CFileBinary file;
		file.CreateFileW(path);
		file.WriteFile((BYTE*)bytes.data(), (DWORD)bytes.size());
		file.CloseFile();
		return path;
	}
}

class OfficeFileFormatCheckerDbTest : public testing::Test
{
protected:
	std::vector<std::wstring> m_tempFiles;

	std::wstring MakeTempFile(const std::wstring& nameHint, const std::string& bytes)
	{
		std::wstring path = WriteTempFile(nameHint, bytes);
		m_tempFiles.push_back(path);
		return path;
	}

	void TearDown() override
	{
		for (const auto& path : m_tempFiles)
			NSFile::CFileBinary::Remove(path);
	}
};

// FR-003: GetFormatByExtension must recognize ".parquet"/".pq" as Parquet,
// which it previously had no branch for at all.
TEST_F(OfficeFileFormatCheckerDbTest, ParquetExtensionIsRecognized)
{
	EXPECT_EQ(COfficeFileFormatChecker::GetFormatByExtension(L".parquet"), AVS_OFFICESTUDIO_FILE_SPREADSHEET_PARQUET);
	EXPECT_EQ(COfficeFileFormatChecker::GetFormatByExtension(L".pq"), AVS_OFFICESTUDIO_FILE_SPREADSHEET_PARQUET);
}

// FR-002: ".fdb" must no longer resolve to a routable format -- the dead
// Firebird route was removed.
TEST_F(OfficeFileFormatCheckerDbTest, FdbExtensionIsNoLongerRecognized)
{
	EXPECT_EQ(COfficeFileFormatChecker::GetFormatByExtension(L".fdb"), AVS_OFFICESTUDIO_FILE_UNKNOWN);
}

// FR-004: a ".db"-extension file's true format must be determined by
// sniffing its content, not assumed from the extension -- three-way across
// SQLite, Access/Jet, and Berkeley DB.
TEST_F(OfficeFileFormatCheckerDbTest, AmbiguousDbExtensionSniffsSqlite)
{
	// The real SQLite file header is exactly these 16 bytes: "SQLite format 3"
	// (15 printable characters) followed by one embedded NUL byte.
	std::string sqliteHeader("SQLite format 3", 15);
	sqliteHeader += '\0';
	std::wstring path = MakeTempFile(L".db", sqliteHeader + std::string(100, '\0'));

	COfficeFileFormatChecker checker;
	EXPECT_TRUE(checker.isOfficeFile(path));
	EXPECT_EQ(checker.nFileType, AVS_OFFICESTUDIO_FILE_SPREADSHEET_SQLITE);
}

TEST_F(OfficeFileFormatCheckerDbTest, AmbiguousDbExtensionSniffsAccessJet)
{
	// "Standard Jet DB" must appear at byte offset 4.
	std::string header(4, '\0');
	header += "Standard Jet DB";
	header += std::string(100, '\0');
	std::wstring path = MakeTempFile(L".db", header);

	COfficeFileFormatChecker checker;
	EXPECT_TRUE(checker.isOfficeFile(path));
	EXPECT_EQ(checker.nFileType, AVS_OFFICESTUDIO_FILE_SPREADSHEET_MDB);
}

TEST_F(OfficeFileFormatCheckerDbTest, AmbiguousDbExtensionFallsBackToBerkeleyDb)
{
	// Bytes matching neither the SQLite nor the Access/Jet signature.
	std::wstring path = MakeTempFile(L".db", std::string(100, (char)0xAB));

	COfficeFileFormatChecker checker;
	EXPECT_TRUE(checker.isOfficeFile(path));
	EXPECT_EQ(checker.nFileType, AVS_OFFICESTUDIO_FILE_SPREADSHEET_BDB);
}
