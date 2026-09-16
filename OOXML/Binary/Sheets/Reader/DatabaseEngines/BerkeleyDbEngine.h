#pragma once

#include "IDatabaseEngine.h"
#include <db.h>
#include <string>
#include <vector>

namespace NExtractTools
{
	// Berkeley DB is a schema-less key/value store: there are no typed
	// columns, tables, or foreign keys, so every "table" is exposed with a
	// fixed synthetic {Key, Value} schema (see GetTableSchema()).
	class BerkeleyDbResultSet : public IDBResultSet
	{
	public:
		BerkeleyDbResultSet(DBC* cursor);
		virtual ~BerkeleyDbResultSet();

		virtual bool Next() override;
		virtual std::wstring GetString(int columnIdx) override;

	private:
		DBC* m_cursor;
		DBT m_key;
		DBT m_data;
		bool m_hasRow;
	};

	class BerkeleyDbEngine : public IDatabaseEngine
	{
	public:
		BerkeleyDbEngine();
		virtual ~BerkeleyDbEngine();

		virtual bool Open(const std::wstring& path) override;
		virtual std::vector<std::wstring> GetTableNames() override;
		virtual TableSchema GetTableSchema(const std::wstring& tableName) override;
		virtual std::unique_ptr<IDBResultSet> QueryTable(const std::wstring& tableName) override;

	private:
		DB* m_db;
		std::wstring m_defaultTableName;
	};
}
