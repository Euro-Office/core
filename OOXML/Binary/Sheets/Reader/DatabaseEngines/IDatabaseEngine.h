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
    // PR #116 review (5415411907): identifies which FK components belong to
    // the *same* multi-column constraint (e.g. SQLite's PRAGMA
    // foreign_key_list "id" column, or an MDB relationship's szRelationship
    // name), scoped to the owning table. Two single-column FKs that merely
    // reference the same table are NOT the same constraint and must not be
    // merged -- an engine that cannot distinguish them should leave this
    // empty; DatabaseReader then treats each such entry as its own group
    // rather than guessing they belong together.
    std::wstring groupKey;
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
