#pragma once

#include <string>
#include <vector>
#include <memory>

class IDBResultSet
{
public:
	virtual ~IDBResultSet() {}
	virtual bool Next() = 0;
	virtual std::wstring GetString(int columnIdx) = 0;
};

struct ForeignKeyDef {
    std::wstring columnName;
    std::wstring referencedTable;
    std::wstring referencedColumn;
};

struct TableSchema {
    std::vector<std::wstring> columns;
    std::vector<ForeignKeyDef> foreignKeys;
    std::vector<std::wstring> primaryKeys;
};

class IDatabaseEngine
{
public:
	virtual ~IDatabaseEngine() {}
	virtual bool Open(const std::wstring& path) = 0;
	virtual std::vector<std::wstring> GetTableNames() = 0;
	virtual TableSchema GetTableSchema(const std::wstring& tableName) = 0;
	virtual std::unique_ptr<IDBResultSet> QueryTable(const std::wstring& tableName) = 0;
};
