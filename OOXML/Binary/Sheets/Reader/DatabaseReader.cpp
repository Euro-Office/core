#define DONT_WRITE_EMBEDDED_FONTS
#include "DatabaseReader.h"
#include "../../../../Common/ATLDefine.h"
#include "../../../../DesktopEditor/common/Path.h"
#include "../../../../DesktopEditor/common/Directory.h"
#include "../../../DocxFormat/Math/oMathPara.h"
#include "../../../DocxFormat/Math/oMathContent.h"
#include "../../../DocxFormat/Drawing/DrawingExt.h"
#include "../../../XlsxFormat/ComplexTypes_Spreadsheet.h"
#include "../../../XlsxFormat/Styles/Styles.h"
#include "../../../XlsxFormat/Styles/Borders.h"
#include "../../../XlsxFormat/Styles/Fills.h"
#include "../../../XlsxFormat/Styles/Fonts.h"
#include "../../../XlsxFormat/Styles/NumFmts.h"
#include "../../../XlsxFormat/Styles/Xfs.h"
#include "../../../XlsxFormat/Styles/dxf.h"
#include "../../../XlsxFormat/Styles/CellStyles.h"
#include "../../../XlsxFormat/Styles/TableStyles.h"
#include "CellFormatController/CellFormatController.h"
#include "../../../XlsxFormat/Xlsx.h"
#include "../../../XlsxFormat/Worksheets/Worksheet.h"
#include "../../../../DesktopEditor/common/File.h"
#include "../../../XlsxFormat/Workbook/Workbook.h"
#include "../../../XlsxFormat/SharedStrings/SharedStrings.h"
#include "../../../XlsxFormat/Styles/Styles.h"
#include "DatabaseEngines/MdbEngine.h"
#include "DatabaseEngines/SqliteEngine.h"
#include "DatabaseEngines/DuckDbEngine.h"
#include "DatabaseEngines/BerkeleyDbEngine.h"
#include "../../../XlsxFormat/Worksheets/DataValidation.h"
#include "../../../../Common/OfficeFileErrorDescription.h"
#include "../../../../Common/DatabaseFormats.h"
#include "../../../../Common/OfficeFileFormats.h"
#include <map>
#include <set>
#include <tuple>
#include <algorithm>
#include <cwctype>
#include <stdexcept>

using namespace NExtractTools;

namespace
{
	// 1,048,576 total rows per XLSX sheet, minus one for the header row (FR-007).
	const size_t kMaxDataRowsPerSheet = 1048575;

	// Sanitizes a candidate sheet name per Excel's rules (FR-006): at most 31
	// characters, none of / \ ? * [ ], and unique within the workbook (Excel
	// compares sheet names case-insensitively, so uniqueness is tracked that
	// way too). usedNames persists across calls so every sheet in the workbook
	// -- summary, constraints, and every table's segments -- is deduplicated
	// against every other one.
	std::wstring SanitizeSheetName(const std::wstring& raw, std::set<std::wstring>& usedNamesLower)
	{
		std::wstring sanitized;
		for (wchar_t ch : raw)
		{
			// PR #116 review (5391562282): ':' is also forbidden in an Excel
			// sheet title -- filter it here too, before collision handling,
			// since e.g. "Alpha:Beta" and "AlphaBeta" would otherwise
			// sanitize to the same name.
			if (ch == L'/' || ch == L'\\' || ch == L'?' || ch == L'*' || ch == L'[' || ch == L']' || ch == L':')
				continue;
			sanitized.push_back(ch);
		}
		if (sanitized.empty())
			sanitized = L"Sheet";
		if (sanitized.size() > 31)
			sanitized.resize(31);

		auto toLower = [](std::wstring s) {
			std::transform(s.begin(), s.end(), s.begin(), ::towlower);
			return s;
		};

		std::wstring candidate = sanitized;
		std::wstring candidateLower = toLower(candidate);
		int suffix = 2;
		while (usedNamesLower.find(candidateLower) != usedNamesLower.end())
		{
			std::wstring suffixStr = L"_" + std::to_wstring(suffix++);
			size_t maxBaseLen = suffixStr.size() >= 31 ? 0 : (31 - suffixStr.size());
			candidate = sanitized.substr(0, std::min(sanitized.size(), maxBaseLen)) + suffixStr;
			candidateLower = toLower(candidate);
		}
		usedNamesLower.insert(candidateLower);
		return candidate;
	}

