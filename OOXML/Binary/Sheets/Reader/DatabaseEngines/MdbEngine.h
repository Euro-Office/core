#pragma once

#include "IDatabaseEngine.h"
#include "mdbtools/mdbtools.h"
#include <string>
#include <vector>
#include <map>
#include "../../../../../DesktopEditor/common/base_export.h"

class Q_DECL_EXPORT MdbResultSet : public IDBResultSet
{
public:
	MdbResultSet(MdbTableDef* table);
	virtual ~MdbResultSet();

	bool Next() override;
	std::wstring GetString(int columnIdx) override;

private:
	MdbTableDef* m_table;
};

class Q_DECL_EXPORT MdbEngine : public IDatabaseEngine
{
public:
	MdbEngine();
	virtual ~MdbEngine();

	bool Open(const std::wstring& path) override;
	std::vector<std::wstring> GetTableNames() override;
	TableSchema GetTableSchema(const std::wstring& tableName) override;
	std::unique_ptr<IDBResultSet> QueryTable(const std::wstring& tableName) override;

private:
	MdbHandle* m_mdb;

	// Table read by GetTableSchema() and not yet claimed by a matching
	// QueryTable() call, keyed by table name (FR-012: a table must not be
	// read from disk twice across a GetTableSchema+QueryTable pair).
	// QueryTable() takes ownership (erases the entry) when it finds a match;
	// anything left here at destruction time is freed by the destructor.
	std::map<std::wstring, MdbTableDef*> m_pendingTables;
};
