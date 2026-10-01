#pragma once

#include "IDatabaseEngine.h"
#include <duckdb.h>

namespace NExtractTools
{
	// Streams the result one chunk at a time via duckdb_fetch_chunk (FR-015,
	// FR-016) instead of the deprecated duckdb_row_count/duckdb_value_varchar
	// whole-result accessors. QueryTable() casts every column to VARCHAR in
	// SQL, so every vector this class reads is guaranteed to be VARCHAR --
	// no per-duckdb_type dispatch is needed to stringify arbitrary column
	// types.
	class DuckDbResultSet : public IDBResultSet
	{
	public:
		DuckDbResultSet(duckdb_result result);
		virtual ~DuckDbResultSet();

		virtual bool Next() override;
		virtual std::wstring GetString(int columnIdx) override;

	private:
		duckdb_result m_result;
		duckdb_data_chunk m_chunk = nullptr;
		idx_t m_chunkSize = 0;
		idx_t m_rowInChunk = 0;
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
