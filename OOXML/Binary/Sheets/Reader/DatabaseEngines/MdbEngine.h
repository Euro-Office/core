#pragma once

#include "IDatabaseEngine.h"
#include <mdbtools.h>
#include <string>
#include <vector>

class MdbResultSet : public IDBResultSet
{
public:
	MdbResultSet(MdbTableDef* table);
	virtual ~MdbResultSet();

	bool Next() override;
	std::wstring GetString(int columnIdx) override;

private:
	MdbTableDef* m_table;
};

class MdbEngine : public IDatabaseEngine
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
};
