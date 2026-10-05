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

#include "SqliteEngine.h"
#include "../../../../../DesktopEditor/common/File.h"
#include "SqlIdentifierQuoting.h"
#include <iostream>
#include <stdexcept>

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
		// PR #116 review (5389484472): a prepare failure (e.g. a locked
		// schema under SQLITE_BUSY) used to fall through to an empty
		// `tables`, indistinguishable from a database that genuinely has no
		// tables -- DatabaseReader would then report S_OK having exported
		// nothing. Propagate discovery errors separately via an exception,
		// and make sure the row loop itself ended at SQLITE_DONE rather than
		// being cut short by an error mid-enumeration.
		if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
			throw std::runtime_error(std::string("SqliteEngine::GetTableNames prepare failed: ") + sqlite3_errmsg(m_db));

		int rc;
		while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
		{
			const unsigned char* name = sqlite3_column_text(stmt, 0);
			if (name)
			{
				std::string sName((const char*)name);
				tables.push_back(UTF8_TO_U(sName));
			}
		}
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
			throw std::runtime_error(std::string("SqliteEngine::GetTableNames step failed: ") + sqlite3_errmsg(m_db));

		return tables;
	}

	TableSchema SqliteEngine::GetTableSchema(const std::wstring& tableName)
	{
		TableSchema schema;
		if (!m_db) return schema;

		std::string sTableName = U_TO_UTF8(tableName);
		std::string sql = "PRAGMA table_info(" + QuoteIdentifier(sTableName) + ")";
		
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

		std::string fkSql = "PRAGMA foreign_key_list(" + QuoteIdentifier(sTableName) + ")";
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

		// PR #116 review (5388569346): `table_info` (and so schema.columns)
		// omits generated columns, but `SELECT *` includes them -- that
		// shifts every column after a generated one, so e.g. a trailing
		// column silently receives the generated column's value instead of
		// its own. Build the same ordered, quoted column list `table_info`
		// produced for schema.columns instead of using `SELECT *`.
		std::vector<std::string> columnNames;
		{
			std::string pragmaSql = "PRAGMA table_info(" + QuoteIdentifier(sTableName) + ")";
			sqlite3_stmt* pragmaStmt = nullptr;
			if (sqlite3_prepare_v2(m_db, pragmaSql.c_str(), -1, &pragmaStmt, nullptr) != SQLITE_OK)
				return nullptr;

			int rc;
			while ((rc = sqlite3_step(pragmaStmt)) == SQLITE_ROW)
			{
				const unsigned char* colName = sqlite3_column_text(pragmaStmt, 1);
				if (colName)
					columnNames.push_back((const char*)colName);
			}
			sqlite3_finalize(pragmaStmt);
			if (rc != SQLITE_DONE)
				return nullptr;
		}
		if (columnNames.empty())
			return nullptr;

		std::string sql = "SELECT ";
		for (size_t i = 0; i < columnNames.size(); ++i)
		{
			if (i > 0) sql += ", ";
			sql += QuoteIdentifier(columnNames[i]);
		}
		sql += " FROM " + QuoteIdentifier(sTableName);

		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			return std::unique_ptr<IDBResultSet>(new SqliteResultSet(stmt));
		}

		return nullptr;
	}
}
