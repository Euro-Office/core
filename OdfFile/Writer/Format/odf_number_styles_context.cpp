// SPDX-FileCopyrightText: 2010-2023 Ascensio System SIA
// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

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
#include "../../../OOXML/Base/Unit.h"

#include "logging.h"

#include "odf_number_styles_context.h"
#include "ods_conversion_context.h"

#include "number_style.h"
#include "style_map.h"
#include "style_text_properties.h"

#include <boost/algorithm/string.hpp>

namespace cpdoccore {

	using namespace odf_types;

namespace odf_writer {


struct def_language_code
{
	std::wstring country;
	std::wstring country_code;
	std::wstring language_code;
	unsigned int id;
};

static const def_language_code LanguageCodeTable[] =
{
	{L"Euro",				L"EU", L"en",	0x0002},
	{L"Afrikaans",			L"", L"",	0x0436},
	{L"Albanian",			L"", L"",	0x041C},
	{L"Arabic",				L"", L"",	0x0401},
	{L"Arabic Algeria",		L"", L"",	0x01401},
	{L"Arabic Bahrain",		L"", L"",	0x03C01},
	{L"Arabic Egypt",		L"", L"",	0x0C01},
	{L"Arabic Iraq",		L"", L"",	0x0801},
	{L"Arabic Jordan",		L"", L"",	0x02C01},
	{L"Arabic Kuwait",		L"", L"",	0x03401},
	{L"Arabic Lebanon",		L"", L"",	0x03001},
	{L"Arabic Libya",		L"", L"",	0x01001},
	{L"Arabic Morocco",		L"", L"",	0x01801},
	{L"Arabic Oman",		L"", L"",	0x02001},
	{L"Arabic Qatar",		L"", L"",	0x04001},
	{L"Arabic Saudi Arabia",L"", L"",	0x0401},
	{L"Arabic Syria",		L"", L"",	0x02801},
	{L"Arabic Tunisia",		L"", L"",	0x01C01},
	{L"Arabic U.A.E",		L"", L"",	0x03801},
	{L"Arabic Yemen",		L"", L"",	0x02401},
	{L"Armenian",			L"", L"",	0x042B},
	{L"Assamese",			L"", L"",	0x044D},
	{L"Azeri Cyrillic",		L"", L"",	0x082C},
	{L"Azeri Latin",		L"", L"",	0x042C},
	{L"Basque",				L"", L"",	0x042D},
	{L"Belgian Dutch",		L"", L"",	0x0813},
	{L"Belgian French",		L"", L"",	0x080C},
	{L"Bengali",			L"", L"",	0x0445},
	{L"Portuguese (Brazil)",L"", L"",	0x0416},
	{L"Bulgarian",			L"", L"",	0x0402},
	{L"Burmese",			L"", L"",	0x0455},
	{L"Byelorussian",		L"", L"",	0x0423},
	{L"Catalan",			L"", L"",	0x0403},
	{L"Chinese Hong Kong SAR",L"", L"",	0x0C04},
	{L"Chinese Macau SAR",	L"", L"",	0x01404},
	{L"Chinese Simplified",	L"CN", L"cn",	0x0804},
	{L"Chinese Singapore",	L"", L"",	0x01004},
	{L"Chinese Traditional",L"CN", L"cn",	0x0404},
	{L"Croatian",			L"", L"",	0x041A},
	{L"Czech",				L"", L"",	0x0405},
	{L"Danish",				L"", L"",	0x0406},
	{L"Dutch",				L"", L"",	0x0413},
	{L"English Australia",	L"", L"en",	0x0C09},
	{L"English Belize",		L"", L"en",	0x02809},
	{L"English Canadian",	L"", L"en",	0x01009},
	{L"English Caribbean",	L"", L"en",	0x02409},
	{L"English Ireland",	L"", L"en",	0x01809},
	{L"English Jamaica",	L"", L"en",	0x02009},
	{L"English New Zealand",L"", L"en",	0x01409},
	{L"English Philippines",L"", L"en",	0x03409},
	{L"English South Africa",L"", L"en",	0x01C09},
	{L"English Trinidad",	L"", L"en",	0x02C09},
	{L"English U.K.",		L"GB",L"en",	0x0809},
	{L"English U.S.",		L"US",L"en",	0x0409},
	{L"English Zimbabwe",	L"", L"en",	0x03009},
	{L"Estonian",			L"EE", L"et",0x0425},
	{L"Faeroese",			L"", L"",	0x0438},
	{L"Farsi",				L"", L"",	0x0429},
	{L"Finnish",			L"", L"",	0x040B},
	{L"French",				L"FR", L"fr",	0x040C},
	{L"French Cameroon",	L"", L"fr",	0x02C0C},
	{L"French Canadian",	L"", L"fr",	0x0C0C},
	{L"French Cote d'Ivoire",L"", L"fr",	0x0300C},
	{L"French Luxembourg",	L"", L"fr",	0x0140C},
	{L"French Mali",		L"", L"fr",	0x0340C},
	{L"French Monaco",		L"", L"fr",	0x0180C},
	{L"French Reunion",		L"", L"fr",	0x0200C},
	{L"French Senegal",		L"", L"fr",	0x0280C},
	{L"French West Indies",	L"", L"fr",	0x01C0C},
	{L"French Congo (DRC)",	L"", L"fr",	0x0240C},
	{L"Frisian Netherlands",L"", L"",	0x0462},
	{L"Gaelic Ireland",		L"", L"",	0x083C},
	{L"Gaelic Scotland",	L"", L"",	0x043C},
	{L"Galician",			L"", L"",	0x0456},
	{L"Georgian",			L"", L"",	0x0437},
	{L"German",				L"DE", L"de",	0x0407},
	{L"German Austria",		L"", L"de",	0x0C07},
	{L"German Liechtenstein",L"", L"de",	0x01407},
	{L"German Luxembourg",	L"", L"de",	0x01007},
	{L"Greek",				L"", L"",	0x0408},
	{L"Gujarati",			L"", L"",	0x0447},
	{L"Hebrew",				L"IN", L"hi",	0x040D},
	{L"Hindi",				L"", L"",	0x0439},
	{L"Hungarian",			L"", L"",	0x040E},
	{L"Icelandic",			L"", L"",	0x040F},
	{L"Indonesian",			L"", L"",	0x0421},
	{L"Italian",			L"IT", L"it",	0x0410},
	{L"Japanese",			L"JP",L"jp",	0x0411},
	{L"Kannada",			L"", L"",	0x044B},
	{L"Kashmiri",			L"", L"",	0x0460},
	{L"Kazakh",				L"", L"",	0x043F},
	{L"Khmer",				L"", L"",	0x0453},
	{L"Kirghiz",			L"", L"",	0x0440},
	{L"Konkani",			L"", L"",	0x0457},
	{L"Korean",				L"", L"",	0x0412},
	{L"Lao",				L"", L"",	0x0454},
	{L"Latvian",			L"LT", L"lt",	0x0426},
	{L"Lithuanian",			L"", L"",	0x0427},
	{L"FYRO Macedonian",	L"", L"",	0x042F},
	{L"Malayalam",			L"", L"",	0x044C},
	{L"Malay Brunei Darussalam",	L"", L"",	0x083E},
	{L"Malaysian",			L"", L"",	0x043E},
	{L"Maltese",			L"", L"",	0x043A},
	{L"Manipuri",			L"", L"",	0x0458},
	{L"Marathi",			L"", L"",	0x044E},
	{L"Mongolian",			L"", L"",	0x0450},
	{L"Nepali",				L"", L"",	0x0461},
	{L"Norwegian Bokmol",	L"", L"",	0x0414},
	{L"Norwegian Nynorsk",	L"", L"",	0x0814},
	{L"Oriya",				L"", L"",	0x0448},
	{L"Polish",				L"", L"",	0x0415},
	{L"Portuguese",			L"", L"",	0x0816},
	{L"Punjabi",			L"", L"",	0x0446},
	{L"Rhaeto-Romanic",		L"", L"",	0x0417},
	{L"Romanian",			L"", L"",	0x0418},
	{L"Romanian Moldova",	L"", L"",	0x0818},
	{L"Russian",			L"RU",L"ru",	0x0419},
	{L"Russian Moldova",	L"", L"",	0x0819},
	{L"Sami Lappish",		L"", L"",	0x043B},
	{L"Sanskrit",			L"", L"",	0x044F},
	{L"Serbian Cyrillic",	L"", L"",	0x0C1A},
	{L"Serbian Latin",		L"", L"",	0x081A},
	{L"Sesotho",			L"", L"",	0x0430},
	{L"Sindhi",				L"", L"",	0x0459},
	{L"Slovak",				L"", L"",	0x041B},
	{L"Slovenian",			L"", L"",	0x0424},
	{L"Sorbian",			L"", L"",	0x042E},
	{L"Spanish (Traditional)",	L"ES", L"es",	0x040A},
	{L"Spanish Argentina",	L"", L"",	0x02C0A},
	{L"Spanish Bolivia",	L"", L"",	0x0400A},
	{L"Spanish Chile",		L"", L"",	0x0340A},
	{L"Spanish Colombia",	L"", L"",	0x0240A},
	{L"Spanish Costa Rica",	L"", L"",	0x0140A},
	{L"Spanish Dominican Republic",	L"", L"",	0x01C0A},
	{L"Spanish Ecuador",	L"", L"",	0x0300A},
	{L"Spanish El Salvador",L"", L"",	0x0440A},
	{L"Spanish Guatemala",	L"", L"",	0x0100A},
	{L"Spanish Honduras",	L"", L"",	0x0480A},
	{L"Spanish Nicaragua",	L"", L"",	0x04C0A},
	{L"Spanish Panama",		L"", L"",	0x0180A},
	{L"Spanish Paraguay",	L"", L"",	0x3C0A},
	{L"Spanish Peru",		L"", L"",	0x0280A},
	{L"Spanish Puerto Rico",L"", L"",	0x0500A},
	{L"Spanish Spain (Modern Sort)",	L"", L"",	0x0C0A},
	{L"Spanish Uruguay",	L"", L"",	0x0380A},
	{L"Spanish Venezuela",	L"", L"",	0x0200A},
	{L"Sutu",				L"", L"",	0x0430},
	{L"Swahili",			L"", L"",	0x0441},
	{L"Swedish",			L"", L"",	0x041D},
	{L"Swedish Finland",	L"", L"",	0x081D},
	{L"Swiss French",		L"", L"",	0x0100C},
	{L"Swiss German",		L"", L"",	0x0807},
	{L"Swiss Italian",		L"", L"",	0x0810},
	{L"Tajik",				L"", L"",	0x0428},
	{L"Tamil",				L"", L"",	0x0449},
	{L"Tatar",				L"", L"",	0x0444},
	{L"Telugu",				L"", L"",	0x044A},
	{L"Thai",				L"", L"",	0x041E},
	{L"Tibetan",			L"", L"",	0x0451},
	{L"Tsonga",				L"", L"",	0x0431},
	{L"Tswana",				L"", L"",	0x0432},
	{L"Turkish",			L"", L"",	0x041F},
	{L"Turkmen",			L"", L"",	0x0442},
	{L"Ukrainian",			L"UA", L"uk",	0x0422},
	{L"Urdu",				L"", L"",	0x0420},
	{L"Uzbek Cyrillic",		L"", L"",	0x0843},
	{L"Uzbek Latin",		L"", L"",	0x0443},
	{L"Venda",				L"", L"",	0x0433},
	{L"Vietnamese",			L"", L"",	0x042A},
	{L"Welsh",				L"", L"",	0x0452},
	{L"Xhosa",				L"", L"",	0x0434},
	{L"Zulu",				L"", L"",	0x0435},
	{L"",					L"PL", L"pl",	0x019F },
	{L"",					L"IR", L"fa",	0x00160429}
};

namespace
{
	// The sections of a format code, split at ';' outside quoted text, escapes and [] tokens; empty sections are kept.
	std::vector<std::wstring> split_format_sections(const std::wstring & code)
	{
		std::vector<std::wstring> sections(1);
		bool quoted = false, bracket = false;
		for (size_t i = 0; i < code.size(); ++i)
		{
			const wchar_t c = code[i];
			if (!quoted && !bracket && c == L'\\' && i + 1 < code.size())
			{
				sections.back() += c;
				sections.back() += code[++i];
				continue;
			}
			if (c == L'"' && !bracket)		quoted = !quoted;
			else if (!quoted && c == L'[')	bracket = true;
			else if (!quoted && c == L']')	bracket = false;
			else if (!quoted && !bracket && c == L';')
			{
				sections.emplace_back();
				continue;
			}
			sections.back() += c;
		}
		return sections;
	}

