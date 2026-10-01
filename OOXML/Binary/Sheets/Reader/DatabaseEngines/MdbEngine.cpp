#include "MdbEngine.h"
#include "../../../../../DesktopEditor/common/File.h"
#include <string.h>
#include <stdlib.h>
#include <algorithm>

static std::wstring utf8_to_wstring(const char* str)
{
	if (!str) return L"";
	std::string s(str);
	return UTF8_TO_U(s);
}

namespace
{
	// index_type == 1 denotes a primary-key index in libmdb (see
	// libmdb/index.c's mdb_read_indices()/mdb_index_dump()).
	const unsigned char kMdbPrimaryKeyIndexType = 1;

	std::wstring FindColumnNameByColNum(MdbTableDef* table, int colNum)
	{
		for (unsigned int i = 0; i < table->num_cols; ++i) {
			MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, i);
			if (col->col_num == colNum)
				return utf8_to_wstring(col->name);
		}
		return L"";
	}

	// Populates schema.primaryKeys from the table's primary-key index, if any
	// (FR-011). mdbtools has no higher-level "give me the PK columns" call --
	// this is the same index_type==1 check libmdb's own dump tooling uses.
	void PopulatePrimaryKeys(MdbTableDef* table, TableSchema& schema)
	{
		GPtrArray* indices = mdb_read_indices(table);
		if (!indices)
			return;

		for (unsigned int i = 0; i < table->num_idxs; ++i) {
			MdbIndex *idx = (MdbIndex *)g_ptr_array_index(table->indices, i);
			if (idx->index_type != kMdbPrimaryKeyIndexType)
				continue;

			for (unsigned int k = 0; k < idx->num_keys; ++k) {
				std::wstring colName = FindColumnNameByColNum(table, idx->key_col_num[k]);
				if (colName.empty())
					continue;
				if (std::find(schema.primaryKeys.begin(), schema.primaryKeys.end(), colName) == schema.primaryKeys.end())
					schema.primaryKeys.push_back(colName);
			}
		}
	}

	// Populates schema.foreignKeys for the given table by reading the hidden
	// MSysRelationships system table (FR-011). mdbtools exposes no FK API
	// directly; MSysRelationships is the same source Access itself uses, and
	// is the standard fallback used by other MDB-reading tools.
	void PopulateForeignKeys(MdbHandle* mdb, const std::wstring& tableName, TableSchema& schema)
	{
		MdbTableDef* relTable = mdb_read_table_by_name(mdb, (gchar*)"MSysRelationships", MDB_TABLE);
		if (!relTable || !relTable->num_rows)
			return;
		if (!mdb_read_columns(relTable)) {
			mdb_free_tabledef(relTable);
			return;
		}

		char childColumn[MDB_BIND_SIZE] = { 0 };
		char childTable[MDB_BIND_SIZE] = { 0 };
		char parentColumn[MDB_BIND_SIZE] = { 0 };
		char parentTable[MDB_BIND_SIZE] = { 0 };

		mdb_bind_column_by_name(relTable, (gchar*)"szColumn", childColumn, NULL);
		mdb_bind_column_by_name(relTable, (gchar*)"szObject", childTable, NULL);
		mdb_bind_column_by_name(relTable, (gchar*)"szReferencedColumn", parentColumn, NULL);
		mdb_bind_column_by_name(relTable, (gchar*)"szReferencedObject", parentTable, NULL);
		mdb_rewind_table(relTable);

		std::string narrowTableName = U_TO_UTF8(tableName);
		while (mdb_fetch_row(relTable)) {
			if (narrowTableName != childTable)
				continue;

			ForeignKeyDef fk;
			fk.columnName = utf8_to_wstring(childColumn);
			fk.referencedTable = utf8_to_wstring(parentTable);
			fk.referencedColumn = utf8_to_wstring(parentColumn);
			schema.foreignKeys.push_back(fk);
		}

		mdb_free_tabledef(relTable);
	}
}

MdbResultSet::MdbResultSet(MdbTableDef* table) : m_table(table)
{
	// mdb_read_columns() unconditionally reallocates table->columns, leaking
	// the previous array's MdbColumn entries if called twice on the same
	// table -- skip it when the caller (MdbEngine::GetTableSchema, under the
	// FR-012 single-read cache) already populated columns for us.
	if (!m_table->columns)
		mdb_read_columns(m_table);
	for (unsigned int i = 0; i < m_table->num_cols; ++i) {
		MdbColumn *col = (MdbColumn *)g_ptr_array_index(m_table->columns, i);
		col->bind_ptr = malloc(MDB_BIND_SIZE);
		col->len_ptr = (int *)malloc(sizeof(int));
		memset(col->bind_ptr, 0, MDB_BIND_SIZE);
		*col->len_ptr = 0;
	}
	mdb_rewind_table(m_table);
}

