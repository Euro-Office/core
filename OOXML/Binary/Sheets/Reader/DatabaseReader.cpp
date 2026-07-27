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
#include <map>
#include <algorithm>

using namespace NExtractTools;

namespace
{
	// ".db" is ambiguous between SQLite and Berkeley DB. SQLite files always
	// start with this literal 16-byte header; anything else with a ".db"
	// extension is assumed to be Berkeley DB.
	bool IsSqliteHeader(const std::wstring& sFilePath)
	{
		const char sSqliteMagic[] = "SQLite format 3\0";
		char buf[16] = { 0 };

		NSFile::CFileBinary oFile;
		if (!oFile.OpenFile(sFilePath))
			return false;

		DWORD dwRead = 0;
		bool bRead = oFile.ReadFile((BYTE*)buf, sizeof(buf), dwRead);
		oFile.CloseFile();

		return bRead && dwRead == sizeof(buf) && 0 == memcmp(buf, sSqliteMagic, sizeof(buf));
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

	std::unique_ptr<IDatabaseEngine> engine;
	if (sExt == L".sqlite" || sExt == L".sqlite3" || sExt == L".db3") {
		engine.reset(new SqliteEngine());
	} else if (sExt == L".db") {
		if (IsSqliteHeader(sFileName))
			engine.reset(new SqliteEngine());
		else
			engine.reset(new BerkeleyDbEngine());
	} else if (sExt == L".duckdb" || sExt == L".parquet" || sExt == L".pq") {
		engine.reset(new DuckDbEngine());
	} else if (sExt == L".mdb" || sExt == L".accdb") {
		engine.reset(new MdbEngine());
	} else if (sExt == L".bdb") {
		engine.reset(new BerkeleyDbEngine());
	} else {
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

	auto getColLetter = [](int colIdx) -> std::wstring {
		std::wstring res;
		while (colIdx >= 0) {
			res.insert(res.begin(), (wchar_t)(L'A' + (colIdx % 26)));
			colIdx = (colIdx / 26) - 1;
		}
		return res;
	};

	std::map<std::wstring, std::wstring> pkCellMap; // "Table.Col.Value" -> "Table!A5"
	for (const auto& tdata : allData) {
		for (size_t rowIdx = 0; rowIdx < tdata.rows.size(); ++rowIdx) {
			for (size_t colIdx = 0; colIdx < tdata.schema.columns.size(); ++colIdx) {
				const std::wstring& colName = tdata.schema.columns[colIdx];
				// check if pk
				if (std::find(tdata.schema.primaryKeys.begin(), tdata.schema.primaryKeys.end(), colName) != tdata.schema.primaryKeys.end()) {
					std::wstring val = tdata.rows[rowIdx][colIdx];
					std::wstring cellRef = L"'" + tdata.name + L"'!" + getColLetter(colIdx) + std::to_wstring(rowIdx + 2);
					pkCellMap[tdata.name + L"." + colName + L"." + val] = cellRef;
				}
			}
		}
	}

	for (const auto& tdata : allData) {
		smart_ptr<OOX::File> oWorksheetFile(new OOX::Spreadsheet::CWorksheet(NULL));
		OOX::Spreadsheet::CWorksheet *pWorksheet = (OOX::Spreadsheet::CWorksheet *)oWorksheetFile.GetPointer();
		pWorksheet->m_oSheetData.Init();
		pWorksheet->m_oSheetFormatPr.Init();
		pWorksheet->m_oSheetFormatPr->m_oBaseColWidth = 9;
		
		cellFormatController->m_pWorksheet = pWorksheet;
		
		// Write Headers
		OOX::Spreadsheet::CRow *pHeaderRow = new OOX::Spreadsheet::CRow();
		pHeaderRow->m_oR.Init();
		pHeaderRow->m_oR->SetValue(1);
		
		for (size_t colIdx = 0; colIdx < tdata.schema.columns.size(); ++colIdx) {
			OOX::Spreadsheet::CCell *pCell = new OOX::Spreadsheet::CCell();
			pCell->m_oType.Init();
			pCell->setRowCol(0, colIdx);
			std::wstring colName = tdata.schema.columns[colIdx];
			pCell->m_oCacheValue = colName;
			cellFormatController->ProcessCellType(pCell, colName, false);
			pHeaderRow->m_arrItems.push_back(pCell);
		}
		pWorksheet->m_oSheetData->m_arrItems.push_back(pHeaderRow);
		
		// Write Data
		int rowIndex = 1;
		for (const auto& rowData : tdata.rows) {
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
				bool isFk = false;
				for (const auto& fk : tdata.schema.foreignKeys) {
					if (fk.columnName == colName) {
						std::wstring key = fk.referencedTable + L"." + fk.referencedColumn + L"." + val;
						if (pkCellMap.find(key) != pkCellMap.end()) {
							pCell->m_oFormula.Init();
							pCell->m_oFormula->m_sText = pkCellMap[key];
							isFk = true;
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
		
		// Data validation for PK uniqueness
		if (tdata.schema.primaryKeys.size() > 0) {
			if (!pWorksheet->m_oDataValidations.IsInit())
				pWorksheet->m_oDataValidations.Init();

			for (const auto& pkName : tdata.schema.primaryKeys) {
				auto it = std::find(tdata.schema.columns.begin(), tdata.schema.columns.end(), pkName);
				if (it != tdata.schema.columns.end()) {
					int pkColIdx = std::distance(tdata.schema.columns.begin(), it);
					std::wstring colLetter = getColLetter(pkColIdx);
					
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
		pSheet->m_oName = tdata.name;
		pSheet->m_oSheetId.Init();
		pSheet->m_oSheetId->SetValue(sheetIndex++);
		pSheet->m_oRid.Init();
		pSheet->m_oRid->SetValue(oRid.ToString());

		if (!oXlsx.m_pWorkbook->m_oSheets.IsInit())
			oXlsx.m_pWorkbook->m_oSheets.Init();
		oXlsx.m_pWorkbook->m_oSheets->m_arrItems.push_back(pSheet);
	}
	
	return 0; // S_OK
}

