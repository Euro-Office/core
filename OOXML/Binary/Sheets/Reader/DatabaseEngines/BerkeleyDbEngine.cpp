#include "BerkeleyDbEngine.h"
#include "../../../../../DesktopEditor/common/File.h"
#include <cstring>

namespace NExtractTools
{
	BerkeleyDbResultSet::BerkeleyDbResultSet(DBC* cursor) : m_cursor(cursor), m_hasRow(false)
	{
		memset(&m_key, 0, sizeof(DBT));
		memset(&m_data, 0, sizeof(DBT));
	}

	BerkeleyDbResultSet::~BerkeleyDbResultSet()
	{
		if (m_cursor)
		{
			m_cursor->close(m_cursor);
			m_cursor = nullptr;
		}
	}

	bool BerkeleyDbResultSet::Next()
	{
		if (!m_cursor)
		{
			m_hasRow = false;
			return false;
		}

		memset(&m_key, 0, sizeof(DBT));
		memset(&m_data, 0, sizeof(DBT));
		m_hasRow = (m_cursor->get(m_cursor, &m_key, &m_data, DB_NEXT) == 0);
		return m_hasRow;
	}

	std::wstring BerkeleyDbResultSet::GetString(int columnIdx)
	{
		if (!m_hasRow) return L"";

		const DBT* pField = (columnIdx == 0) ? &m_key : &m_data;
		if (!pField->data || pField->size == 0) return L"";

		std::string sVal((const char*)pField->data, pField->size);
		return UTF8_TO_U(sVal);
	}

	BerkeleyDbEngine::BerkeleyDbEngine() : m_db(nullptr)
	{
	}

	BerkeleyDbEngine::~BerkeleyDbEngine()
	{
		if (m_db)
		{
			m_db->close(m_db, 0);
			m_db = nullptr;
		}
	}

	bool BerkeleyDbEngine::Open(const std::wstring& path)
	{
		if (db_create(&m_db, nullptr, 0) != 0)
		{
			m_db = nullptr;
			return false;
		}

		std::string sPath = U_TO_UTF8(path);
		// DB_UNKNOWN auto-detects the on-disk access method (Btree/Hash/Queue/Recno).
		if (m_db->open(m_db, nullptr, sPath.c_str(), nullptr, DB_UNKNOWN, DB_RDONLY, 0) != 0)
		{
			m_db->close(m_db, 0);
			m_db = nullptr;
			return false;
		}

		std::wstring::size_type nSlash = path.find_last_of(L"/\\");
		std::wstring sFileName = (nSlash == std::wstring::npos) ? path : path.substr(nSlash + 1);
		std::wstring::size_type nDot = sFileName.rfind(L'.');
		m_defaultTableName = (nDot == std::wstring::npos) ? sFileName : sFileName.substr(0, nDot);

		return true;
	}

	std::vector<std::wstring> BerkeleyDbEngine::GetTableNames()
	{
		std::vector<std::wstring> tables;
		if (m_db)
			tables.push_back(m_defaultTableName);
		return tables;
	}

	TableSchema BerkeleyDbEngine::GetTableSchema(const std::wstring& tableName)
	{
		// Berkeley DB is a schema-less key/value store: expose a fixed
		// synthetic schema with no primary/foreign keys.
		TableSchema schema;
		schema.columns.push_back(L"Key");
		schema.columns.push_back(L"Value");
		return schema;
	}

	std::unique_ptr<IDBResultSet> BerkeleyDbEngine::QueryTable(const std::wstring& tableName)
	{
		if (!m_db) return nullptr;

		DBC* cursor = nullptr;
		if (m_db->cursor(m_db, nullptr, &cursor, 0) != 0 || !cursor)
			return nullptr;

		return std::unique_ptr<IDBResultSet>(new BerkeleyDbResultSet(cursor));
	}
}