MdbResultSet::~MdbResultSet()
{
	if (m_table) {
		for (unsigned int i = 0; i < m_table->num_cols; ++i) {
			MdbColumn *col = (MdbColumn *)g_ptr_array_index(m_table->columns, i);
			free(col->bind_ptr);
			free(col->len_ptr);
			col->bind_ptr = nullptr;
			col->len_ptr = nullptr;
		}
		mdb_free_tabledef(m_table);
	}
}

bool MdbResultSet::Next()
{
	return mdb_fetch_row(m_table) != 0;
}

std::wstring MdbResultSet::GetString(int columnIdx)
{
	if (columnIdx < 0 || columnIdx >= (int)m_table->num_cols) return L"";
	MdbColumn *col = (MdbColumn *)g_ptr_array_index(m_table->columns, columnIdx);
	
	if (col->col_type == MDB_OLE || col->col_type == MDB_BINARY) {
		return L"[Binary Data]";
	}
	
	return utf8_to_wstring((const char*)col->bind_ptr);
}

MdbEngine::MdbEngine() : m_mdb(nullptr) {}

MdbEngine::~MdbEngine()
{
	for (auto& entry : m_pendingTables) {
		if (entry.second)
			mdb_free_tabledef(entry.second);
	}
	if (m_mdb) {
		mdb_close(m_mdb);
	}
}

bool MdbEngine::Open(const std::wstring& path)
{
	std::string narrowPath = U_TO_UTF8(path);
	
	m_mdb = mdb_open(narrowPath.c_str(), MDB_NOFLAGS);
	return m_mdb != nullptr;
}

std::vector<std::wstring> MdbEngine::GetTableNames()
{
	std::vector<std::wstring> result;
	if (!m_mdb) return result;

	mdb_read_catalog(m_mdb, MDB_TABLE);
	for (unsigned int i = 0; i < m_mdb->num_catalog; ++i) {
		MdbCatalogEntry *entry = (MdbCatalogEntry *)g_ptr_array_index(m_mdb->catalog, i);
		if (entry->object_type == MDB_TABLE) {
			if (mdb_is_user_table(entry)) {
				result.push_back(utf8_to_wstring(entry->object_name));
			}
		}
	}
	return result;
}

TableSchema MdbEngine::GetTableSchema(const std::wstring& tableName)
{
	TableSchema schema;
	if (!m_mdb) return schema;

	// Reuse an already-read table for this name if one is pending (shouldn't
	// normally happen -- GetTableSchema is expected to be called once per
	// table -- but avoids a redundant read if it is called twice).
	MdbTableDef *table = nullptr;
	auto pending = m_pendingTables.find(tableName);
	if (pending != m_pendingTables.end()) {
		table = pending->second;
	} else {
		std::string narrowName = U_TO_UTF8(tableName);
		MdbCatalogEntry *entry = mdb_get_catalogentry_by_name(m_mdb, (char*)narrowName.c_str());
		if (!entry) return schema;

		table = mdb_read_table(entry);
		if (!table) return schema;
	}

	mdb_read_columns(table);
	for (unsigned int i = 0; i < table->num_cols; ++i) {
		MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, i);
		schema.columns.push_back(utf8_to_wstring(col->name));
	}

	PopulatePrimaryKeys(table, schema);
	PopulateForeignKeys(m_mdb, tableName, schema);

	// Keep the table open for QueryTable() to reuse instead of re-reading it
	// from disk (FR-012).
	m_pendingTables[tableName] = table;
	return schema;
}

std::unique_ptr<IDBResultSet> MdbEngine::QueryTable(const std::wstring& tableName)
{
	if (!m_mdb) return nullptr;

	auto pending = m_pendingTables.find(tableName);
	if (pending != m_pendingTables.end()) {
		MdbTableDef *table = pending->second;
		m_pendingTables.erase(pending); // MdbResultSet now owns it
		return std::unique_ptr<IDBResultSet>(new MdbResultSet(table));
	}

	std::string narrowName = U_TO_UTF8(tableName);

	MdbCatalogEntry *entry = mdb_get_catalogentry_by_name(m_mdb, (char*)narrowName.c_str());
	if (!entry) return nullptr;

	MdbTableDef *table = mdb_read_table(entry);
	if (!table) return nullptr;

	return std::unique_ptr<IDBResultSet>(new MdbResultSet(table));
}
