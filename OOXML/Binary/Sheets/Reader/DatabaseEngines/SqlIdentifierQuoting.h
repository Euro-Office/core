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