	std::wstring GetColLetter(int colIdx)
	{
		std::wstring res;
		while (colIdx >= 0) {
			res.insert(res.begin(), (wchar_t)(L'A' + (colIdx % 26)));
			colIdx = (colIdx / 26) - 1;
		}
		return res;
	}
}

DatabaseReader::DatabaseReader() {}
DatabaseReader::~DatabaseReader() {}

_UINT32 DatabaseReader::Read(const std::wstring &sFileName, OOX::Spreadsheet::CXlsx &oXlsx, _INT32 lcid, bool readToCache)
{
	oXlsx.CreateWorkbook();
	oXlsx.CreateStyles();
	if (!oXlsx.m_pSharedStrings)
	{
		oXlsx.CreateSharedStrings();
	}
	
	std::shared_ptr<CellFormatController> cellFormatController = std::make_shared<CellFormatController>(oXlsx.m_pStyles, lcid);

	// PR #116 review (5388969387): ProcessCellType() returns 1 when a cell's
	// text exceeds Excel's 32,767-character limit and gets truncated. Latch
	// that here (as CSVReader does) instead of discarding it, so Read()
	// reports AVS_FILEUTILS_ERROR_CONVERT_CELLLIMITS rather than S_OK.
	bool bMsLimitCell = false;

	std::wstring sExt;
	std::wstring::size_type nExtPos = sFileName.rfind(L'.');
	if (nExtPos != std::wstring::npos) {
		sExt = sFileName.substr(nExtPos);
		std::transform(sExt.begin(), sExt.end(), sExt.begin(), ::tolower);
	}

	int nFormat = AVS_OFFICESTUDIO_FILE_UNKNOWN;
	for (const auto& dbEntry : GetSupportedDatabaseExtensions())
	{
		if (sExt != dbEntry.extension)
			continue;
		nFormat = dbEntry.requiresContentSniff ? DetectAmbiguousDbFormat(sFileName) : dbEntry.formatConstant;
		break;
	}

	std::unique_ptr<IDatabaseEngine> engine;
	switch (nFormat)
	{
	case AVS_OFFICESTUDIO_FILE_SPREADSHEET_SQLITE:
		engine.reset(new SqliteEngine());
		break;
	case AVS_OFFICESTUDIO_FILE_SPREADSHEET_DUCKDB:
	case AVS_OFFICESTUDIO_FILE_SPREADSHEET_PARQUET:
		engine.reset(new DuckDbEngine());
		break;
	case AVS_OFFICESTUDIO_FILE_SPREADSHEET_MDB:
		engine.reset(new MdbEngine());
		break;
	case AVS_OFFICESTUDIO_FILE_SPREADSHEET_BDB:
		engine.reset(new BerkeleyDbEngine());
		break;
	default:
		return AVS_FILEUTILS_ERROR_CONVERT;
	}

	if (!engine->Open(sFileName)) {
		return AVS_FILEUTILS_ERROR_CONVERT;
	}

	// PR #116 review (5389484472): GetTableNames() can now throw on a
	// discovery error (prepare/step failure) instead of silently returning
	// an empty list indistinguishable from a database with no tables.
	std::vector<std::wstring> tables;
	try {
		tables = engine->GetTableNames();
	} catch (const std::exception&) {
		return AVS_FILEUTILS_ERROR_CONVERT;
	}
	int sheetIndex = 1;

	struct TableMeta {
		std::wstring name;
		TableSchema schema;
		size_t rowCount = 0;
	};
	// A PK column's value at a given row, before the row's eventual sheet
	// name/row number are known (those depend on every table's row count,
	// which this same streamed pass is what discovers them).
	struct PkValue {
		size_t rowIndex;
		int colIdx;
		std::wstring value;
	};

	// Pass 1 (FR-015): stream each table exactly once, column-by-column,
	// to learn its row count and collect only its primary-key column values
	// -- never buffering full row data for every table in memory at once,
	// which is what let memory use scale with total database size before.
	std::vector<TableMeta> allTables;
	std::map<std::wstring, std::vector<PkValue>> pkValuesByTable;

	for (const auto& tableName : tables) {
		TableMeta tmeta;
		tmeta.name = tableName;
		tmeta.schema = engine->GetTableSchema(tableName);

		std::vector<int> pkColIndices;
		for (size_t colIdx = 0; colIdx < tmeta.schema.columns.size(); ++colIdx) {
			if (std::find(tmeta.schema.primaryKeys.begin(), tmeta.schema.primaryKeys.end(), tmeta.schema.columns[colIdx]) != tmeta.schema.primaryKeys.end())
				pkColIndices.push_back((int)colIdx);
		}

		std::vector<PkValue>& pkValues = pkValuesByTable[tableName];
		std::unique_ptr<IDBResultSet> rs = engine->QueryTable(tableName);
		if (rs) {
			while (rs->Next()) {
				for (int colIdx : pkColIndices) {
					PkValue pv;
					pv.rowIndex = tmeta.rowCount;
					pv.colIdx = colIdx;
					pv.value = rs->GetString(colIdx);
					pkValues.push_back(pv);
				}
				tmeta.rowCount++;
			}
		}
		allTables.push_back(tmeta);
	}

	// PR #116 review (5388727348): FK resolution below needs to look up a
	// referenced table's own primary-key column order to match a composite
	// key's full tuple, not just one column at a time.
	std::map<std::wstring, const TableMeta*> tableMetaByName;
	for (const auto& tmeta : allTables)
		tableMetaByName[tmeta.name] = &tmeta;

	// Reserve sheet names and work out how many sheets each table needs
	// (FR-006, FR-007). A table's rows are split into pages of at most
	// kMaxDataRowsPerSheet; segment 1 keeps the table's own (sanitized) name,
	// later segments get "<name>_2", "<name>_3", etc. -- all run through the
	// same sanitizer/dedup pool as every other sheet in the workbook, so a
	// split segment can never collide with another table's name either.
	std::set<std::wstring> usedSheetNamesLower;
	const std::wstring summarySheetName = SanitizeSheetName(L"Migration Summary", usedSheetNamesLower);

	bool anyConstraints = false;
	for (const auto& tmeta : allTables) {
		if (!tmeta.schema.primaryKeys.empty() || !tmeta.schema.foreignKeys.empty()) {
			anyConstraints = true;
			break;
		}
	}
	std::wstring constraintsSheetName;
	if (anyConstraints)
		constraintsSheetName = SanitizeSheetName(L"Constraints", usedSheetNamesLower);

	struct TableLayout {
		std::vector<std::wstring> segmentSheetNames; // one per segment, in order
	};
	std::map<std::wstring, TableLayout> layoutByTable; // keyed by table name (unique per allTables)

	for (const auto& tmeta : allTables) {
		TableLayout layout;
		size_t numSegments = tmeta.rowCount == 0 ? 1 : (tmeta.rowCount + kMaxDataRowsPerSheet - 1) / kMaxDataRowsPerSheet;

		for (size_t seg = 0; seg < numSegments; ++seg) {
			std::wstring rawName = (seg == 0) ? tmeta.name : (tmeta.name + L"_" + std::to_wstring(seg + 1));
			layout.segmentSheetNames.push_back(SanitizeSheetName(rawName, usedSheetNamesLower));
		}

		layoutByTable[tmeta.name] = layout;
	}

	// PR #116 review (5388727348): a composite primary key's components
	// aren't unique individually -- parents (1,10) and (1,20) share a first
	// column value of 1, so resolving a composite FK one column at a time
	// can point a child row at the wrong parent. Key on the full ordered
	// tuple of a row's primary-key values (schema.primaryKeys order) instead
	// of one (table, column, value) triple at a time (no string
	// concatenation, so FR-013's '.'-collision concern still doesn't apply).
	std::map<std::wstring, std::map<std::vector<std::wstring>, std::vector<std::wstring>>> pkRowMap;
	for (const auto& tmeta : allTables) {
		if (tmeta.schema.primaryKeys.empty())
			continue;
		const TableLayout& layout = layoutByTable[tmeta.name];

		// Regroup this table's per-(row,column) PK values back into one
		// (value tuple, cellRef tuple) pair per row.
		std::map<size_t, std::map<int, std::wstring>> valuesByRowThenCol;
		for (const PkValue& pv : pkValuesByTable[tmeta.name])
			valuesByRowThenCol[pv.rowIndex][pv.colIdx] = pv.value;

		for (const auto& rowEntry : valuesByRowThenCol) {
			size_t rowIndex = rowEntry.first;
			size_t seg = rowIndex / kMaxDataRowsPerSheet;
			size_t rowNumberInSheet = (rowIndex % kMaxDataRowsPerSheet) + 2; // +1 header, +1 for 1-based
			// PR #116 review (5391755120): a sheet name quoted in a formula
			// reference must have its own embedded apostrophes doubled --
			// "O'Brien" needs '!A2 as 'O''Brien'!A2, or the reference is
			// invalid and the actual worksheet title is lost.
			std::wstring quotedSheetName = layout.segmentSheetNames[seg];
			size_t aposPos = 0;
			while ((aposPos = quotedSheetName.find(L'\'', aposPos)) != std::wstring::npos)
			{
				quotedSheetName.insert(aposPos, L"'");
				aposPos += 2;
			}

			std::vector<std::wstring> valueTuple, cellRefTuple;
			for (const std::wstring& pkCol : tmeta.schema.primaryKeys) {
				auto colIt = std::find(tmeta.schema.columns.begin(), tmeta.schema.columns.end(), pkCol);
				if (colIt == tmeta.schema.columns.end())
					continue;
				int colIdx = (int)std::distance(tmeta.schema.columns.begin(), colIt);
				auto valIt = rowEntry.second.find(colIdx);
				if (valIt == rowEntry.second.end())
					continue;
				valueTuple.push_back(valIt->second);
				cellRefTuple.push_back(L"'" + quotedSheetName + L"'!" + GetColLetter(colIdx) + std::to_wstring(rowNumberInSheet));
			}
			if (valueTuple.size() == tmeta.schema.primaryKeys.size())
				pkRowMap[tmeta.name][valueTuple] = cellRefTuple;
		}
	}
	pkValuesByTable.clear(); // no longer needed once pkRowMap is built

	// Migration Summary sheet (FR-009), written first.
	{
		smart_ptr<OOX::File> oSummaryFile(new OOX::Spreadsheet::CWorksheet(NULL));
		OOX::Spreadsheet::CWorksheet *pSummary = (OOX::Spreadsheet::CWorksheet *)oSummaryFile.GetPointer();
		pSummary->m_oSheetData.Init();
		pSummary->m_oSheetFormatPr.Init();
		pSummary->m_oSheetFormatPr->m_oBaseColWidth = 9;
		cellFormatController->m_pWorksheet = pSummary;

		auto writeLiteralRow = [&](int rowNumber, const std::vector<std::wstring>& values) {
			OOX::Spreadsheet::CRow *pRow = new OOX::Spreadsheet::CRow();
			pRow->m_oR.Init();
			pRow->m_oR->SetValue(rowNumber);
			for (size_t colIdx = 0; colIdx < values.size(); ++colIdx) {
				OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
				pCell->m_oType.Init();
				pCell->setRowCol(rowNumber - 1, colIdx);
				pCell->m_oCacheValue = values[colIdx];
				pCell->m_oType->SetValue(SimpleTypes::Spreadsheet::celltypeInlineStr);
				pCell->m_oRichText.Init();
				OOX::Spreadsheet::CText *pText = new OOX::Spreadsheet::CText();
				pText->m_sText = values[colIdx];
				pCell->m_oRichText->m_arrItems.push_back(pText);
				pRow->m_arrItems.push_back(pCell);
			}
			pSummary->m_oSheetData->m_arrItems.push_back(pRow);
		};

		writeLiteralRow(1, { L"Table", L"Rows", L"Columns", L"Sheets" });
		int summaryRow = 2;
		for (const auto& tmeta : allTables) {
			const TableLayout& layout = layoutByTable[tmeta.name];
			std::wstring sheetsJoined;
			for (size_t i = 0; i < layout.segmentSheetNames.size(); ++i) {
				if (i > 0)
					sheetsJoined += L", ";
				sheetsJoined += layout.segmentSheetNames[i];
			}
			writeLiteralRow(summaryRow++, {
				tmeta.name,
				std::to_wstring(tmeta.rowCount),
				std::to_wstring(tmeta.schema.columns.size()),
				sheetsJoined
			});
		}

		oXlsx.m_arWorksheets.push_back(pSummary);
		const OOX::RId oSummaryRid = oXlsx.m_pWorkbook->Add(oSummaryFile);
		oXlsx.m_mapWorksheets.insert(std::make_pair(oSummaryRid.ToString(), pSummary));

		OOX::Spreadsheet::CSheet *pSummarySheetEntry = new OOX::Spreadsheet::CSheet();
		pSummarySheetEntry->m_oName = summarySheetName;
		pSummarySheetEntry->m_oSheetId.Init();
		pSummarySheetEntry->m_oSheetId->SetValue(sheetIndex++);
		pSummarySheetEntry->m_oRid.Init();
		pSummarySheetEntry->m_oRid->SetValue(oSummaryRid.ToString());
		if (!oXlsx.m_pWorkbook->m_oSheets.IsInit())
			oXlsx.m_pWorkbook->m_oSheets.Init();
		oXlsx.m_pWorkbook->m_oSheets->m_arrItems.push_back(pSummarySheetEntry);
	}

	// Constraints sheet (FR-010), written second, only when at least one
	// primary/foreign key constraint exists anywhere in the source.
	if (anyConstraints) {
		smart_ptr<OOX::File> oConstraintsFile(new OOX::Spreadsheet::CWorksheet(NULL));
		OOX::Spreadsheet::CWorksheet *pConstraints = (OOX::Spreadsheet::CWorksheet *)oConstraintsFile.GetPointer();
		pConstraints->m_oSheetData.Init();
		pConstraints->m_oSheetFormatPr.Init();
		pConstraints->m_oSheetFormatPr->m_oBaseColWidth = 9;
		cellFormatController->m_pWorksheet = pConstraints;

		auto writeLiteralRow = [&](int rowNumber, const std::vector<std::wstring>& values) {
			OOX::Spreadsheet::CRow *pRow = new OOX::Spreadsheet::CRow();
			pRow->m_oR.Init();
			pRow->m_oR->SetValue(rowNumber);
			for (size_t colIdx = 0; colIdx < values.size(); ++colIdx) {
				OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
				pCell->m_oType.Init();
				pCell->setRowCol(rowNumber - 1, colIdx);
				pCell->m_oCacheValue = values[colIdx];
				pCell->m_oType->SetValue(SimpleTypes::Spreadsheet::celltypeInlineStr);
				pCell->m_oRichText.Init();
				OOX::Spreadsheet::CText *pText = new OOX::Spreadsheet::CText();
				pText->m_sText = values[colIdx];
				pCell->m_oRichText->m_arrItems.push_back(pText);
				pRow->m_arrItems.push_back(pCell);
			}
			pConstraints->m_oSheetData->m_arrItems.push_back(pRow);
		};

		writeLiteralRow(1, { L"Table", L"Column", L"ConstraintType", L"ReferencedTable", L"ReferencedColumn" });
		int constraintsRow = 2;
		for (const auto& tmeta : allTables) {
			for (const auto& pkName : tmeta.schema.primaryKeys)
				writeLiteralRow(constraintsRow++, { tmeta.name, pkName, L"PrimaryKey", L"", L"" });
			for (const auto& fk : tmeta.schema.foreignKeys)
				writeLiteralRow(constraintsRow++, { tmeta.name, fk.columnName, L"ForeignKey", fk.referencedTable, fk.referencedColumn });
		}

		oXlsx.m_arWorksheets.push_back(pConstraints);
		const OOX::RId oConstraintsRid = oXlsx.m_pWorkbook->Add(oConstraintsFile);
		oXlsx.m_mapWorksheets.insert(std::make_pair(oConstraintsRid.ToString(), pConstraints));

		OOX::Spreadsheet::CSheet *pConstraintsSheetEntry = new OOX::Spreadsheet::CSheet();
		pConstraintsSheetEntry->m_oName = constraintsSheetName;
		pConstraintsSheetEntry->m_oSheetId.Init();
		pConstraintsSheetEntry->m_oSheetId->SetValue(sheetIndex++);
		pConstraintsSheetEntry->m_oRid.Init();
		pConstraintsSheetEntry->m_oRid->SetValue(oConstraintsRid.ToString());
		if (!oXlsx.m_pWorkbook->m_oSheets.IsInit())
			oXlsx.m_pWorkbook->m_oSheets.Init();
		oXlsx.m_pWorkbook->m_oSheets->m_arrItems.push_back(pConstraintsSheetEntry);
	}

	// Pass 2 (FR-015): re-query each table and stream its rows directly into
	// worksheet cells, rolling over to the next segment sheet every
	// kMaxDataRowsPerSheet rows, instead of writing from an in-memory row
	// buffer. At most one row is held in memory at a time per table. This is
	// a second read of each table (the first was pass 1, above) -- a
	// deliberate trade of one extra read for bounded memory use, per
	// contracts/idatabaseengine-schema.md rule 3 (QueryTable must be safely
	// re-callable for the same table).
	for (const auto& tmeta : allTables) {
		const TableLayout& layout = layoutByTable[tmeta.name];

		size_t seg = 0;
		int rowIndexInSheet = 1; // 1-based index of the next row to write into the current segment sheet
		OOX::Spreadsheet::CWorksheet *pWorksheet = nullptr;
		smart_ptr<OOX::File> oWorksheetFile;

		auto startSegmentSheet = [&]() {
			oWorksheetFile = smart_ptr<OOX::File>(new OOX::Spreadsheet::CWorksheet(NULL));
			pWorksheet = (OOX::Spreadsheet::CWorksheet *)oWorksheetFile.GetPointer();
			pWorksheet->m_oSheetData.Init();
			pWorksheet->m_oSheetFormatPr.Init();
			pWorksheet->m_oSheetFormatPr->m_oBaseColWidth = 9;
			cellFormatController->m_pWorksheet = pWorksheet;

			// Write Headers (repeated on every segment, per FR-007)
			OOX::Spreadsheet::CRow *pHeaderRow = new OOX::Spreadsheet::CRow();
			pHeaderRow->m_oR.Init();
			pHeaderRow->m_oR->SetValue(1);
			for (size_t colIdx = 0; colIdx < tmeta.schema.columns.size(); ++colIdx) {
				OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
				pCell->m_oType.Init();
				pCell->setRowCol(0, colIdx);
				std::wstring colName = tmeta.schema.columns[colIdx];
				pCell->m_oCacheValue = colName;
				// Headers are always literal text: skip ProcessCellType's type inference,
				// which would otherwise turn names like "=1+1" into formulas or "00123" into 123.
				pCell->m_oType->SetValue(SimpleTypes::Spreadsheet::celltypeInlineStr);
				pCell->m_oRichText.Init();
				OOX::Spreadsheet::CText *pText = new OOX::Spreadsheet::CText();
				pText->m_sText = colName;
				pCell->m_oRichText->m_arrItems.push_back(pText);
				pHeaderRow->m_arrItems.push_back(pCell);
			}
			pWorksheet->m_oSheetData->m_arrItems.push_back(pHeaderRow);

			rowIndexInSheet = 1;
		};

		auto finalizeSegmentSheet = [&]() {
			// Data validation for PK uniqueness, scoped to this segment's own rows.
			if (!tmeta.schema.primaryKeys.empty()) {
				if (!pWorksheet->m_oDataValidations.IsInit())
					pWorksheet->m_oDataValidations.Init();

				for (const auto& pkName : tmeta.schema.primaryKeys) {
					auto it = std::find(tmeta.schema.columns.begin(), tmeta.schema.columns.end(), pkName);
					if (it != tmeta.schema.columns.end()) {
						int pkColIdx = std::distance(tmeta.schema.columns.begin(), it);
						std::wstring colLetter = GetColLetter(pkColIdx);

						OOX::Spreadsheet::CDataValidation* pValidation = new OOX::Spreadsheet::CDataValidation();
						pValidation->m_oType.Init();
						pValidation->m_oType->SetValue(SimpleTypes::Spreadsheet::validationTypeCustom);

						pValidation->m_oSqRef = colLetter + L"2:" + colLetter + L"1048576";

						pValidation->m_oFormula1.Init();
						pValidation->m_oFormula1->m_sText = L"COUNTIF(" + colLetter + L":" + colLetter + L", " + colLetter + L"2)<=1";

						pWorksheet->m_oDataValidations->m_arrItems.push_back(pValidation);
					}
				}
			}

			oXlsx.m_arWorksheets.push_back(pWorksheet);
			const OOX::RId oRid = oXlsx.m_pWorkbook->Add(oWorksheetFile);
			oXlsx.m_mapWorksheets.insert(std::make_pair(oRid.ToString(), pWorksheet));

			OOX::Spreadsheet::CSheet *pSheet = new OOX::Spreadsheet::CSheet();
			pSheet->m_oName = layout.segmentSheetNames[seg];
			pSheet->m_oSheetId.Init();
			pSheet->m_oSheetId->SetValue(sheetIndex++);
			pSheet->m_oRid.Init();
			pSheet->m_oRid->SetValue(oRid.ToString());

			if (!oXlsx.m_pWorkbook->m_oSheets.IsInit())
				oXlsx.m_pWorkbook->m_oSheets.Init();
			oXlsx.m_pWorkbook->m_oSheets->m_arrItems.push_back(pSheet);
		};

		startSegmentSheet();

		std::unique_ptr<IDBResultSet> rs = engine->QueryTable(tmeta.name);
		if (rs) {
			while (rs->Next()) {
				if (rowIndexInSheet > (int)kMaxDataRowsPerSheet) {
					finalizeSegmentSheet();
					seg++;
					startSegmentSheet();
				}

				OOX::Spreadsheet::CRow *pRow = new OOX::Spreadsheet::CRow();
				pRow->m_oR.Init();
				pRow->m_oR->SetValue(rowIndexInSheet + 1);

				// Read every column of this row up front (needed below to
				// resolve a composite FK, which must see all of its
				// component columns' values together, not one at a time).
				std::vector<std::wstring> rowVals(tmeta.schema.columns.size());
				for (size_t colIdx = 0; colIdx < tmeta.schema.columns.size(); ++colIdx)
					rowVals[colIdx] = rs->GetString(colIdx);

				// PR #116 review (5388727348): group this row's FK columns
				// by referencedTable and resolve the complete tuple against
				// that table's own primary-key column order, so every
				// component of a composite key must match the same parent
				// row -- not just whichever single column happened to.
				std::map<int, std::wstring> formulaByColIdx;
				{
					std::map<std::wstring, std::vector<const ForeignKeyDef*>> fkGroupsByReferencedTable;
					for (const auto& fk : tmeta.schema.foreignKeys)
						fkGroupsByReferencedTable[fk.referencedTable].push_back(&fk);

					for (const auto& group : fkGroupsByReferencedTable) {
						auto refTableIt = tableMetaByName.find(group.first);
						if (refTableIt == tableMetaByName.end() || refTableIt->second->schema.primaryKeys.empty())
							continue;

						std::vector<std::wstring> valueTuple;
						std::vector<int> fkColIdxInPkOrder;
						bool complete = true;
						for (const std::wstring& pkCol : refTableIt->second->schema.primaryKeys) {
							const ForeignKeyDef* matchingFk = nullptr;
							for (const ForeignKeyDef* fk : group.second) {
								if (fk->referencedColumn == pkCol) { matchingFk = fk; break; }
							}
							if (!matchingFk) { complete = false; break; }
							auto colIt = std::find(tmeta.schema.columns.begin(), tmeta.schema.columns.end(), matchingFk->columnName);
							if (colIt == tmeta.schema.columns.end()) { complete = false; break; }
							int colIdx = (int)std::distance(tmeta.schema.columns.begin(), colIt);
							valueTuple.push_back(rowVals[colIdx]);
							fkColIdxInPkOrder.push_back(colIdx);
						}
						if (!complete)
							continue;

						auto tableRowsIt = pkRowMap.find(group.first);
						if (tableRowsIt == pkRowMap.end())
							continue;
						auto rowMatchIt = tableRowsIt->second.find(valueTuple);
						if (rowMatchIt == tableRowsIt->second.end())
							continue;

						for (size_t i = 0; i < fkColIdxInPkOrder.size(); ++i)
							formulaByColIdx[fkColIdxInPkOrder[i]] = rowMatchIt->second[i];
					}
				}

				for (size_t colIdx = 0; colIdx < tmeta.schema.columns.size(); ++colIdx) {
					const std::wstring& val = rowVals[colIdx];

					OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
					pCell->m_oType.Init();
					pCell->setRowCol(rowIndexInSheet, colIdx);
					pCell->m_oCacheValue = val;

					auto formulaIt = formulaByColIdx.find((int)colIdx);
					if (formulaIt != formulaByColIdx.end()) {
						pCell->m_oFormula.Init();
						pCell->m_oFormula->m_sText = formulaIt->second;
					}

					if (1 == cellFormatController->ProcessCellType(pCell, val, false))
						bMsLimitCell = true;
					pRow->m_arrItems.push_back(pCell);
				}
				pWorksheet->m_oSheetData->m_arrItems.push_back(pRow);
				rowIndexInSheet++;
			}
		}

		finalizeSegmentSheet();
	}

	return bMsLimitCell ? AVS_FILEUTILS_ERROR_CONVERT_CELLLIMITS : 0; // S_OK
}

