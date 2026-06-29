#include "DuckDbEngine.h"
#include "../../../../../DesktopEditor/common/File.h"

namespace NExtractTools
{
	DuckDbResultSet::DuckDbResultSet(duckdb_result result) : m_result(result), m_currentRow(0)
	{
		m_rowCount = duckdb_row_count(&m_result);
	}

	DuckDbResultSet::~DuckDbResultSet()
	{
		duckdb_destroy_result(&m_result);
	}

	bool DuckDbResultSet::Next()
	{
		if (m_currentRow < m_rowCount)
		{
			m_currentRow++;
			return true;
		}
		return false;
	}

	std::wstring DuckDbResultSet::GetString(int columnIdx)
	{
		if (m_currentRow == 0 || m_currentRow > m_rowCount) return L"";
		
		char* val = duckdb_value_varchar(&m_result, columnIdx, m_currentRow - 1);
		if (!val) return L"";
		
		std::string sVal(val);
		duckdb_free(val);
		return UTF8_TO_U(sVal);
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
			idx_t rowCount = duckdb_row_count(&result);
			for (idx_t i = 0; i < rowCount; i++)
			{
				char* val = duckdb_value_varchar(&result, 0, i);
				if (val)
				{
					std::string sVal(val);
					tables.push_back(UTF8_TO_U(sVal));
					duckdb_free(val);
				}
			}
			duckdb_destroy_result(&result);
		}
		
		return tables;
	}

	TableSchema DuckDbEngine::GetTableSchema(const std::wstring& tableName)
	{
		TableSchema schema;
		if (!m_conn) return schema;

		std::string sTableName = U_TO_UTF8(tableName);
		std::string sql = "PRAGMA table_info('" + sTableName + "')";
		
		duckdb_result result;
		if (duckdb_query(m_conn, sql.c_str(), &result) == DuckDBSuccess)
		{
			idx_t rowCount = duckdb_row_count(&result);
			for (idx_t i = 0; i < rowCount; i++)
			{
				// PRAGMA table_info returns: cid, name, type, notnull, dflt_value, pk
				char* val = duckdb_value_varchar(&result, 1, i);
				char* pk = duckdb_value_varchar(&result, 5, i);
				if (val)
				{
					std::string sVal(val);
					std::wstring wColName = UTF8_TO_U(sVal);
					schema.columns.push_back(wColName);
					if (pk && std::string(pk) == "true") { // DuckDB PRAGMA table_info returns "true" or "false" for pk usually, or "1" / "0"
						schema.primaryKeys.push_back(wColName);
					} else if (pk && std::string(pk) == "1") {
						schema.primaryKeys.push_back(wColName);
					}
					duckdb_free(val);
				}
				if (pk) duckdb_free(pk);
			}
			duckdb_destroy_result(&result);
		}
		
		// DuckDB's support for PRAGMA foreign_key_list is limited or missing compared to SQLite.
		// For DuckDB, we'll extract columns but might skip foreign keys if not explicitly supported 
		// by PRAGMA foreign_key_list in standard way. But let's try it just in case:
		std::string fkSql = "PRAGMA foreign_key_list('" + sTableName + "')";
		duckdb_result fkResult;
		if (duckdb_query(m_conn, fkSql.c_str(), &fkResult) == DuckDBSuccess)
		{
			idx_t rowCount = duckdb_row_count(&fkResult);
			for (idx_t i = 0; i < rowCount; i++)
			{
				// id, seq, table, from, to
				char* table = duckdb_value_varchar(&fkResult, 2, i);
				char* from = duckdb_value_varchar(&fkResult, 3, i);
				char* to = duckdb_value_varchar(&fkResult, 4, i);
				
				if (table && from && to)
				{
					ForeignKeyDef fk;
					std::string sTable(table);
					std::string sFrom(from);
					std::string sTo(to);
					
					fk.referencedTable = UTF8_TO_U(sTable);
					fk.columnName = UTF8_TO_U(sFrom);
					fk.referencedColumn = UTF8_TO_U(sTo);
					schema.foreignKeys.push_back(fk);
				}
				
				if (table) duckdb_free(table);
				if (from) duckdb_free(from);
				if (to) duckdb_free(to);
			}
			duckdb_destroy_result(&fkResult);
		}

		return schema;
	}

	std::unique_ptr<IDBResultSet> DuckDbEngine::QueryTable(const std::wstring& tableName)
	{
		if (!m_conn) return nullptr;

		std::string sTableName = U_TO_UTF8(tableName);
		std::string sql = "SELECT * FROM '" + sTableName + "'";
		
		duckdb_result result;
		if (duckdb_query(m_conn, sql.c_str(), &result) == DuckDBSuccess)
		{
			return std::unique_ptr<IDBResultSet>(new DuckDbResultSet(result));
		}
		
		return nullptr;
	}
}
