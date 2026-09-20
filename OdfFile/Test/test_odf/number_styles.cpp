// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-or-later

// Covers odf_number_styles_context::detect_format(), which maps an OOXML number
// format code onto the ODF data style written on the way out to ODS/ODT. The
// currency token regex there is shared by every conversion into ODF, so the
// date, time, percentage and accounting rows below exist to pin down the
// formats that must keep their current classification.

#include "gtest/gtest.h"

#include "OdfFile/Writer/Format/odf_number_styles_context.h"

#include <string>

namespace
{
using cpdoccore::odf_types::office_value_type;
using cpdoccore::odf_writer::number_format_state;
using cpdoccore::odf_writer::odf_number_styles_context;

struct NumberFormatCase
{
	const wchar_t*				format_code;
	office_value_type::type		ods_type;
	const wchar_t*				currency_str;
	unsigned int				language_code;
	unsigned int				currency_pos;	// 0 none, 1 symbol after number, 2 symbol before
	const char*					name;
};

class NumberFormatDetection : public ::testing::TestWithParam<NumberFormatCase>
{
};

TEST_P(NumberFormatDetection, DetectsOdfDataStyle)
{
	const NumberFormatCase& expected = GetParam();

	// Any id at or above 164 is a custom format, which is the only kind
	// detect_format() inspects; built-in ids are classified from the id alone.
	odf_number_styles_context context;
	const number_format_state& state = context.add_or_find(164, expected.format_code);

	EXPECT_EQ(expected.ods_type, state.ods_type);
	EXPECT_EQ(std::wstring(expected.currency_str), state.currency_str);
	EXPECT_EQ(expected.language_code, state.language_code);
	EXPECT_EQ(expected.currency_pos, state.currency_pos);
}

// A currency token carrying an explicit locale, the form Excel itself writes.
// These classifications predate the locale-free token support and must not move.
const NumberFormatCase kLocaleSuffixedCurrency[] = {
	{ L"[$€-407]#,##0.00",			office_value_type::Currency,	L"€",	0x407,	2,	"EuroGerman" },
	{ L"_-[$€-2]* #,##0.00_-",		office_value_type::Currency,	L"€",	0x2,	2,	"EuroAccounting" },
	{ L"[$$-409]#,##0.00_)",			office_value_type::Currency,	L"$",		0x409,	2,	"DollarEnglish" },
	{ L"[$€-407]#,##0.00;[Red]-[$€-407]#,##0.00",
										office_value_type::Currency,	L"€",	0x407,	2,	"EuroWithRedNegative" },
};

// Everything that must not be dragged into the currency branch by the widened
// token regex: [$-<locale>] prefixes carry no symbol, and the rest hold no
// [$...] token at all.
const NumberFormatCase kNonCurrency[] = {
	{ L"[$-409]mmmm d, yyyy",	office_value_type::Date,		L"",	0x409,	0,	"LocaleOnlyDate" },
	{ L"[$-409]h:mm:ss AM/PM",	office_value_type::Time,		L"",	0x409,	0,	"LocaleOnlyTime" },
	{ L"[$-F800]h:mm:ss",		office_value_type::Time,		L"",	0xF800,	0,	"SystemLongTime" },
	{ L"[Red][$-409]#,##0.00",	office_value_type::Float,		L"",	0x409,	0,	"LocaleOnlyRedNumber" },
	{ L"[h]:mm:ss",				office_value_type::Time,		L"",	0,		0,	"ElapsedTime" },
	{ L"mm-dd-yy",				office_value_type::Date,		L"",	0,		0,	"PlainDate" },
	{ L"0.00%",					office_value_type::Percentage,	L"",	0,		0,	"Percentage" },
	{ L"#,##0.00",				office_value_type::Float,		L"",	0,		0,	"PlainNumber" },
};

// A currency token with no locale suffix. ODF carries the symbol on the data
// style rather than in a locale, so this is the form the ODF reader produces;
// before the token regex accepted it these all fell through to a plain number
// and the symbol was dropped on the way back out to ODS.
const NumberFormatCase kLocaleFreeCurrency[] = {
	{ L"[$€]#,##0.00",		office_value_type::Currency,	L"€",	0,	2,	"EuroPrefix" },
	{ L"#,##0.00[$€]",		office_value_type::Currency,	L"€",	0,	1,	"EuroSuffix" },
	{ L"[$USD] #,##0.00",		office_value_type::Currency,	L"USD",		0,	2,	"MultiCharSymbol" },
	{ L"#,##0.00[$€];[Red]-#,##0.00[$€]",
								office_value_type::Currency,	L"€",	0,	1,	"EuroSuffixWithRedNegative" },
	// The previous token regex was greedy past the closing bracket, so a code
	// holding both a symbol token and a locale token matched the whole span and
	// yielded "€]#,##0.00[$" as the currency symbol.
	{ L"[$€]#,##0.00[$-407]",
								office_value_type::Currency,	L"€",	0,	2,	"SymbolAndLocaleTokens" },
};

std::string CaseName(const ::testing::TestParamInfo<NumberFormatCase>& info)
{
	return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(LocaleSuffixedCurrency, NumberFormatDetection,
						 ::testing::ValuesIn(kLocaleSuffixedCurrency), CaseName);
INSTANTIATE_TEST_SUITE_P(NonCurrency, NumberFormatDetection,
						 ::testing::ValuesIn(kNonCurrency), CaseName);
INSTANTIATE_TEST_SUITE_P(LocaleFreeCurrency, NumberFormatDetection,
						 ::testing::ValuesIn(kLocaleFreeCurrency), CaseName);
}
