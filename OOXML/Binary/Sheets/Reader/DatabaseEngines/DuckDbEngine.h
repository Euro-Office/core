#pragma once

#include "IDatabaseEngine.h"
#include "duckdb/duckdb.h"

namespace NExtractTools
{
	class DuckDbResultSet : public IDBResultSet
	{
	public:
		DuckDbResultSet(duckdb_result result);
		virtual ~DuckDbResultSet();

		virtual bool Next() override;
		virtual std::wstring GetString(int columnIdx) override;

	private:
		duckdb_result m_result;
		idx_t m_currentRow;
		idx_t m_rowCount;
	};

	class DuckDbEngine : public IDatabaseEngine
	{
	public:
		DuckDbEngine();
		virtual ~DuckDbEngine();

		virtual bool Open(const std::wstring& path) override;
		virtual std::vector<std::wstring> GetTableNames() override;
		virtual TableSchema GetTableSchema(const std::wstring& tableName) override;
		virtual std::unique_ptr<IDBResultSet> QueryTable(const std::wstring& tableName) override;

	private:
		duckdb_database m_db;
		duckdb_connection m_conn;
	};
}
