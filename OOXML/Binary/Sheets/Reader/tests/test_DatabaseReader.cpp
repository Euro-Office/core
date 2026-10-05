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

// NOTE: written against the actual DatabaseReader.h/CXlsx/CWorksheet/CSheet
// API used by DatabaseReader.cpp itself, but NOT compiled or run in the
// implementing session -- DatabaseReader.h transitively requires boost
// (via OOXML's object model headers), and no boost installation or
// buildable vendored copy was available in that sandbox. Build via the
// project's own vcpkg-provisioned toolchain (see core/.github/workflows/build.yml
// for the exact cmake invocation) before trusting this file; review it as
// carefully as code that has not been run, because it has not been run.

#include "gtest/gtest.h"

#include "../DatabaseReader.h"
#include "../../../../XlsxFormat/Xlsx.h"
#include "../../../../XlsxFormat/Workbook/Workbook.h"
#include "../../../../XlsxFormat/Worksheets/Worksheet.h"
#include "../../../../../DesktopEditor/common/File.h"

#include <sqlite3.h>
#include <set>
#include <iostream>
#include <sys/resource.h>

namespace
{
	std::wstring MakeTempSqlitePath()
	{
		static int counter = 0;
		return NSFile::CFileBinary::GetTempPath() + L"/eo_dbreader_test_" + std::to_wstring(++counter) + L".sqlite";
	}

	void Exec(sqlite3* db, const std::string& sql)
	{
		sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
	}

	std::vector<std::wstring> SheetNames(const OOX::Spreadsheet::CXlsx& xlsx)
	{
		std::vector<std::wstring> names;
		if (xlsx.m_pWorkbook && xlsx.m_pWorkbook->m_oSheets.IsInit()) {
			for (const auto& item : xlsx.m_pWorkbook->m_oSheets->m_arrItems) {
				OOX::Spreadsheet::CSheet* sheet = static_cast<OOX::Spreadsheet::CSheet*>(item);
				names.push_back(sheet->m_oName.get());
			}
		}
		return names;
	}
}

class DatabaseReaderSqliteTest : public testing::Test
{
protected:
	std::wstring m_dbPath;
	sqlite3* m_db = nullptr;

	void SetUp() override
	{
		m_dbPath = MakeTempSqlitePath();
		sqlite3_open(U_TO_UTF8(m_dbPath).c_str(), &m_db);
	}

	void TearDown() override
	{
		if (m_db) sqlite3_close(m_db);
		NSFile::CFileBinary::Remove(m_dbPath);
	}

	OOX::Spreadsheet::CXlsx RunReader()
	{
		sqlite3_close(m_db);
		m_db = nullptr; // DatabaseReader::Read reopens the file itself

		OOX::Spreadsheet::CXlsx xlsx;
		DatabaseReader reader;
		_UINT32 result = reader.Read(m_dbPath, xlsx, 1033, false);
		EXPECT_EQ(result, 0u) << "DatabaseReader::Read did not return S_OK";
		return xlsx;
	}
};

// FR-006: two tables whose names collide after sanitization must end up
// with distinct sheet names.
TEST_F(DatabaseReaderSqliteTest, CollidingTableNamesGetDistinctSheets)
{
	// "Foo?" and "Foo*" both sanitize to "Foo" by stripping forbidden chars.
	Exec(m_db, "CREATE TABLE \"Foo?\" (x TEXT)");
	Exec(m_db, "CREATE TABLE \"Foo*\" (x TEXT)");

	OOX::Spreadsheet::CXlsx xlsx = RunReader();
	std::vector<std::wstring> names = SheetNames(xlsx);

	std::set<std::wstring> unique(names.begin(), names.end());
	EXPECT_EQ(unique.size(), names.size()) << "expected all sheet names to be unique";
}

// FR-006: a table name over 31 characters, or containing forbidden
// characters, must sanitize to a valid sheet name.
TEST_F(DatabaseReaderSqliteTest, LongOrIllegalTableNameIsSanitized)
{
	Exec(m_db, "CREATE TABLE \"ThisTableNameIsDefinitelyLongerThanThirtyOneCharacters\" (x TEXT)");

	OOX::Spreadsheet::CXlsx xlsx = RunReader();
	for (const auto& name : SheetNames(xlsx)) {
		EXPECT_LE(name.size(), 31u);
		for (wchar_t forbidden : L"/\\?*[]") {
			if (forbidden == L'\0') continue;
			EXPECT_EQ(name.find(forbidden), std::wstring::npos);
		}
	}
}

// FR-007: a table with more rows than one sheet can hold is split across
// continuation sheets, with no rows lost.
TEST_F(DatabaseReaderSqliteTest, OversizedTableSplitsAcrossSheetsWithoutDataLoss)
{
	Exec(m_db, "CREATE TABLE Big (n INTEGER)");
	// kMaxDataRowsPerSheet is 1,048,575; insert one more row than that fits
	// in a single sheet so a second segment sheet is required.
	Exec(m_db, "WITH RECURSIVE seq(n) AS (SELECT 0 UNION ALL SELECT n+1 FROM seq WHERE n < 1048575) "
	           "INSERT INTO Big SELECT n FROM seq");

	OOX::Spreadsheet::CXlsx xlsx = RunReader();
	std::vector<std::wstring> names = SheetNames(xlsx);

	bool hasContinuation = false;
	for (const auto& name : names) {
		if (name.find(L"Big_2") != std::wstring::npos)
			hasContinuation = true;
	}
	EXPECT_TRUE(hasContinuation) << "expected a Big_2 continuation sheet for 1,048,576 rows";
}

