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


#include "DuckDbEngine.h"
#include "../../../../../DesktopEditor/common/File.h"
#include "SqlIdentifierQuoting.h"
#include <algorithm>

namespace
{
	std::wstring ReadVarcharCell(duckdb_vector vec, idx_t row)
	{
		uint64_t* validity = duckdb_vector_get_validity(vec);
		if (validity && !duckdb_validity_row_is_valid(validity, row))
			return L""; // SQL NULL

		duckdb_string_t* data = (duckdb_string_t*)duckdb_vector_get_data(vec);
		duckdb_string_t str = data[row];
		uint32_t len = duckdb_string_t_length(str);
		const char* ptr = duckdb_string_t_data(&str);
		if (!ptr || len == 0) return L"";

		std::string sVal(ptr, len);
		return UTF8_TO_U(sVal);
	}

	// Streams every row of a (small, metadata-sized) duckdb_result via the
	// chunk API (FR-016), calling rowCallback(chunk, rowInChunk) for each.
	template<typename RowCallback>
	void ForEachDuckDbRow(duckdb_result& result, RowCallback rowCallback)
	{
		duckdb_data_chunk chunk = nullptr;
		for (;;)
		{
			if (chunk)
			{
				duckdb_destroy_data_chunk(&chunk);
				chunk = nullptr;
			}
			chunk = duckdb_fetch_chunk(result);
			if (!chunk)
				break;
			idx_t chunkSize = duckdb_data_chunk_get_size(chunk);
			for (idx_t row = 0; row < chunkSize; ++row)
				rowCallback(chunk, row);
		}
	}
}

namespace NExtractTools
{
	DuckDbResultSet::DuckDbResultSet(duckdb_result result) : m_result(result)
	{
	}

	DuckDbResultSet::~DuckDbResultSet()
	{
		if (m_chunk)
		{
			duckdb_destroy_data_chunk(&m_chunk);
			m_chunk = nullptr;
		}
		duckdb_destroy_result(&m_result);
	}

	bool DuckDbResultSet::Next()
	{
		if (m_chunk && m_rowInChunk + 1 < m_chunkSize)
		{
			m_rowInChunk++;
			return true;
		}

		// Advance to the next chunk (FR-016: duckdb_fetch_chunk, not the
		// deprecated duckdb_row_count/whole-result accessors). Skip any
		// zero-size chunk rather than treating it as end-of-result.
		for (;;)
		{
			if (m_chunk)
			{
				duckdb_destroy_data_chunk(&m_chunk);
				m_chunk = nullptr;
			}
			m_chunk = duckdb_fetch_chunk(m_result);
			if (!m_chunk)
				return false; // result exhausted
			m_chunkSize = duckdb_data_chunk_get_size(m_chunk);
			if (m_chunkSize == 0)
				continue;
			m_rowInChunk = 0;
			return true;
		}
	}

	std::wstring DuckDbResultSet::GetString(int columnIdx)
	{
		if (!m_chunk) return L"";

		// Every column QueryTable() selects is explicitly CAST(... AS VARCHAR),
		// so this vector's data is always duckdb_string_t -- no per-duckdb_type
		// dispatch is needed here.
		duckdb_vector vec = duckdb_data_chunk_get_vector(m_chunk, columnIdx);
		return ReadVarcharCell(vec, m_rowInChunk);
	}

	DuckDbEngine::DuckDbEngine() : m_db(nullptr), m_conn(nullptr)
	{
	}

	DuckDbEngine::~DuckDbEngine()
	{
		if (m_conn)
		{
			duckdb_disconnect(&m_conn);
			m_conn = nullptr;
		}
		if (m_db)
		{
			duckdb_close(&m_db);
			m_db = nullptr;
		}
	}

	bool DuckDbEngine::Open(const std::wstring& path)
	{
		std::string sPath = U_TO_UTF8(path);

		std::string sExt;
		std::string::size_type nExtPos = sPath.rfind('.');
		if (nExtPos != std::string::npos) {
			sExt = sPath.substr(nExtPos);
			std::transform(sExt.begin(), sExt.end(), sExt.begin(), ::tolower);
		}

		if (sExt == ".parquet" || sExt == ".pq") {
			if (duckdb_open(NULL, &m_db) == DuckDBError)
			{
				return false;
			}
			if (duckdb_connect(m_db, &m_conn) == DuckDBError)
			{
				duckdb_close(&m_db);
				m_db = nullptr;
				return false;
			}
			std::string sql = "CREATE VIEW \"ParquetData\" AS SELECT * FROM '" + sPath + "'";
			duckdb_result result;
			if (duckdb_query(m_conn, sql.c_str(), &result) != DuckDBSuccess) {
				const char* error = duckdb_result_error(&result);
				duckdb_destroy_result(&result);
				return false;
			}
			duckdb_destroy_result(&result);
			return true;
		}

		if (duckdb_open(sPath.c_str(), &m_db) == DuckDBError)
		{
			return false;
		}
		
		if (duckdb_connect(m_db, &m_conn) == DuckDBError)
		{
			duckdb_close(&m_db);
			m_db = nullptr;
			return false;
		}
		
		return true;
	}