	// [Red] etc. and [Color1] to [Color8] (the same colours in the default palette). Other [ColorN] are colours too, but unknown.
	bool format_color(const std::wstring & token, std::wstring & color)
	{
		static const wchar_t* names[]	= {L"black", L"white", L"red", L"green", L"blue", L"yellow", L"magenta", L"cyan"};
		static const wchar_t* colors[]	= {L"#000000", L"#ffffff", L"#ff0000", L"#00ff00", L"#0000ff", L"#ffff00", L"#ff00ff", L"#00ffff"};

		const std::wstring name = boost::algorithm::to_lower_copy(token);
		for (size_t i = 0; i < 8; ++i)
		{
			if (name == names[i])
			{
				color = colors[i];
				return true;
			}
		}
		if (name.size() > 5 && name.compare(0, 5, L"color") == 0 &&
			name.find_first_not_of(L"0123456789", 5) == std::wstring::npos)
		{
			const int index = std::stoi(name.substr(5, 3));
			color = (index >= 1 && index <= 8) ? colors[index - 1] : L"";
			return true;
		}
		return false;
	}

	// [<0], [>=100], [<>0] -> the ODF condition value()<0, value()>=100, value()!=0; empty when the token is no condition.
	std::wstring format_condition(const std::wstring & token)
	{
		static const wchar_t* ops[] = {L"<=", L">=", L"<>", L"<", L">", L"="};
		for (const wchar_t* op : ops)
		{
			const std::wstring o(op);
			if (token.compare(0, o.size(), o) != 0)
				continue;

			const std::wstring limit = boost::algorithm::trim_copy(token.substr(o.size()));
			if (limit.empty() || limit.find_first_not_of(L"0123456789.-+eE") != std::wstring::npos)
				return L"";
			return L"value()" + (o == L"<>" ? std::wstring(L"!=") : o) + limit;
		}
		return L"";
	}

