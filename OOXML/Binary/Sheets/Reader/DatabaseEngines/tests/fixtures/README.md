# Test fixtures

`sample_with_relationships.mdb` — referenced by `test_MdbEngine.cpp`'s
`MdbEngineSchemaTest` suite. Not yet committed: mdbtools has no write API,
so this must be a real Access database file, and none small/license-clean
enough was settled on during implementation. Needs an `Orders` table with
a primary key and a foreign key to a `Customers.CustomerID` column (or the
test updated to match whatever fixture is added).

Until this file is added, `MdbEngineSchemaTest`'s cases report `SKIPPED`,
not failed.
