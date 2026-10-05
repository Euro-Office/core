#pragma once

#include "IDatabaseEngine.h"
#include <sqlite3.h>
#include "../../../../../DesktopEditor/common/base_export.h"

namespace NExtractTools
{
	class Q_DECL_EXPORT SqliteResultSet : public IDBResultSet
	{
	public:
		SqliteResultSet(sqlite3_stmt* stmt);
		virtual ~SqliteResultSet();

		virtual bool Next() override;
		virtual std::wstring GetString(int columnIdx) override;

	private:
		sqlite3_stmt* m_stmt;
	};

	class Q_DECL_EXPORT SqliteEngine : public IDatabaseEngine
	{
	public:
		SqliteEngine();
		virtual ~SqliteEngine();

		virtual bool Open(const std::wstring& path) override;
		virtual std::vector<std::wstring> GetTableNames() override;
		virtual TableSchema GetTableSchema(const std::wstring& tableName) override;
		virtual std::unique_ptr<IDBResultSet> QueryTable(const std::wstring& tableName) override;

	private:
		sqlite3* m_db;
	};
}