	struct format_section
	{
		std::wstring code;		// without colour and condition
		std::wstring color;		// #rrggbb, empty when none
		std::wstring condition;	// value()..., empty when none
	};

	format_section parse_format_section(const std::wstring & code)
	{
		format_section result;
		bool quoted = false;
		for (size_t i = 0; i < code.size(); ++i)
		{
			const wchar_t c = code[i];
			if (!quoted && c == L'\\' && i + 1 < code.size())
			{
				result.code += c;
				result.code += code[++i];
				continue;
			}
			if (c == L'"')
				quoted = !quoted;
			if (!quoted && c == L'[')
			{
				const size_t end = code.find(L']', i);
				if (end != std::wstring::npos)
				{
					const std::wstring token = code.substr(i + 1, end - i - 1);
					std::wstring color;
					if (format_color(token, color))
					{
						if (result.color.empty())
							result.color = color;
						i = end;
						continue;
					}
					const std::wstring condition = format_condition(token);
					if (false == condition.empty())
					{
						result.condition = condition;
						i = end;
						continue;
					}
				}
			}
			result.code += c;
		}
		return result;
	}

	// A section owns its sign: the literal text before its first and after its last digit placeholder ('-', '+', '(' and ')',
	// quoted text and characters escaped with '\', but not an escaped space) is shown as written, e.g. -#,##0, \(#,##0\) or
	// "-"0. Returns false when the section has no digit placeholder (only text). "General" counts as a placeholder.
	bool split_sign_literals(const std::wstring & code, bool question_marks_are_digits,
							 std::wstring & prefix, std::wstring & suffix, std::wstring & core)
	{
		enum kind { other, digit, literal };
		struct token { std::wstring code; kind type; std::wstring text; };
		std::vector<token> tokens;
		for (size_t i = 0; i < code.size(); ++i)
		{
			const wchar_t c = code[i];
			if (c == L'"')
			{
				size_t end = code.find(L'"', i + 1);
				if (end == std::wstring::npos) end = code.size() - 1;
				const std::wstring text = code.substr(i + 1, end - i - 1);
				tokens.push_back({code.substr(i, end - i + 1), text.empty() ? other : literal, text});
				i = end;
			}
			else if (c == L'[')
			{
				size_t end = code.find(L']', i);
				if (end == std::wstring::npos) end = code.size() - 1;
				tokens.push_back({code.substr(i, end - i + 1), other, L""});
				i = end;
			}
			else if ((c == L'\\' || c == L'_' || c == L'*') && i + 1 < code.size())
			{
				const wchar_t next = code[i + 1];
				const bool is_literal = c == L'\\' && next != L' ';
				tokens.push_back({code.substr(i, 2), is_literal ? literal : other, std::wstring(1, next)});
				++i;
			}
			else if ((c == L'E' || c == L'e') && i + 1 < code.size() && (code[i + 1] == L'+' || code[i + 1] == L'-'))
			{
				tokens.push_back({code.substr(i, 2), other, L""});
				++i;
			}
			else if ((c == L'G' || c == L'g') && boost::algorithm::iequals(code.substr(i, 7), L"General"))
			{
				tokens.push_back({code.substr(i, 7), digit, L""});
				i += 6;
			}
			else if (c == L'0' || c == L'#' || (c == L'?' && question_marks_are_digits))
				tokens.push_back({std::wstring(1, c), digit, L""});
			else if (c == L'-' || c == L'+' || c == L'(' || c == L')')
				tokens.push_back({std::wstring(1, c), literal, std::wstring(1, c)});
			else
				tokens.push_back({std::wstring(1, c), other, L""});
		}

		size_t first = tokens.size(), last = 0;
		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].type != digit) continue;
			if (first == tokens.size()) first = i;
			last = i;
		}
		if (first == tokens.size())
			return false;

		for (size_t i = 0; i < tokens.size(); ++i)
		{
			if (tokens[i].type == literal && (i < first || i > last))
				(i < first ? prefix : suffix) += tokens[i].text;
			else
				core += tokens[i].code;
		}
		return true;
	}

	// The text a section without digits shows: quoted text and escaped characters as written; padding (_x), fill (*x) and
	// [] tokens are left out.
	std::wstring section_text(const std::wstring & code)
	{
		std::wstring text;
		for (size_t i = 0; i < code.size(); ++i)
		{
			const wchar_t c = code[i];
			if (c == L'"')
			{
				size_t end = code.find(L'"', i + 1);
				if (end == std::wstring::npos) end = code.size();
				text += code.substr(i + 1, end - i - 1);
				i = end;
			}
			else if (c == L'[')
			{
				size_t end = code.find(L']', i);
				i = (end == std::wstring::npos) ? code.size() : end;
			}
			else if (c == L'\\' && i + 1 < code.size())	text += code[++i];
			else if ((c == L'_' || c == L'*') && i + 1 < code.size()) ++i;
			else if (c != L'?' && c != L'@') text += c;
		}
		return text;
	}

	bool is_number_type(odf_types::office_value_type::type type)
	{
		switch (type)
		{
		case office_value_type::Float:
		case office_value_type::Scientific:
		case office_value_type::Fraction:
		case office_value_type::Currency:
		case office_value_type::Percentage:	return true;
		default:							return false;
		}
	}
}