	std::vector<std::wstring> DuckDbEngine::GetTableNames()
	{
		std::vector<std::wstring> tables;
		if (!m_conn) return tables;

		duckdb_result result;
		if (duckdb_query(m_conn, "SELECT table_name FROM information_schema.tables WHERE table_schema='main'", &result) == DuckDBSuccess)
		{
			ForEachDuckDbRow(result, [&](duckdb_data_chunk chunk, idx_t row) {
				tables.push_back(ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 0), row));
			});
			duckdb_destroy_result(&result);
		}

		return tables;
	}

	TableSchema DuckDbEngine::GetTableSchema(const std::wstring& tableName)
	{
		TableSchema schema;
		if (!m_conn) return schema;

		std::string sTableName = U_TO_UTF8(tableName);
		std::string sql = "PRAGMA table_info(" + QuoteIdentifier(sTableName) + ")";

		duckdb_result result;
		if (duckdb_query(m_conn, sql.c_str(), &result) == DuckDBSuccess)
		{
			// PRAGMA table_info returns: cid, name, type, notnull, dflt_value, pk
			ForEachDuckDbRow(result, [&](duckdb_data_chunk chunk, idx_t row) {
				std::wstring wColName = ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 1), row);
				std::wstring pk = ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 5), row);
				schema.columns.push_back(wColName);
				// DuckDB's PRAGMA table_info returns "true"/"false" for pk in
				// some versions, "1"/"0" in others -- accept either.
				if (pk == L"true" || pk == L"1")
					schema.primaryKeys.push_back(wColName);
			});
			duckdb_destroy_result(&result);
		}

		// DuckDB's support for PRAGMA foreign_key_list is limited or missing compared to SQLite.
		// For DuckDB, we'll extract columns but might skip foreign keys if not explicitly supported
		// by PRAGMA foreign_key_list in standard way. But let's try it just in case:
		std::string fkSql = "PRAGMA foreign_key_list(" + QuoteIdentifier(sTableName) + ")";
		duckdb_result fkResult;
		if (duckdb_query(m_conn, fkSql.c_str(), &fkResult) == DuckDBSuccess)
		{
			// id, seq, table, from, to
			ForEachDuckDbRow(fkResult, [&](duckdb_data_chunk chunk, idx_t row) {
				ForeignKeyDef fk;
				fk.groupKey = ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 0), row);
				fk.referencedTable = ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 2), row);
				fk.columnName = ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 3), row);
				fk.referencedColumn = ReadVarcharCell(duckdb_data_chunk_get_vector(chunk, 4), row);
				if (!fk.referencedTable.empty() && !fk.columnName.empty() && !fk.referencedColumn.empty())
					schema.foreignKeys.push_back(fk);
			});
			duckdb_destroy_result(&fkResult);
		}

		return schema;
	}

	std::unique_ptr<IDBResultSet> DuckDbEngine::QueryTable(const std::wstring& tableName)
	{
		if (!m_conn) return nullptr;

		std::string sTableName = U_TO_UTF8(tableName);

		// Cast every column to VARCHAR explicitly, so DuckDbResultSet can read
		// every vector as duckdb_string_t without a per-duckdb_type dispatch.
		TableSchema schema = GetTableSchema(tableName);
		std::string columnList;
		for (size_t i = 0; i < schema.columns.size(); ++i) {
			if (i > 0) columnList += ", ";
			columnList += "CAST(" + QuoteIdentifier(U_TO_UTF8(schema.columns[i])) + " AS VARCHAR)";
		}
		if (columnList.empty())
			columnList = "*";

		std::string sql = "SELECT " + columnList + " FROM " + QuoteIdentifier(sTableName);

		duckdb_result result;
		if (duckdb_query(m_conn, sql.c_str(), &result) == DuckDBSuccess)
		{
			return std::unique_ptr<IDBResultSet>(new DuckDbResultSet(result));
		}

		return nullptr;
	}
}
