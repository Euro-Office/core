/*
 * (c) Copyright Ascensio System SIA 2010-2024
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
 * You can contact Ascensio System SIA at 20A-6 Ernesta Birznieka-Upish
 * street, Riga, Latvia, EU, LV-1050.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#pragma once

#include "../../../../Base/Base.h"

#include <string>
#include <vector>

namespace lcInfo
{

/// @brief class containing locale information and standards used in it
class LocalInfo
{
public:

    /// @brief build short date format from template
    /// @return template with numbers defining date element order where 0-1 days 2-3 months 4-5 years
    std::wstring GetShortDateFormat();

    /// @brief get month names in this locale
    /// @param index number from month list
    /// @param shortName whether abbreviated name is requested
    /// @return vector of month names starting from January
    std::vector<std::wstring> GetMonthNames(const _INT16 &index, const bool isShortName = false);

    /// @brief build short date format from template
    /// @return month number starting from zero on success, negative number on failure
    _INT16 GetMonthNumber(const std::wstring &monthName,  const bool isShortName = false);

    /// @brief get string month name in this locale
    /// @param index requested month number
    /// @param shortName whether abbreviated name is requested
    /// @return month name
    std::wstring GetLocMonthName(const _INT16 &index, bool shortName = false);

    /// @brief locale id
    _INT32 lcid;

    /// @brief locale name
    std::wstring Name;

    /// @brief date separator
    std::wstring DateSeparator;

    /// @brief short date
    std::wstring ShortDatePattern;

    /// @brief local month names
    _INT16 MonthNamesIndex;

    /// @brief maximum characters in abbreviated month length
    _INT16 MonthAbrvLen;
};

/// @brief get locale information by its id
LocalInfo getLocalInfo(const _INT32 lcid);

}