odf_number_styles_context::odf_number_styles_context()
{
}
void odf_number_styles_context::set_odf_context(odf_conversion_context * Context)
{
	odf_context_ = Context;
}
void odf_number_styles_context::create(int oox_num_fmt, std::wstring formatCode)
{
	if (oox_num_fmt < 164) return create_default(oox_num_fmt, formatCode);

	number_format_state state;

	state.oox_num_fmt = oox_num_fmt;
	state.style_name = std::wstring(L"NF1000") + boost::lexical_cast<std::wstring>( number_format_array_.size() + 1);
	state.ods_type = office_value_type::Custom;
	state.language_code = 0;

//////////////////////////////

	state.format_code = split_format_sections(formatCode);

	if (state.format_code.size() > 1 && state.format_code[state.format_code.size() - 1].find(L"@") != std::wstring::npos)
	{
		state.format_code.pop_back();
	}

	number_format_array_.push_back(state);
	named_link_map_[oox_num_fmt] = (int)number_format_array_.size() - 1;

	detect_format(number_format_array_.back());
}


void odf_number_styles_context::create_default(int oox_num_fmt, std::wstring formatCode)
{
	number_format_state state;
	
	state.oox_num_fmt = oox_num_fmt;
	state.ods_type = office_value_type::Custom;
	state.language_code = 0;

	if (oox_num_fmt == 0 && formatCode.empty()) 
	{
		//general
	}
	else
		state.style_name = std::wstring(L"NF1000") + boost::lexical_cast<std::wstring>( number_format_array_.size()+1);

	switch (oox_num_fmt)
	{
	case 1:		formatCode = L"0";					state.ods_type =office_value_type::Float; break;
	case 2:		formatCode = L"0.00";				state.ods_type =office_value_type::Float; break;
	case 3:		formatCode = L"#,##0";				state.ods_type =office_value_type::Float; break;
	case 4:		formatCode = L"#,##0.00";			state.ods_type =office_value_type::Float; break;
	
	case 9:		formatCode = L"0%";					state.ods_type =office_value_type::Percentage; break;
	case 10:	formatCode = L"0.00%";				state.ods_type =office_value_type::Percentage; break;
	
	case 11:	formatCode = L"0.00E+00";			state.ods_type =office_value_type::Scientific; break;
	case 12:	formatCode = L"# ?/?";				state.ods_type =office_value_type::Fraction; break;
	case 13:	formatCode = L"# ??/??";			state.ods_type =office_value_type::Fraction; break;
	
	case 14:	formatCode = L"mm-dd-yy";			state.ods_type =office_value_type::Date; break;
	case 15:	formatCode = L"d-mmm-yy";			state.ods_type =office_value_type::Date; break;
	case 16:	formatCode = L"d-mmm";				state.ods_type =office_value_type::Date; break;
	case 17:	formatCode = L"mmm-yy";				state.ods_type =office_value_type::Date; break;

	case 18:	formatCode = L"h:mm AM/PM";			state.ods_type =office_value_type::Time; break;
	case 19:	formatCode = L"h:mm:ss AM/PM";		state.ods_type =office_value_type::Time; break;
	case 20:	formatCode = L"h:mm";				state.ods_type =office_value_type::Time; break;
	case 21:	formatCode = L"h:mm:ss";			state.ods_type =office_value_type::Time; break;
	case 22:	formatCode = L"m/d/yy h:mm";		state.ods_type =office_value_type::Time; break;

	case 37:	formatCode = L"#,##0 ;(#,##0)";		state.ods_type =office_value_type::Float; break;
	case 38:	formatCode = L"#,##0 ;[Red](#,##0)"; state.ods_type =office_value_type::Float; break;
	case 39:	formatCode = L"#,##0.00;(#,##0.00)"; state.ods_type =office_value_type::Float; break;
	case 40:	formatCode = L"#,##0.00;[Red](#,##0.00)"; state.ods_type =office_value_type::Float; break;

	case 45:	formatCode = L"mm:ss";			state.ods_type =office_value_type::Time; break;
	case 46:	formatCode = L"[h]:mm:ss";		state.ods_type =office_value_type::Time; break;
	case 47:	formatCode = L"mmss.0";			state.ods_type =office_value_type::Time; break;

	case 49:	formatCode = L"@";		state.ods_type =office_value_type::String; break;

	default:
////////////////////////////////// with undefined formatCode .. it is set in the file
			 if (oox_num_fmt >= 5  && oox_num_fmt <= 8)		state.ods_type =office_value_type::Currency; 
		else if (oox_num_fmt >= 41 && oox_num_fmt <= 44)	state.ods_type =office_value_type::Currency; 
		
		else if (oox_num_fmt >= 27 && oox_num_fmt <= 31)	state.ods_type =office_value_type::Date;
		else if (oox_num_fmt >= 50 && oox_num_fmt <= 54)	state.ods_type =office_value_type::Date;
		else if (oox_num_fmt >= 57 && oox_num_fmt <= 58)	state.ods_type =office_value_type::Date;
		else if (oox_num_fmt == 36)							state.ods_type =office_value_type::Date;

		else if (oox_num_fmt >= 32 && oox_num_fmt <= 35)	state.ods_type =office_value_type::Time;
		else if (oox_num_fmt >= 55 && oox_num_fmt <= 56)	state.ods_type =office_value_type::Time;

		else if (oox_num_fmt >= 60 && oox_num_fmt <= 62)	state.ods_type =office_value_type::Float;
		else if (oox_num_fmt >= 69 && oox_num_fmt <= 70)	state.ods_type =office_value_type::Float;

		else if (oox_num_fmt >= 67 && oox_num_fmt <= 68)	state.ods_type =office_value_type::Percentage;

		else if (oox_num_fmt >= 71 && oox_num_fmt <= 74)	state.ods_type =office_value_type::Date;
		else if (oox_num_fmt >= 75 && oox_num_fmt <= 80)	state.ods_type =office_value_type::Time;
		else if (oox_num_fmt == 81)							state.ods_type =office_value_type::Date;
	}
    state.format_code = split_format_sections(formatCode);

	if (state.format_code.size() > 1 && state.format_code[state.format_code.size()-1].find(L"@") != std::wstring::npos)
	{
		state.format_code.pop_back();
	}
////////////////////////////////////////////

	number_format_array_.push_back(state);
	named_link_map_[oox_num_fmt] = (int)number_format_array_.size() - 1;

	detect_format(number_format_array_.back());

}
number_format_state & odf_number_styles_context::add_or_find(int oox_num_fmt, std::wstring formatCode)
{
	if (oox_num_fmt < 0)
	{
		//not spreadsheet TODO - search by formatCode
		oox_num_fmt = 0xffff + named_link_map_.size();
	}	
	
	if (named_link_map_.count(oox_num_fmt) > 0)
	{
        return number_format_array_[named_link_map_.at(oox_num_fmt)];
	}
	else
	{
		create(oox_num_fmt, formatCode);
		return number_format_array_.back();
	}
}

