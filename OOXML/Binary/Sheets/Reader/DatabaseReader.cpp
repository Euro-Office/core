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
			if (ch == L'/' || ch == L'\\' || ch == L'?' || ch == L'*' || ch == L'[' || ch == L']')
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

	std::vector<std::wstring> tables = engine->GetTableNames();
	int sheetIndex = 1;
	
	struct TableData {
		std::wstring name;
		TableSchema schema;
		std::vector<std::vector<std::wstring>> rows;
	};
	std::vector<TableData> allData;

	for (const auto& tableName : tables) {
		TableData tdata;
		tdata.name = tableName;
		tdata.schema = engine->GetTableSchema(tableName);
		std::unique_ptr<IDBResultSet> rs = engine->QueryTable(tableName);
		if (rs) {
			while (rs->Next()) {
				std::vector<std::wstring> rowData;
				for (size_t colIdx = 0; colIdx < tdata.schema.columns.size(); ++colIdx) {
					rowData.push_back(rs->GetString(colIdx));
				}
				tdata.rows.push_back(rowData);
			}
		}
		allData.push_back(tdata);
	}

	// Reserve sheet names and work out how many sheets each table needs
	// (FR-006, FR-007). A table's rows are split into pages of at most
	// kMaxDataRowsPerSheet; segment 1 keeps the table's own (sanitized) name,
	// later segments get "<name>_2", "<name>_3", etc. -- all run through the
	// same sanitizer/dedup pool as every other sheet in the workbook, so a
	// split segment can never collide with another table's name either.
	std::set<std::wstring> usedSheetNamesLower;
	const std::wstring summarySheetName = SanitizeSheetName(L"Migration Summary", usedSheetNamesLower);

	bool anyConstraints = false;
	for (const auto& tdata : allData) {
		if (!tdata.schema.primaryKeys.empty() || !tdata.schema.foreignKeys.empty()) {
			anyConstraints = true;
			break;
		}
	}
	std::wstring constraintsSheetName;
	if (anyConstraints)
		constraintsSheetName = SanitizeSheetName(L"Constraints", usedSheetNamesLower);

	struct TableLayout {
		std::vector<std::wstring> segmentSheetNames; // one per segment, in order
		std::vector<std::wstring> rowSheetName;      // one per data row
		std::vector<size_t> rowSheetRowNumber;       // 1-based row number within that row's sheet
	};
	std::map<std::wstring, TableLayout> layoutByTable; // keyed by table name (unique per allData)

	for (const auto& tdata : allData) {
		TableLayout layout;
		size_t totalRows = tdata.rows.size();
		size_t numSegments = totalRows == 0 ? 1 : (totalRows + kMaxDataRowsPerSheet - 1) / kMaxDataRowsPerSheet;

		for (size_t seg = 0; seg < numSegments; ++seg) {
			std::wstring rawName = (seg == 0) ? tdata.name : (tdata.name + L"_" + std::to_wstring(seg + 1));
			layout.segmentSheetNames.push_back(SanitizeSheetName(rawName, usedSheetNamesLower));
		}

		layout.rowSheetName.resize(totalRows);
		layout.rowSheetRowNumber.resize(totalRows);
		for (size_t i = 0; i < totalRows; ++i) {
			size_t seg = i / kMaxDataRowsPerSheet;
			layout.rowSheetName[i] = layout.segmentSheetNames[seg];
			layout.rowSheetRowNumber[i] = (i % kMaxDataRowsPerSheet) + 2; // +1 header, +1 for 1-based
		}

		layoutByTable[tdata.name] = layout;
	}

	// Keyed by (table, column, value) with no string concatenation, so a
	// value containing the old separator character ('.') can never collide
	// with an unrelated (table, column, value) triple (FR-013).
	using PkKey = std::tuple<std::wstring, std::wstring, std::wstring>;
	std::map<PkKey, std::wstring> pkCellMap; // (Table, Col, Value) -> "'Sheet'!A5"
	for (const auto& tdata : allData) {
		const TableLayout& layout = layoutByTable[tdata.name];
		for (size_t rowIdx = 0; rowIdx < tdata.rows.size(); ++rowIdx) {
			for (size_t colIdx = 0; colIdx < tdata.schema.columns.size(); ++colIdx) {
				const std::wstring& colName = tdata.schema.columns[colIdx];
				if (std::find(tdata.schema.primaryKeys.begin(), tdata.schema.primaryKeys.end(), colName) != tdata.schema.primaryKeys.end()) {
					std::wstring val = tdata.rows[rowIdx][colIdx];
					std::wstring cellRef = L"'" + layout.rowSheetName[rowIdx] + L"'!" + GetColLetter(colIdx) + std::to_wstring(layout.rowSheetRowNumber[rowIdx]);
					pkCellMap[PkKey(tdata.name, colName, val)] = cellRef;
				}
			}
		}
	}

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
		for (const auto& tdata : allData) {
			const TableLayout& layout = layoutByTable[tdata.name];
			std::wstring sheetsJoined;
			for (size_t i = 0; i < layout.segmentSheetNames.size(); ++i) {
				if (i > 0)
					sheetsJoined += L", ";
				sheetsJoined += layout.segmentSheetNames[i];
			}
			writeLiteralRow(summaryRow++, {
				tdata.name,
				std::to_wstring(tdata.rows.size()),
				std::to_wstring(tdata.schema.columns.size()),
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
		for (const auto& tdata : allData) {
			for (const auto& pkName : tdata.schema.primaryKeys)
				writeLiteralRow(constraintsRow++, { tdata.name, pkName, L"PrimaryKey", L"", L"" });
			for (const auto& fk : tdata.schema.foreignKeys)
				writeLiteralRow(constraintsRow++, { tdata.name, fk.columnName, L"ForeignKey", fk.referencedTable, fk.referencedColumn });
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

	for (const auto& tdata : allData) {
		const TableLayout& layout = layoutByTable[tdata.name];
		size_t totalRows = tdata.rows.size();
		size_t numSegments = layout.segmentSheetNames.size();

		for (size_t seg = 0; seg < numSegments; ++seg) {
			size_t segStart = seg * kMaxDataRowsPerSheet;
			size_t segEnd = std::min(totalRows, segStart + kMaxDataRowsPerSheet);

			smart_ptr<OOX::File> oWorksheetFile(new OOX::Spreadsheet::CWorksheet(NULL));
			OOX::Spreadsheet::CWorksheet *pWorksheet = (OOX::Spreadsheet::CWorksheet *)oWorksheetFile.GetPointer();
			pWorksheet->m_oSheetData.Init();
			pWorksheet->m_oSheetFormatPr.Init();
			pWorksheet->m_oSheetFormatPr->m_oBaseColWidth = 9;

			cellFormatController->m_pWorksheet = pWorksheet;

			// Write Headers (repeated on every segment, per FR-007)
			OOX::Spreadsheet::CRow *pHeaderRow = new OOX::Spreadsheet::CRow();
			pHeaderRow->m_oR.Init();
			pHeaderRow->m_oR->SetValue(1);

			for (size_t colIdx = 0; colIdx < tdata.schema.columns.size(); ++colIdx) {
				OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
				pCell->m_oType.Init();
				pCell->setRowCol(0, colIdx);
				std::wstring colName = tdata.schema.columns[colIdx];
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

			// Write Data for this segment's row range
			int rowIndex = 1;
			for (size_t rowIdx = segStart; rowIdx < segEnd; ++rowIdx) {
				const auto& rowData = tdata.rows[rowIdx];
				OOX::Spreadsheet::CRow *pRow = new OOX::Spreadsheet::CRow();
				pRow->m_oR.Init();
				pRow->m_oR->SetValue(rowIndex + 1);

				for (size_t colIdx = 0; colIdx < tdata.schema.columns.size(); ++colIdx) {
					OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
					pCell->m_oType.Init();
					pCell->setRowCol(rowIndex, colIdx);

					std::wstring val = rowData[colIdx];
					std::wstring colName = tdata.schema.columns[colIdx];
					pCell->m_oCacheValue = val;

					// check if fk
					for (const auto& fk : tdata.schema.foreignKeys) {
						if (fk.columnName == colName) {
							auto pkIt = pkCellMap.find(PkKey(fk.referencedTable, fk.referencedColumn, val));
							if (pkIt != pkCellMap.end()) {
								pCell->m_oFormula.Init();
								pCell->m_oFormula->m_sText = pkIt->second;
							}
							break;
						}
					}

					cellFormatController->ProcessCellType(pCell, val, false);
					pRow->m_arrItems.push_back(pCell);
				}
				pWorksheet->m_oSheetData->m_arrItems.push_back(pRow);
				rowIndex++;
			}

			// Data validation for PK uniqueness, scoped to this segment's own rows.
			if (!tdata.schema.primaryKeys.empty()) {
				if (!pWorksheet->m_oDataValidations.IsInit())
					pWorksheet->m_oDataValidations.Init();

				for (const auto& pkName : tdata.schema.primaryKeys) {
					auto it = std::find(tdata.schema.columns.begin(), tdata.schema.columns.end(), pkName);
					if (it != tdata.schema.columns.end()) {
						int pkColIdx = std::distance(tdata.schema.columns.begin(), it);
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
		}
	}

	return 0; // S_OK
}

