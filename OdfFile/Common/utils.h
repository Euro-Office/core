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

namespace NSFonts
{
	class IApplicationFonts;
}

namespace _graphics_utils_
{
    bool GetResolution(const wchar_t* fileName, double & Width, double &Height);
	std::pair<double, double> calculate_size_symbol_win(std::wstring name, double size, bool italic, bool bold, std::wstring test_str = L"");
	std::pair<double, double> calculate_size_symbol_asc(std::wstring name, double size, bool italic, bool bold , NSFonts::IApplicationFonts *appFonts);
};

// Values of document properties, shared by the ODF reader (docProps/core.xml and custom.xml) and the ODF
// writer (meta.xml).
namespace _doc_props_utils_
{
	// An xsd:date or xsd:dateTime value as W3CDTF: YYYY-MM-DDThh:mm:ss, without the fraction, and with the
	// value's own time zone or "Z" when it has none (the wall-clock time, as LibreOffice writes it). Empty
	// when the value is no valid date (a day the month does not have, a time out of range, text after it) or
	// its year is outside 1601..9999 (Office reports such a core.xml as corrupt).
	std::wstring w3cdtf_date(const std::wstring & value);

	// The shortest text that reads back as the same number, with '.' as the decimal point. As in
	// LibreOffice, whole numbers below 2^53 and numbers with a decimal exponent from -14 to 14 are written
	// without an exponent (2000, not 2e+03). Empty for a number that is not finite.
	std::wstring shortest_number(double value);
}