void odf_number_styles_context::process_styles(office_element_ptr root )
{
	for (size_t i = 0; i< number_format_array_.size(); i++)
	{
		create_style(number_format_array_[i]);

		for (size_t j = 0; j < section_styles_.size(); j++)
			root->add_child_element(section_styles_[j]);
		section_styles_.clear();

		root->add_child_element(styles_elments.back());
	}
}	

void odf_number_styles_context::create_style(number_format_state & state)
{
	office_element_ptr elm;

	if (state.format_code.size() > 1 && is_number_type(state.ods_type))
		create_section_styles(state, elm);

	if (!elm) switch(state.ods_type)
	{
		case office_value_type::Float:		
		case office_value_type::Scientific:	
		case office_value_type::Fraction:	create_number_style		(state, elm);	break;
		case office_value_type::Currency:	create_currency_style	(state, elm);	break;
		case office_value_type::Percentage:	create_percentage_style	(state, elm);	break;
		case office_value_type::DateTime:	
											state.ods_type = office_value_type::Date;
		case office_value_type::Date:		
											create_date_style		(state, elm);	break;
		case office_value_type::Time:		create_time_style		(state, elm);	break;
		case office_value_type::Boolean:	create_boolean_style	(state, elm);	break;
		case office_value_type::String:		create_text_style		(state, elm);	break;
	}

	number_style_base* number_style_ = dynamic_cast<number_style_base*>(elm.get());
	if (number_style_)
	{
		number_style_->set_style_name(state.style_name);
	}
	
	styles_elments.push_back(elm);		
}

// A format with sections (positive;negative;zero) is written as LibreOffice does: one sub-style <name>P<n> per section but the
// last, and the last section as the style itself, with a style:map per sub-style. Each section keeps its colour and its sign.
void odf_number_styles_context::create_section_styles(number_format_state & state, office_element_ptr & main_elm)
{
	std::vector<format_section> sections;
	for (size_t i = 0; i < state.format_code.size() && i < 3; ++i)
		sections.push_back(parse_format_section(state.format_code[i]));

	const size_t last = sections.size() - 1;
	const bool default_conditions = sections[0].condition.empty() && sections[last > 1 ? 1 : 0].condition.empty();

	std::vector<office_element_ptr> parts;
	std::vector<std::wstring> names;
	for (size_t i = 0; i <= last; ++i)
	{
		number_format_state part = state;
		part.format_code.assign(1, sections[i].code);

		office_element_ptr elm;
		// in the zero section of positive;negative;zero, '?' shows a blank (e.g. accounting formats show "-" for zero)
		create_section_part(part, state.ods_type, elm, !(i == 2 && default_conditions));

		number_style_base* style = dynamic_cast<number_style_base*>(elm.get());
		if (!style)
			return;

		if (false == sections[i].color.empty())
		{
			create_element(L"style", L"text-properties", style->style_text_properties_, odf_context_);
			if (style_text_properties* props = dynamic_cast<style_text_properties*>(style->style_text_properties_.get()))
				props->content_.fo_color_ = odf_types::color(sections[i].color);
		}
		std::wstring name = (i < last) ? state.style_name + L"P" + std::to_wstring(i) : state.style_name;
		style->set_style_name(name);

		parts.push_back(elm);
		names.push_back(name);
	}

	number_style_base* main_style = dynamic_cast<number_style_base*>(parts[last].get());
	for (size_t i = 0; i < last; ++i)
	{
		std::wstring condition = sections[i].condition;
		if (condition.empty())
			condition = (last == 1) ? L"value()>=0" : (i == 0 ? L"value()>0" : L"value()<0");

		office_element_ptr map_elm;
		create_element(L"style", L"map", map_elm, odf_context_);
		if (style_map* map = dynamic_cast<style_map*>(map_elm.get()))
		{
			map->style_condition_ = condition;
			map->style_apply_style_name_ = names[i];
			main_style->style_map_.push_back(map_elm);
		}
		section_styles_.push_back(parts[i]);
	}
	main_elm = parts[last];
}

void odf_number_styles_context::create_section_part(number_format_state & part, office_value_type::type type,
													office_element_ptr & elm, bool question_marks_are_digits)
{
	std::wstring prefix, suffix, core;
	if (!split_sign_literals(part.format_code[0], question_marks_are_digits, prefix, suffix, core))
	{
		const wchar_t* name = (type == office_value_type::Currency) ? L"currency-style"
							: (type == office_value_type::Percentage) ? L"percentage-style" : L"number-style";
		create_element(L"number", name, elm, odf_context_);

		office_element_ptr elm_text;
		create_element(L"number", L"text", elm_text, odf_context_);
		if (number_text* text = dynamic_cast<number_text*>(elm_text.get()))
			text->add_text(section_text(part.format_code[0]));
		if (elm)
			elm->add_child_element(elm_text);
		return;
	}

	part.format_code[0] = core;
	switch (type)
	{
	case office_value_type::Currency:	create_currency_style	(part, elm);	break;
	case office_value_type::Percentage:	create_percentage_style	(part, elm);	break;
	default:							create_number_style		(part, elm);	break;
	}

	number_style_base* style = dynamic_cast<number_style_base*>(elm.get());
	if (!style)
		return;

	for (int i = 0; i < 2; ++i)
	{
		const std::wstring & literal = i == 0 ? prefix : suffix;
		if (literal.empty())
			continue;

		office_element_ptr elm_text;
		create_element(L"number", L"text", elm_text, odf_context_);
		if (number_text* text = dynamic_cast<number_text*>(elm_text.get()))
			text->add_text(literal);

		if (i == 0)	style->content_.insert(style->content_.begin(), elm_text);
		else		style->content_.push_back(elm_text);
	}
}

