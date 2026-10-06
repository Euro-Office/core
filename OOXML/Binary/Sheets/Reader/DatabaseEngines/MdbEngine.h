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