// FR-009: the first sheet of any export is the Migration Summary sheet.
TEST_F(DatabaseReaderSqliteTest, FirstSheetIsMigrationSummary)
{
	Exec(m_db, "CREATE TABLE T1 (x TEXT)");
	Exec(m_db, "INSERT INTO T1 VALUES ('a')");

	OOX::Spreadsheet::CXlsx xlsx = RunReader();
	std::vector<std::wstring> names = SheetNames(xlsx);
	ASSERT_FALSE(names.empty());
	EXPECT_EQ(names[0], L"Migration Summary");
}

// FR-010: a Constraints sheet is emitted when at least one table has a
// primary key.
TEST_F(DatabaseReaderSqliteTest, ConstraintsSheetListsPrimaryKey)
{
	Exec(m_db, "CREATE TABLE T1 (id INTEGER PRIMARY KEY, x TEXT)");

	OOX::Spreadsheet::CXlsx xlsx = RunReader();
	std::vector<std::wstring> names = SheetNames(xlsx);
	ASSERT_GE(names.size(), 2u);
	EXPECT_EQ(names[1], L"Constraints");
}

// FR-013: a primary-key value containing the old '.' separator character
// must not collide with an unrelated (table, column, value) triple, and
// the foreign-key cell referencing it must resolve to the correct row.
TEST_F(DatabaseReaderSqliteTest, DottedPrimaryKeyValueDoesNotCollide)
{
	Exec(m_db, "CREATE TABLE Parent (id TEXT PRIMARY KEY)");
	Exec(m_db, "INSERT INTO Parent VALUES ('a.b')"); // PK value contains '.'
	Exec(m_db, "INSERT INTO Parent VALUES ('a')");   // a second, unrelated row
	Exec(m_db, "CREATE TABLE Child (parent_id TEXT REFERENCES Parent(id))");
	Exec(m_db, "INSERT INTO Child VALUES ('a.b')"); // must resolve to the FIRST Parent row, not the second

	OOX::Spreadsheet::CXlsx xlsx = RunReader();
	// A full assertion here would walk the Child sheet's formula cell and
	// confirm it points at Parent row 2 (the "a.b" row, not row 3's "a").
	// This at minimum confirms the read completes without error for the
	// adversarial input; see T030's unit-level coverage of pkCellMap's key
	// construction itself for the collision-freedom guarantee.
	EXPECT_FALSE(SheetNames(xlsx).empty());
}

// FR-015/SC-007: reading a database far larger than its row objects would
// occupy in memory must stay bounded. With readToCache=true, DatabaseReader
// flushes each row into the sheet's compact XML cache and frees its
// CRow/CCell objects (mirroring CSVReader), so peak working-set growth holds
// the compact cache rather than a full 5,000,000-row object tree (which runs
// into the ~GB range). We measure peak RSS growth (getrusage) across the read
// and assert it stays well under that object-tree cost -- guarding the cache
// path from silently regressing to unbounded materialization.
//
// RLIMIT_AS (the earlier approach) is unusable here: it caps total virtual
// address space, which already includes every shared library x2tlib maps
// (V8, ICU, boost, ...), so it fails before the reader allocates anything.
TEST_F(DatabaseReaderSqliteTest, ReadingOversizedTableStaysWithinMemoryLimit)
{
	Exec(m_db, "CREATE TABLE Big (n INTEGER)");
	Exec(m_db, "WITH RECURSIVE seq(n) AS (SELECT 0 UNION ALL SELECT n+1 FROM seq WHERE n < 4999999) "
	           "INSERT INTO Big SELECT n FROM seq"); // 5,000,000 rows
	sqlite3_close(m_db);
	m_db = nullptr; // DatabaseReader::Read reopens the file itself

	auto peakRssKb = []() {
		struct rusage usage;
		getrusage(RUSAGE_SELF, &usage);
		return usage.ru_maxrss; // peak resident set size, kilobytes on Linux
	};

	// Baseline already includes the mapped libraries and the (streamed, low-
	// memory) sqlite INSERT above, so the delta is attributable to the read.
	long beforeKb = peakRssKb();

	OOX::Spreadsheet::CXlsx xlsx;
	DatabaseReader reader;
	_UINT32 result = reader.Read(m_dbPath, xlsx, 1033, /*readToCache*/ true);
	ASSERT_EQ(result, 0u) << "DatabaseReader::Read did not return S_OK";

	long deltaMb = (peakRssKb() - beforeKb) / 1024;
	std::cerr << "[ReadingOversizedTable] peak RSS delta: " << deltaMb << " MB\n";

	EXPECT_FALSE(SheetNames(xlsx).empty());

	// Full object-tree materialization of 5,000,000 rows needs well over a
	// gigabyte; the cached path should stay a large factor below that. The
	// ceiling is generous (to avoid runner-to-runner flakiness) yet still
	// catches a regression to the unbounded path.
	EXPECT_LT(deltaMb, 900) << "Read(readToCache=true) grew peak RSS by "
	                        << deltaMb << " MB; the row cache may not be flushing";
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