void odf_number_styles_context::create_number_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"number-style", root_elm, odf_context_);

	if (false == state.format_code.empty())
	{
		office_element_ptr elm;
	
		create_numbers(state, elm, root_elm);
	}
}
void odf_number_styles_context::create_numbers(number_format_state& state, office_element_ptr& elm, office_element_ptr& root_elm)
{
	_CP_OPT(int) min_digit, min_decimal;
	_CP_OPT(int) separate;

	switch (state.ods_type)
	{
	case office_value_type::Currency:
	case office_value_type::Percentage:
	case office_value_type::Float:		create_element(L"number", L"number", elm, odf_context_); break;
	case office_value_type::Scientific:	create_element(L"number", L"scientific-number", elm, odf_context_); break;
	case office_value_type::Fraction:	create_element(L"number", L"fraction", elm, odf_context_); break;
	}

	office_element_ptr elm_text;

	bool bText = false;
	int indText = -1;
	int indNumber = -1;

	if (state.format_code[0].empty())
	{
		//format not defined .. default
		min_digit = 1;
	}
	else
	{
		std::vector<std::wstring> splits;
		boost::algorithm::split(splits, state.format_code[0], boost::algorithm::is_any_of(L"\\"), boost::algorithm::token_compress_on);

		for (size_t i = 0; i < splits.size(); i++)
		{
			if (std::wstring::npos != splits[i].find(L"\""))
			{
				bText = true;
				indText = (int)i;
				break;
			}
		}
		for (size_t i = 0; i < splits.size(); i++)
		{
			if (i != indText)
			{
				indNumber = (int)i;
				break;
			}
		}

		if (indNumber < 0)
			indNumber = 0;

		// split by space, find #0 - use it ... ???? 
		std::wstring str1;
		boost::wregex re1(L"([^#0.,]+)");

		str1 = boost::regex_replace(splits[indNumber], re1, L"", boost::match_default | boost::format_all);

		size_t pos_separate_1000 = str1.find(L",");
		size_t pos_separate_decimal = str1.find(L".");

		if (pos_separate_decimal != std::wstring::npos)
		{
			std::wstring str2;
			boost::wregex re2(L"([^0]+)");

			str2 = boost::regex_replace(str1.substr(pos_separate_decimal), re2, L"", boost::match_default | boost::format_all);

			if (false == str2.empty())
				min_decimal = str2.length();

			str1 = str1.substr(0, pos_separate_decimal);
		}
		else
		{
			min_decimal = 0;
		}
		{
			std::wstring str2;
			boost::wregex re2(L"([^0]+)");

			str2 = boost::regex_replace(str1, re2, L"", boost::match_default | boost::format_all);

			if (false == str2.empty())
				min_digit = str2.length();
		}
		if (pos_separate_1000 != std::wstring::npos)
		{
			separate = str1.length() - pos_separate_1000 - 1;
		}

		if (bText && root_elm)
		{
			int res1 = (int)splits[indText].find(L"\"");
			int res2 = (int)splits[indText].find(L"\"", res1 + 1);

			if (res2 > 0)
			{
				std::wstring text = splits[indText].substr(res1 + 1, res2 - res1 - 1);

				if (!text.empty())
				{
					if (indText < indNumber) text = text + L" ";
					else					 text = L" " + text;

					create_element(L"number", L"text", elm_text, odf_context_);
					number_text* number_text_ = dynamic_cast<number_text*>(elm_text.get());

					if (number_text_)
						number_text_->add_text(text);
				}
			}
		}
	}

	number_number* number_ = dynamic_cast<number_number*>(elm.get());
	if (number_)
	{
		number_->number_min_integer_digits_ = min_digit;
		number_->number_decimal_places_ = min_decimal.get_value_or(0);

		if (separate)
			number_->number_grouping_ = true;
	}
	number_scientific_number* scientific_ = dynamic_cast<number_scientific_number*>(elm.get());
	if (scientific_)
	{
		scientific_->number_min_integer_digits_ = min_digit;
		scientific_->number_decimal_places_ = min_decimal;
		scientific_->number_min_exponent_digits_ = min_digit;
		scientific_->number_min_decimal_places_ = min_decimal;
		scientific_->number_forced_exponent_sign_ = true;

		if (separate)
			scientific_->number_grouping_ = true;
	}
	number_fraction* fraction_ = dynamic_cast<number_fraction*>(elm.get());
	if (fraction_)
	{
		fraction_->number_min_integer_digits_ = 0;
		fraction_->number_min_numerator_digits_ = min_digit;
		fraction_->number_min_denominator_digits_ = min_digit;

		if (separate)
			fraction_->number_grouping_ = true;
	}
	if (root_elm)
	{
		if (bText)
		{
			if (indText < indNumber)
			{
				root_elm->add_child_element(elm_text);
				root_elm->add_child_element(elm);
			}
			else
			{
				root_elm->add_child_element(elm);
				root_elm->add_child_element(elm_text);
			}
		}
		else
		{
			root_elm->add_child_element(elm);
		}
	}
}
void odf_number_styles_context::create_percentage_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"percentage-style", root_elm, odf_context_);
	
	office_element_ptr elm;

    office_element_ptr empty;
    create_numbers(state, elm, empty);
	root_elm->add_child_element(elm); 

	office_element_ptr elm_text;
	create_element(L"number", L"text", elm_text, odf_context_);
	number_text* number_text_ = dynamic_cast<number_text*>(elm_text.get());
	if (number_text_)number_text_->add_text(L"%"); 	

	root_elm->add_child_element(elm_text);
	styles_elments.push_back(elm_text);
}

