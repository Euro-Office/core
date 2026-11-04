/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <string>
#include <vector>

namespace OOX
{
namespace Spreadsheet
{
	class CWorkbook;

	// A part of the original package, copied byte for byte.
	struct CPreservedPart
	{
		std::wstring sName;        // package path without a leading '/', e.g. L"xl/drawings/drawing1.xml"
		std::wstring sContentType; // empty for relationship parts
		std::string  sData;
	};

	struct CPreservedDefinedName
	{
		std::wstring sXml;   // the <definedName> element as read
		bool bLocal = false; // scoped to the preserved sheet; localSheetId is reapplied on write
	};

	// A sheet the editor cannot represent (Excel 4.0 macro sheet). It is kept out of
	// the editor's sheet list and carried through Editor.bin unchanged, so it can be
	// written back on save.
	struct CPreservedSheet
	{
		std::wstring sName;
		std::wstring sState;
		std::wstring sRelType;
		unsigned int nSheetId = 0;
		int nIndex = 0; // position in the original <sheets> list

		std::vector<CPreservedPart> arParts; // arParts[0] is the sheet part itself
		std::vector<CPreservedDefinedName> arDefinedNames;
	};

	bool IsPreservedSheetRelType(const std::wstring& sRelType);

	// xlsx -> Editor.bin: moves sheets the editor cannot represent out of the workbook
	// model into workbook.m_arPreservedSheets and adjusts sheet indices accordingly.
	void CollectPreservedSheets(CWorkbook& workbook, const std::wstring& sPackageDir);

	// Editor.bin -> xlsx: puts the sheets from workbook.m_arPreservedSheets back at their
	// original positions once the editor's sheets are in place.
	void RestorePreservedSheets(CWorkbook& workbook);
}
}
