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

#include <string>

// Shared identifier-quoting helper for every engine that builds dynamic SQL
// (FR-014). SQLite and DuckDB both use the SQL-standard double-quote form for
// identifiers (doubling any embedded double quote); neither targets a dialect
// that needs a different form (e.g. MySQL backticks), so one function serves
// both call sites.
inline std::wstring QuoteIdentifier(const std::wstring& identifier)
{
	std::wstring quoted = L"\"";
	for (wchar_t ch : identifier)
	{
		if (ch == L'"')
			quoted += L"\"\"";
		else
			quoted += ch;
	}
	quoted += L"\"";
	return quoted;
}

inline std::string QuoteIdentifier(const std::string& identifier)
{
	std::string quoted = "\"";
	for (char ch : identifier)
	{
		if (ch == '"')
			quoted += "\"\"";
		else
			quoted += ch;
	}
	quoted += "\"";
	return quoted;
}