void odf_number_styles_context::create_currency_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"currency-style", root_elm, odf_context_);
	{
        int res1 = (int)state.format_code[0].rfind(L"[");
        int res2 = (int)state.format_code[0].rfind(L"#");
        int res3 = (int)state.format_code[0].rfind(L"0");
		int res4 = (int)state.format_code[0].rfind(L"]");

		if (res1 >= 0 && res4 >= 0)
		{
			state.format_code[0].erase(state.format_code[0].begin() + res1, state.format_code[0].begin() + res4 + 1);
			
			std::vector<std::wstring> tmp;
			boost::algorithm::split(tmp, state.format_code[0], boost::algorithm::is_any_of(L"\\"), boost::algorithm::token_compress_on);
			for (size_t i = 0; i < tmp.size(); ++i)
			{
				if (!tmp[i].empty())
				{
					state.format_code[0] = tmp[i];
					break;
				}
			}
		}
		std::wstring number_country, number_language;

		for (long i = 0; state.language_code > 0 && i < sizeof(LanguageCodeTable) / sizeof(def_language_code); i++)
		{
			if (LanguageCodeTable[i].id == state.language_code)
			{
				number_country = LanguageCodeTable[i].country_code;
				number_language = LanguageCodeTable[i].language_code;
				break;
			}
		}
		//number_currency_style* currency_style_ = dynamic_cast<number_currency_style*>(root_elm.get());
		//if (currency_style_)
		//{
		//	if (false == number_country.empty())
		//	{
		//		currency_style_->common_data_style_attlist_.number_country_ = number_country;
		//	}
		//	if (number_language.length() > 0)
		//	{
		//		currency_style_->common_data_style_attlist_.number_language_ = number_language;
		//	}
		//}
		office_element_ptr elm_symbol;
		create_element(L"number", L"currency-symbol", elm_symbol, odf_context_);
		styles_elments.push_back(elm_symbol);

		number_currency_symbol* number_currency_symbol_ = dynamic_cast<number_currency_symbol*>(elm_symbol.get());
		if (number_currency_symbol_)
		{
			if (false == number_country.empty())
			{
				number_currency_symbol_->number_country_ = number_country;
			}			
			if (number_language.length()>0)
			{
				number_currency_symbol_->number_language_ = number_language;
			}
			number_currency_symbol_->add_text(state.currency_str);
		}
///////////////////
		office_element_ptr elm_number;
        office_element_ptr empty;
        create_numbers(state, elm_number, empty);
//////////////////////////////////////////
		office_element_ptr elm_text;
		create_element(L"number", L"text", elm_text, odf_context_);
		number_text* number_text_ = dynamic_cast<number_text*>(elm_text.get());		
		if (number_text_) number_text_->add_text(L" "); 
		styles_elments.push_back(elm_text);
////////////////////////////////////////////
		if (res1 > res2 || res1 > res3)
		{
			root_elm->add_child_element(elm_number);  
			root_elm->add_child_element(elm_text);
			root_elm->add_child_element(elm_symbol);
		}
		else
		{
			root_elm->add_child_element(elm_symbol);
			root_elm->add_child_element(elm_text);
			root_elm->add_child_element(elm_number);  
		}
	}
}

void odf_number_styles_context::create_date_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"date-style", root_elm, odf_context_);
	number_date_style *date_ = dynamic_cast<number_date_style*>(root_elm.get());

	std::wstring number_country, number_language;

	for (long i = 0; state.language_code > 0 && i < sizeof(LanguageCodeTable) / sizeof(def_language_code); i++)
	{
		if (LanguageCodeTable[i].id == state.language_code)
		{
			number_country = LanguageCodeTable[i].country_code;
			number_language = LanguageCodeTable[i].language_code;
			break;
		}
	}
	if (false == number_country.empty())
	{
		date_->common_data_style_attlist_.number_country_ = number_country;
	}
	if (number_language.length() > 0)
	{
		date_->common_data_style_attlist_.number_language_ = number_language;
	}
//---------------------------------------------------------------------------------
	std::wstring s = state.format_code[0];
	
	boost::wregex re(L"([mMdDyYhHsS^(AM)^(PM)^(am)^(pm)]+)([^m^M^d^D^y^Y^h^H^s^S^(AM)^(PM)^(am)^(pm)]+)");
	
	std::list<std::wstring> result;
	bool b = boost::regex_split(std::back_inserter(result),s, re);
	result.push_back(s);

	size_t sz = 0;
	
	bool bDate = false;
	bool bTime = false;
	bool bAM = false;

	for (std::list<std::wstring>::iterator i = result.begin(); i != result.end(); ++i)
	{
		office_element_ptr elm;
		s = *i;
		if (s.empty()) continue;

		sz = s.length();
		
		if (std::wstring::npos != s.find(L"AM") || std::wstring::npos != s.find(L"am"))
		{
			bAM = true;
		}
		else if (std::wstring::npos != s.find(L"PM") || std::wstring::npos != s.find(L"pm"))
		{
			if (bAM)
			{
				create_element(L"number", L"am-pm", elm, odf_context_);
				bAM = false;
			}
		}
		else if (std::wstring::npos != s.find(L"m") || std::wstring::npos != s.find(L"M"))
		{
			if (bDate)
			{
				create_element(L"number", L"month", elm, odf_context_);
				number_month* number_month_ = dynamic_cast<number_month*>(elm.get());

				if (number_month_ && sz > 2)number_month_->number_textual_ = true;
				if (sz == 1 || sz == 3) number_month_->number_style_ = L"short";
				if (sz == 2 || sz == 4) number_month_->number_style_ = L"long";
			}
			else
			{
				create_element(L"number", L"minutes", elm, odf_context_);
			}
		}
		else if (std::wstring::npos != s.find(L"h") || std::wstring::npos != s.find(L"H"))
		{
			bTime = true;
			bDate = false;

			create_element(L"number", L"hours", elm, odf_context_);
		}
		else if (std::wstring::npos != s.find(L"s") || std::wstring::npos != s.find(L"S"))
		{
			bTime = true;
			bDate = false;

			create_element(L"number", L"seconds", elm, odf_context_);
		}
		else if (std::wstring::npos != s.find(L"d") || std::wstring::npos != s.find(L"D"))
		{
			bTime = false;
			bDate = true;
			
			if (sz < 3)
			{
				create_element(L"number", L"day", elm, odf_context_);
				number_day* number_day_ = dynamic_cast<number_day*>(elm.get());
				if (number_day_)
				{
					if (sz == 1) number_day_->number_style_ = L"short";
					if (sz == 2) number_day_->number_style_ = L"long";
				}
			}
			else
			{
				bTime = false;
				bDate = true;
				
				create_element(L"number", L"day-of-week", elm, odf_context_);
				number_day_of_week* number_day_of_week_ = dynamic_cast<number_day_of_week*>(elm.get());
				if (number_day_of_week_)
				{
					if (sz == 3) number_day_of_week_->number_style_ = L"short";
					if (sz == 4) number_day_of_week_->number_style_ = L"long";
				}
			}
		}
		else if (std::wstring::npos != s.find(L"y") || std::wstring::npos != s.find(L"Y"))
		{
			bTime = false;
			bDate = true;

			create_element(L"number", L"year", elm, odf_context_);
			number_year* number_year_ = dynamic_cast<number_year*>(elm.get());
			if (number_year_)
			{
				if (sz < 3)	number_year_->number_style_ = L"short";
				else		number_year_->number_style_ = L"long";
			}
		}
		else if (!bAM) // there is no am/pm separator in LibreOffice
		{	//////////////////// divider ////////////////////
			if(sz > 1) 
			{
				//Remove the "extra" slashes
				XmlUtils::replace_all( s, L"\\", L"");
			}
			create_element(L"number", L"text", elm, odf_context_);
			number_text* number_text_ = dynamic_cast<number_text*>(elm.get());
			if (number_text_)number_text_->add_text(s); 	
		}
		if (elm)
		{
			root_elm->add_child_element(elm);
			styles_elments.push_back(elm);

		}
	}
}

