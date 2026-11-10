/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <map>
#include <string>

// What a spreadsheet contains, independent of how it is written: part names,
// relationship ids, attribute order, string tables and style indexes differ
// between applications, the content does not.
//
// Each fact is "<category>/<what>" -> value, UTF-8, e.g.
//   "cells/Sheet1/B2"   -> "n|8||"   (type|value|formula|formula type and ref)
//   "styles/Sheet1/B2"  -> number format, font, fill, border, alignment
//   "pivots/Sheet2/PivotTable1/records" -> the pivot cache records
// Categories: workbook, names, cells, styles, merges, comments, tables,
// pivots, slicers, chartsheets.
typedef std::map<std::string, std::string> WorkbookFacts;

// Reads an unpacked XLSX package.
WorkbookFacts LoadWorkbookFacts(const std::wstring& unpackedDir);
