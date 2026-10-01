#include "gtest/gtest.h"

#include "../DuckDbEngine.h"
#include "../../../../../../DesktopEditor/common/File.h"

#include <duckdb.h>

using namespace NExtractTools;

namespace
{
	// Creates a temp DuckDB file with one table whose name and one column
	// name both contain a literal single-quote character -- the same
	// adversarial case as SqliteEngine's identifier-quoting test: a naive
	// `"SELECT * FROM '" + name + "'"` string literal breaks on an embedded
	// unescaped quote, while identifier quoting (double quotes) does not.
	std::wstring MakeQuotedIdentifierFixture()
	{
		static int counter = 0;
		std::wstring path = NSFile::CFileBinary::GetTempPath() + L"/eo_duckdb_quote_test_" + std::to_wstring(++counter) + L".duckdb";

		duckdb_database db = nullptr;
		duckdb_connection conn = nullptr;
		duckdb_open(U_TO_UTF8(path).c_str(), &db);
		duckdb_connect(db, &conn);
		duckdb_result result;
		duckdb_query(conn, "CREATE TABLE \"Weird'Table\" (\"Odd'Col\" VARCHAR PRIMARY KEY)", &result);
		duckdb_destroy_result(&result);
		duckdb_query(conn, "INSERT INTO \"Weird'Table\" (\"Odd'Col\") VALUES ('hello')", &result);
		duckdb_destroy_result(&result);
		duckdb_disconnect(&conn);
		duckdb_close(&db);

		return path;
	}
}

class DuckDbEngineIdentifierQuotingTest : public testing::Test
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
TEST_F(DuckDbEngineIdentifierQuotingTest, TableNameWithEmbeddedQuoteIsQueryable)
{
	DuckDbEngine engine;
	ASSERT_TRUE(engine.Open(m_dbPath));

	std::vector<std::wstring> tables = engine.GetTableNames();
	ASSERT_EQ(tables.size(), 1u);
	EXPECT_EQ(tables[0], L"Weird'Table");

	TableSchema schema = engine.GetTableSchema(tables[0]);
	ASSERT_EQ(schema.columns.size(), 1u);
	EXPECT_EQ(schema.columns[0], L"Odd'Col");

	std::unique_ptr<IDBResultSet> rs = engine.QueryTable(tables[0]);
	ASSERT_TRUE(rs);
	ASSERT_TRUE(rs->Next());
	EXPECT_EQ(rs->GetString(0), L"hello");
	EXPECT_FALSE(rs->Next());
}

// FR-016: the DuckDB-backed engine must not depend on duckdb_row_count or
// duckdb_value_varchar. This is verified at the source level by T035
// (static grep check / build-time macro gate), not re-asserted here; this
// suite instead confirms the chunk-based replacement produces correct
// results, including across a result spanning more than one internal
// chunk (DuckDB's default chunk size is 2048 rows).
TEST_F(DuckDbEngineIdentifierQuotingTest, ResultSpanningMultipleChunksReadsAllRows)
{
	std::wstring path = NSFile::CFileBinary::GetTempPath() + L"/eo_duckdb_chunk_test.duckdb";
	duckdb_database db = nullptr;
	duckdb_connection conn = nullptr;
	duckdb_open(U_TO_UTF8(path).c_str(), &db);
	duckdb_connect(db, &conn);
	duckdb_result result;
	duckdb_query(conn, "CREATE TABLE Big (n INTEGER)", &result);
	duckdb_destroy_result(&result);
	duckdb_query(conn, "INSERT INTO Big SELECT * FROM range(5000)", &result);
	duckdb_destroy_result(&result);
	duckdb_disconnect(&conn);
	duckdb_close(&db);

	DuckDbEngine engine;
	ASSERT_TRUE(engine.Open(path));
	std::unique_ptr<IDBResultSet> rs = engine.QueryTable(L"Big");
	ASSERT_TRUE(rs);

	int count = 0;
	while (rs->Next()) {
		EXPECT_EQ(rs->GetString(0), std::to_wstring(count));
		++count;
	}
	EXPECT_EQ(count, 5000);

	NSFile::CFileBinary::Remove(path);
}