void odf_number_styles_context::create_time_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"time-style", root_elm, odf_context_);
	
	// state.language_code == L"F400")//System time format
	std::wstring s = state.format_code[0];
	XmlUtils::GetLower(s);
	
	boost::wregex re(L"([a-zA-Z]+)(\\W+)");//(L"(\\w+)");
	std::list<std::wstring> result;
	bool b = boost::regex_split(std::back_inserter(result),s, re);
	if (b)result.push_back(s);//the last one ..is taken out - that's how boost.regex_split works

	size_t sz=0;
	for (std::list<std::wstring>::iterator it = result.begin(); it != result.end(); ++it)
	{
		office_element_ptr elm;
		s = *it;
		sz = s.length();
		if (std::wstring::npos != s.find(L"h"))
		{
			create_element(L"number", L"hours", elm, odf_context_);
			number_hours* number_hours_ = dynamic_cast<number_hours*>(elm.get());		
			if (number_hours_)
			{
				if (sz == 1) number_hours_->number_style_ = L"short";
				if (sz == 2) number_hours_->number_style_ = L"long";
			}
		}
		else if (std::wstring::npos != s.find(L"am"))  //unnecessary..
		{
			create_element(L"number", L"am-pm", elm, odf_context_);
		}
		else if (std::wstring::npos != s.find(L"m") && std::wstring::npos == s.find(L"am") && std::wstring::npos == s.find(L"pm")) 
		{
			create_element(L"number", L"minutes", elm, odf_context_);
			number_minutes* number_minutes_ = dynamic_cast<number_minutes*>(elm.get());
			if (number_minutes_)
			{
				if (sz == 1)number_minutes_->number_style_ = L"short";
				if (sz == 2)number_minutes_->number_style_ = L"long";
			}
		}
		else if (std::wstring::npos != s.find(L"s"))
		{
			create_element(L"number", L"seconds", elm, odf_context_);
			number_seconds* number_seconds_ = dynamic_cast<number_seconds*>(elm.get());
			if (number_seconds_)
			{
				if (sz == 1)	number_seconds_->number_style_ = L"short";
				if (sz == 2)	number_seconds_->number_style_ = L"long";
				//number_decimal_places_
			}
		}
		else if (std::wstring::npos == s.find(L"pm"))//that's how the format is divided .. and we don't need this text
		{
		//////////////////// divider ////////////////////
			if(sz > 1) 
			{
				//Remove the "extra" slashes
				XmlUtils::replace_all( s, L"\\", L"");
			}
			create_element(L"number", L"text", elm, odf_context_);
			number_text* number_text_ = dynamic_cast<number_text*>(elm.get());
			if (number_text_)number_text_->add_text(s); 
		}
		if (elm)
		{
			root_elm->add_child_element(elm);
			styles_elments.push_back(elm);
		}
	}
}

void odf_number_styles_context::create_boolean_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"boolean-style", root_elm, odf_context_);
}

void odf_number_styles_context::create_text_style(number_format_state & state, office_element_ptr & root_elm)
{
	create_element(L"number", L"text-style", root_elm, odf_context_);
}
static std::wstring replace_unwanted(boost::wsmatch const & what)
{
	return L"";
}
void odf_number_styles_context::detect_format(number_format_state & state)
{
	if (state.ods_type != office_value_type::Custom) return;
	if (state.format_code.empty()) return;

	boost::wregex re_unwanted(L"(\\\\.{1})");
	
	std::wstring strFormatCode = boost::regex_replace(state.format_code[0], re_unwanted, &replace_unwanted,	boost::match_any | boost::format_all);

	// The locale suffix is optional, e.g. [$€] and [$€-407].
	boost::wregex re(L"(\\[\\$[^\\]]*\\])");
	
	std::vector<std::wstring> result;
	std::wstring tmp = strFormatCode;
	bool b = boost::regex_split(std::back_inserter(result), tmp, re);

	if (b && result.size() > 0)
	{//split Currency String		
		tmp = result[0].substr(1, result[0].size() - 2);
		size_t sep = tmp.find(L"-");
		if (sep != std::wstring::npos)
		{
			state.currency_str = tmp.substr(1, sep - 1);
			
			try
			{
				std::wstringstream ss;
				ss << std::hex << tmp.substr(sep + 1);
				ss >> state.language_code;
			}
			catch (...) {}
		}
		else
		{
			state.currency_str = tmp.substr(1);
		}
		if (false == state.currency_str.empty())
		{
			size_t pos_digit = (std::min)(strFormatCode.find(L"0"), strFormatCode.find(L"#"));
			size_t pos_currency = strFormatCode.find(state.currency_str);

			if (pos_digit != std::wstring::npos) state.currency_pos = pos_digit < pos_currency ? 1 : 2;
		}
		XmlUtils::replace_all(strFormatCode, result[0], L"");
	}

	if (state.format_code.size() > 0) //any
	{
		boost::wregex re1(L"([mM]{2,}|[hH]{1,}|[sS]{2,})");
		boost::wregex re2(L"([M]{2,}|[m]{1,}|[dD]{1,}|[yY]{2,})");

		tmp = strFormatCode;
		
		std::vector<std::wstring> result1;
		bool b1 = boost::regex_split(std::back_inserter(result1), tmp, re1);

		tmp = strFormatCode;
		std::vector<std::wstring> result2;
		bool b2 = boost::regex_split(std::back_inserter(result2), tmp, re2);
		
		if (b1 && b2 && result1.size() > 2 && result2.size() > 2)
		{
			state.ods_type = office_value_type::DateTime;
			return;
		}
		if (b1 && result1.size() > 2)
		{
			state.ods_type = office_value_type::Time;
			return;
		}
		if (b2 && result2.size() > 1)
		{
			state.ods_type = office_value_type::Date;
			return;
		}
		if (!state.currency_str.empty() && state.language_code != 0xF400 && state.language_code != 0xF800)
		{
			state.ods_type = office_value_type::Currency;
			return;
		}
		if (std::wstring::npos != strFormatCode.find(L"%"))
		{
			state.ods_type = office_value_type::Percentage;
			return;
		}
		if (std::wstring::npos != strFormatCode.find(L"#") ||
			std::wstring::npos != strFormatCode.find(L"?") ||
			std::wstring::npos != strFormatCode.find(L"0"))
		{
			state.ods_type = office_value_type::Float;
			return;
		}
	}
}
}
}
