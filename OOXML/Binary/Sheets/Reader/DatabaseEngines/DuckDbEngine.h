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

#pragma once

#include "IDatabaseEngine.h"
#include <duckdb.h>
#include "../../../../../DesktopEditor/common/base_export.h"

namespace NExtractTools
{
	// Streams the result one chunk at a time via duckdb_fetch_chunk (FR-015,
	// FR-016) instead of the deprecated duckdb_row_count/duckdb_value_varchar
	// whole-result accessors. QueryTable() casts every column to VARCHAR in
	// SQL, so every vector this class reads is guaranteed to be VARCHAR --
	// no per-duckdb_type dispatch is needed to stringify arbitrary column
	// types.
	class Q_DECL_EXPORT DuckDbResultSet : public IDBResultSet
	{
	public:
		DuckDbResultSet(duckdb_result result);
		virtual ~DuckDbResultSet();

		virtual bool Next() override;
		virtual std::wstring GetString(int columnIdx) override;

	private:
		duckdb_result m_result;
		duckdb_data_chunk m_chunk = nullptr;
		idx_t m_chunkSize = 0;
		idx_t m_rowInChunk = 0;
	};

	class Q_DECL_EXPORT DuckDbEngine : public IDatabaseEngine
	{
	public:
		DuckDbEngine();
		virtual ~DuckDbEngine();

		virtual bool Open(const std::wstring& path) override;
		virtual std::vector<std::wstring> GetTableNames() override;
		virtual TableSchema GetTableSchema(const std::wstring& tableName) override;
		virtual std::unique_ptr<IDBResultSet> QueryTable(const std::wstring& tableName) override;

	private:
		duckdb_database m_db;
		duckdb_connection m_conn;
	};
}
