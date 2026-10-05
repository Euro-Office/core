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

#include "../SqliteEngine.h"
#include "../../../../../../DesktopEditor/common/File.h"

#include <sqlite3.h>

using namespace NExtractTools;

namespace
{
	// Creates a temp SQLite file with one table whose name and one column
	// name both contain a literal single-quote character. SQLite accepts
	// '...'-quoted strings as identifiers when the grammar requires one
	// (e.g. SELECT * FROM 'Table'), which is why the old string-literal
	// quoting "worked" for ordinary names -- but an *embedded* single quote
	// (e.g. Weird'Table) breaks it, since the old code never doubled the
	// embedded quote: `"SELECT * FROM '" + name + "'"` becomes the malformed
	// `SELECT * FROM 'Weird'Table'`. Identifier quoting (double quotes,
	// doubling an embedded ") has no such problem with an embedded single
	// quote.
	std::wstring MakeQuotedIdentifierFixture()
	{
		static int counter = 0;
		std::wstring path = NSFile::CFileBinary::GetTempPath() + L"/eo_sqlite_quote_test_" + std::to_wstring(++counter) + L".sqlite";

		sqlite3* db = nullptr;
		sqlite3_open(U_TO_UTF8(path).c_str(), &db);
		// Table name: Weird'Table ; column name: Odd'Col
		sqlite3_exec(db, "CREATE TABLE \"Weird'Table\" (\"Odd'Col\" TEXT PRIMARY KEY)", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "INSERT INTO \"Weird'Table\" (\"Odd'Col\") VALUES ('hello')", nullptr, nullptr, nullptr);
		sqlite3_close(db);

		return path;
	}
}

class SqliteEngineIdentifierQuotingTest : public testing::Test
{
protected:
	std::wstring m_dbPath;

	void SetUp() override
	{
		m_dbPath = MakeQuotedIdentifierFixture();
	}

	void TearDown() override
	{
		NSFile::CFileBinary::Remove(m_dbPath);
	}
};

// FR-014: a table/column name containing a quote character must be quoted
// as an identifier, not as a string literal, and the query must still
// succeed and return correct data.
TEST_F(SqliteEngineIdentifierQuotingTest, TableNameWithEmbeddedQuoteIsQueryable)
{
	SqliteEngine engine;
	ASSERT_TRUE(engine.Open(m_dbPath));

	std::vector<std::wstring> tables = engine.GetTableNames();
	ASSERT_EQ(tables.size(), 1u);
	EXPECT_EQ(tables[0], L"Weird'Table");

	TableSchema schema = engine.GetTableSchema(tables[0]);
	ASSERT_EQ(schema.columns.size(), 1u);
	EXPECT_EQ(schema.columns[0], L"Odd'Col");
	ASSERT_EQ(schema.primaryKeys.size(), 1u);
	EXPECT_EQ(schema.primaryKeys[0], L"Odd'Col");

	std::unique_ptr<IDBResultSet> rs = engine.QueryTable(tables[0]);
	ASSERT_TRUE(rs);
	ASSERT_TRUE(rs->Next());
	EXPECT_EQ(rs->GetString(0), L"hello");
	EXPECT_FALSE(rs->Next());
}

namespace
{
	std::wstring MakeGeneratedColumnFixture()
	{
		static int counter = 0;
		std::wstring path = NSFile::CFileBinary::GetTempPath() + L"/eo_sqlite_gencol_test_" + std::to_wstring(++counter) + L".sqlite";

		sqlite3* db = nullptr;
		sqlite3_open(U_TO_UTF8(path).c_str(), &db);
		sqlite3_exec(db, "CREATE TABLE t(a INTEGER, doubled INTEGER GENERATED ALWAYS AS (a*2) VIRTUAL)", nullptr, nullptr, nullptr);
		sqlite3_exec(db, "INSERT INTO t(a) VALUES (7)", nullptr, nullptr, nullptr);
		sqlite3_close(db);

		return path;
	}
}

