#include "SqliteEngine.h"
#include "../../../../../DesktopEditor/common/File.h"
#include <iostream>

namespace NExtractTools
{
	SqliteResultSet::SqliteResultSet(sqlite3_stmt* stmt) : m_stmt(stmt)
	{
	}

	SqliteResultSet::~SqliteResultSet()
	{
		if (m_stmt)
		{
			sqlite3_finalize(m_stmt);
			m_stmt = nullptr;
		}
	}

	bool SqliteResultSet::Next()
	{
		if (!m_stmt) return false;
		return sqlite3_step(m_stmt) == SQLITE_ROW;
	}

	std::wstring SqliteResultSet::GetString(int columnIdx)
	{
		if (!m_stmt) return L"";
		
		const unsigned char* val = sqlite3_column_text(m_stmt, columnIdx);
		if (!val) return L"";
		
		std::string sVal((const char*)val);
		return UTF8_TO_U(sVal);
	}

	SqliteEngine::SqliteEngine() : m_db(nullptr)
	{
	}

	SqliteEngine::~SqliteEngine()
	{
		if (m_db)
		{
			sqlite3_close(m_db);
			m_db = nullptr;
		}
	}

	bool SqliteEngine::Open(const std::wstring& path)
	{
		std::string sPath = U_TO_UTF8(path);
		int rc = sqlite3_open(sPath.c_str(), &m_db);
		if (rc != SQLITE_OK)
		{
			if (m_db)
			{
				sqlite3_close(m_db);
				m_db = nullptr;
			}
			return false;
		}
		return true;
	}

	std::vector<std::wstring> SqliteEngine::GetTableNames()
	{
		std::vector<std::wstring> tables;
		if (!m_db) return tables;

		sqlite3_stmt* stmt = nullptr;
		const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
		if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				const unsigned char* name = sqlite3_column_text(stmt, 0);
				if (name)
				{
					std::string sName((const char*)name);
					tables.push_back(UTF8_TO_U(sName));
				}
			}
			sqlite3_finalize(stmt);
		}
		return tables;
	}

	TableSchema SqliteEngine::GetTableSchema(const std::wstring& tableName)
	{
		TableSchema schema;
		if (!m_db) return schema;

		std::string sTableName = U_TO_UTF8(tableName);
		std::string sql = "PRAGMA table_info('" + sTableName + "')";
		
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				const unsigned char* colName = sqlite3_column_text(stmt, 1);
				const unsigned char* pk = sqlite3_column_text(stmt, 5);
				if (colName)
				{
					std::string sColName((const char*)colName);
					std::wstring wColName = UTF8_TO_U(sColName);
					schema.columns.push_back(wColName);
					if (pk && std::string((const char*)pk) != "0") {
						schema.primaryKeys.push_back(wColName);
					}
				}
			}
			sqlite3_finalize(stmt);
		}

		std::string fkSql = "PRAGMA foreign_key_list('" + sTableName + "')";
		if (sqlite3_prepare_v2(m_db, fkSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				const unsigned char* table = sqlite3_column_text(stmt, 2);
				const unsigned char* from = sqlite3_column_text(stmt, 3);
				const unsigned char* to = sqlite3_column_text(stmt, 4);
				
				if (table && from && to)
				{
					ForeignKeyDef fk;
					std::string sTable((const char*)table);
					std::string sFrom((const char*)from);
					std::string sTo((const char*)to);
					
					fk.referencedTable = UTF8_TO_U(sTable);
					fk.columnName = UTF8_TO_U(sFrom);
					fk.referencedColumn = UTF8_TO_U(sTo);
					schema.foreignKeys.push_back(fk);
				}
			}
			sqlite3_finalize(stmt);
		}

		return schema;
	}

	std::unique_ptr<IDBResultSet> SqliteEngine::QueryTable(const std::wstring& tableName)
	{
		if (!m_db) return nullptr;

		std::string sTableName = U_TO_UTF8(tableName);
		std::string sql = "SELECT * FROM '" + sTableName + "'";
		
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			return std::unique_ptr<IDBResultSet>(new SqliteResultSet(stmt));
		}
		
		return nullptr;
	}
}
