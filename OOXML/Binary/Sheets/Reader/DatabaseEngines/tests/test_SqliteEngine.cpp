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