class SqliteEngineGeneratedColumnTest : public testing::Test
{
protected:
	std::wstring m_dbPath;

	void SetUp() override { m_dbPath = MakeGeneratedColumnFixture(); }
	void TearDown() override { NSFile::CFileBinary::Remove(m_dbPath); }
};

// PR #116 review (5415389555): a generated column is an ordinary, readable
// SELECT target -- table_info/table_xinfo differ only in whether they admit
// it exists, not in whether it can be queried. Its value must still reach
// the recovered worksheet.
TEST_F(SqliteEngineGeneratedColumnTest, GeneratedColumnValueIsExported)
{
	SqliteEngine engine;
	ASSERT_TRUE(engine.Open(m_dbPath));

	TableSchema schema = engine.GetTableSchema(L"t");
	ASSERT_EQ(schema.columns.size(), 2u);
	EXPECT_EQ(schema.columns[0], L"a");
	EXPECT_EQ(schema.columns[1], L"doubled");

	std::unique_ptr<IDBResultSet> rs = engine.QueryTable(L"t");
	ASSERT_TRUE(rs);
	ASSERT_TRUE(rs->Next());
	EXPECT_EQ(rs->GetString(0), L"7");
	EXPECT_EQ(rs->GetString(1), L"14");
	EXPECT_FALSE(rs->Next());
}

namespace
{
	std::wstring MakeIndependentForeignKeysFixture()
	{
		static int counter = 0;
		std::wstring path = NSFile::CFileBinary::GetTempPath() + L"/eo_sqlite_fkgroup_test_" + std::to_wstring(++counter) + L".sqlite";

		sqlite3* db = nullptr;
		sqlite3_open(U_TO_UTF8(path).c_str(), &db);
		sqlite3_exec(db, "CREATE TABLE People(id INTEGER PRIMARY KEY)", nullptr, nullptr, nullptr);
		sqlite3_exec(db,
			"CREATE TABLE Tasks("
			"  owner_id INTEGER REFERENCES People(id),"
			"  reviewer_id INTEGER REFERENCES People(id)"
			")", nullptr, nullptr, nullptr);
		sqlite3_close(db);

		return path;
	}
}

class SqliteEngineForeignKeyGroupingTest : public testing::Test
{
protected:
	std::wstring m_dbPath;

	void SetUp() override { m_dbPath = MakeIndependentForeignKeysFixture(); }
	void TearDown() override { NSFile::CFileBinary::Remove(m_dbPath); }
};

// PR #116 review (5415411907): two single-column FKs that each independently
// reference the same table are separate relationships, not components of
// one composite key. SQLite's PRAGMA foreign_key_list "id" column must give
// them distinct groupKey values so a caller grouping by (referencedTable,
// groupKey) does not merge them.
TEST_F(SqliteEngineForeignKeyGroupingTest, IndependentForeignKeysGetDistinctGroupKeys)
{
	SqliteEngine engine;
	ASSERT_TRUE(engine.Open(m_dbPath));

	TableSchema schema = engine.GetTableSchema(L"Tasks");
	ASSERT_EQ(schema.foreignKeys.size(), 2u);

	const ForeignKeyDef* ownerFk = nullptr;
	const ForeignKeyDef* reviewerFk = nullptr;
	for (const auto& fk : schema.foreignKeys) {
		EXPECT_EQ(fk.referencedTable, L"People");
		EXPECT_EQ(fk.referencedColumn, L"id");
		if (fk.columnName == L"owner_id") ownerFk = &fk;
		if (fk.columnName == L"reviewer_id") reviewerFk = &fk;
	}
	ASSERT_NE(ownerFk, nullptr);
	ASSERT_NE(reviewerFk, nullptr);
	EXPECT_NE(ownerFk->groupKey, reviewerFk->groupKey);
	EXPECT_FALSE(ownerFk->groupKey.empty());
	EXPECT_FALSE(reviewerFk->groupKey.empty());
}
