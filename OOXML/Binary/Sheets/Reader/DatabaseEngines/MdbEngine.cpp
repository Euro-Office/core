#include "MdbEngine.h"
#include "../../../../../DesktopEditor/common/File.h"
#include <string.h>
#include <stdlib.h>

static std::wstring utf8_to_wstring(const char* str)
{
	if (!str) return L"";
	std::string s(str);
	return UTF8_TO_U(s);
}

MdbResultSet::MdbResultSet(MdbTableDef* table) : m_table(table)
{
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

	std::string narrowName = U_TO_UTF8(tableName);

	MdbCatalogEntry *entry = mdb_get_catalogentry_by_name(m_mdb, (char*)narrowName.c_str());
	if (!entry) return schema;

	MdbTableDef *table = mdb_read_table(entry);
	if (!table) return schema;

	mdb_read_columns(table);
	for (unsigned int i = 0; i < table->num_cols; ++i) {
		MdbColumn *col = (MdbColumn *)g_ptr_array_index(table->columns, i);
		schema.columns.push_back(utf8_to_wstring(col->name));
	}
	
	// Try to get foreign keys if possible in mdbtools (simplified: none for now, wait: is there an API?)
	// mdbtools doesn't expose FKs nicely, but maybe we can parse indexes.
	// We will skip FK extraction for MDB for now, or mock it if needed.

	mdb_free_tabledef(table);
	return schema;
}

std::unique_ptr<IDBResultSet> MdbEngine::QueryTable(const std::wstring& tableName)
{
	if (!m_mdb) return nullptr;

	std::string narrowName = U_TO_UTF8(tableName);

	MdbCatalogEntry *entry = mdb_get_catalogentry_by_name(m_mdb, (char*)narrowName.c_str());
	if (!entry) return nullptr;

	MdbTableDef *table = mdb_read_table(entry);
	if (!table) return nullptr;

	return std::unique_ptr<IDBResultSet>(new MdbResultSet(table));
}
