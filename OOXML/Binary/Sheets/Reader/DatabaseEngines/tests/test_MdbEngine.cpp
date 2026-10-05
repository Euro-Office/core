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

#include "gtest/gtest.h"

#include "../MdbEngine.h"
#include "../../../../../../DesktopEditor/common/File.h"

#include <algorithm>

namespace
{
	// No Access/Jet write API exists in mdbtools (it is a read-only
	// implementation), so this fixture cannot be synthesized at test time
	// the way the SQLite/DuckDB fixtures are -- it must be a real .mdb file
	// checked in here. None is committed yet (see tests/fixtures/README).
	// Verified ad-hoc in the implementing session against a real Access
	// sample database (Northwind, Orders/Customers/Employees/"Order
	// Details"/Products tables; composite PK on "Order Details" came back
	// correctly as OrderID+ProductID) -- that run caught a real bug: the
	// first implementation matched idx->key_col_num[k] against
	// MdbColumn::col_num, which is wrong; libmdb's own code
	// (libmdb/index.c) always uses key_col_num[k]-1 as a direct 1-based
	// array index into table->columns. Fixed in MdbEngine.cpp accordingly.
	std::wstring FixturePath()
	{
		return L"OOXML/Binary/Sheets/Reader/DatabaseEngines/tests/fixtures/sample_with_relationships.mdb";
	}
}

class MdbEngineSchemaTest : public testing::Test
{
protected:
	void SetUp() override
	{
		if (!NSFile::CFileBinary::Exists(FixturePath()))
			GTEST_SKIP() << "Fixture " << U_TO_UTF8(FixturePath())
			             << " not present -- mdbtools has no write API, so this "
			             << "fixture must be a committed real .mdb file, not yet added.";
	}
};

// FR-011: primary keys must be populated from the source database's real
// index metadata.
TEST_F(MdbEngineSchemaTest, PrimaryKeyIsPopulated)
{
	MdbEngine engine;
	ASSERT_TRUE(engine.Open(FixturePath()));
	engine.GetTableNames(); // reads the catalog; GetTableSchema/QueryTable require it, matching DatabaseReader::Read's real call order

	TableSchema schema = engine.GetTableSchema(L"Orders");
	ASSERT_FALSE(schema.primaryKeys.empty());
	EXPECT_NE(std::find(schema.primaryKeys.begin(), schema.primaryKeys.end(), L"OrderID"), schema.primaryKeys.end());
}

// FR-011: foreign keys must be populated by reading MSysRelationships.
TEST_F(MdbEngineSchemaTest, ForeignKeyIsPopulated)
{
	MdbEngine engine;
	ASSERT_TRUE(engine.Open(FixturePath()));
	engine.GetTableNames(); // reads the catalog; GetTableSchema/QueryTable require it, matching DatabaseReader::Read's real call order

	TableSchema schema = engine.GetTableSchema(L"Orders");
	bool foundCustomerFk = false;
	for (const auto& fk : schema.foreignKeys) {
		if (fk.columnName == L"CustomerID" && fk.referencedTable == L"Customers" && fk.referencedColumn == L"CustomerID")
			foundCustomerFk = true;
	}
	EXPECT_TRUE(foundCustomerFk);
}

// FR-012: GetTableSchema followed by QueryTable for the same table must not
// read the table from disk twice.
TEST_F(MdbEngineSchemaTest, SchemaAndQueryShareOneRead)
{
	MdbEngine engine;
	ASSERT_TRUE(engine.Open(FixturePath()));
	engine.GetTableNames(); // reads the catalog; GetTableSchema/QueryTable require it, matching DatabaseReader::Read's real call order

	TableSchema schema = engine.GetTableSchema(L"Orders");
	// If QueryTable re-read the table independently, this would still work
	// functionally, so the real assertion here is structural: the fix
	// (MdbEngine.h's m_pendingTables map) exists specifically so the
	// MdbTableDef* obtained above is reused, not re-fetched, by the call
	// below. There is no public API to directly observe "was it re-read,"
	// so this test exercises the documented contract (schema then query
	// succeeds without a crash or empty result) and relies on code review
	// of MdbEngine.cpp for the single-read guarantee itself.
	std::unique_ptr<IDBResultSet> rs = engine.QueryTable(L"Orders");
	ASSERT_TRUE(rs);
	EXPECT_TRUE(rs->Next());
}
