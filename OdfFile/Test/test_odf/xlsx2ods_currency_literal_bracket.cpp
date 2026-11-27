// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// XLSX -> ODS: currency formats whose code has a literal '[' after the [$...]
// token, escaped (\[) or in quoted text (" ["). The writer cuts the token out of
// the code by its brackets; the literal '[' must not be taken as the token's
// opening bracket.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The number:currency-style element with the given style name, or "".
	std::wstring currency_style(const std::wstring& xml, const std::wstring& name)
	{
		const size_t pos = xml.find(L"<number:currency-style style:name=\"" + name + L"\"");
		if (pos == std::wstring::npos)
			return L"";
		const size_t end = xml.find(L"</number:currency-style>", pos);
		if (end == std::wstring::npos)
			return L"";
		return xml.substr(pos, end - pos);
	}
}

TEST(XlsxToOds, CurrencyFormatWithLiteralBracketAfterToken)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/currency_literal_bracket.xlsx");
	ASSERT_FALSE(dir.empty()) << "XLSX -> ODS conversion failed";
	const std::wstring xml = read_part(dir, L"styles.xml");
	ASSERT_FALSE(xml.empty());

	// NF10001 = [$€-407]#,##0.00\[ , NF10002 = [$€-407]#,##0.00" ["
	for (const wchar_t* name : {L"NF10001", L"NF10002"})
	{
		const std::wstring style = currency_style(xml, name);
		ASSERT_FALSE(style.empty()) << name;

		// The symbol comes first, as in the format code.
		const size_t symbol = style.find(L"<number:currency-symbol");
		const size_t number = style.find(L"<number:number ");
		ASSERT_NE(symbol, std::wstring::npos) << name;
		ASSERT_NE(number, std::wstring::npos) << name;
		EXPECT_LT(symbol, number) << name;
		EXPECT_NE(style.find(L">€</number:currency-symbol>"), std::wstring::npos) << name;

		// The number part is #,##0.00 once the token is cut out.
		EXPECT_NE(style.find(L"number:decimal-places=\"2\""), std::wstring::npos) << name;
		EXPECT_NE(style.find(L"number:min-integer-digits=\"1\""), std::wstring::npos) << name;
		EXPECT_NE(style.find(L"number:grouping=\"true\""), std::wstring::npos) << name;
	}
}
