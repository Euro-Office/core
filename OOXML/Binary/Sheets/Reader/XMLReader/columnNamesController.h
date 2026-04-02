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

#include  <utility>
#include <string>
#include <map>

/// @brief class that controls uniqueness of column names when opening xml documents
class ColumnNameController
{

public:
    /// @brief Create unique column name and set its number
    /// @param column string with column name
    /// @return number that this column will have
    _UINT32 CreateColumnName(std::wstring &column);

    /// @brief Search for column number with specified name
    /// @param columnName string with column name
    /// @return column number if exists, -1 if no such column
    _INT64 GetColumnNumber(const std::wstring &columnName);

    /// @brief Try to find and get original xml node name by unique one
    /// @param columnName unique node name created by controller
    /// @return non-unique xml node name from which unique was created
    std::wstring GetXmlName(const std::wstring &columnName);

    /// @brief Get all contained names and their column numbers
    /// @return map with unique names as keys and column numbers as values
    std::map<std::wstring, _UINT32> GetColumnNames();

private:

/// @brief stores unique column names as keys and pair of non-unique name and column number as value
std::map<std::wstring, std::pair<std::wstring, _UINT32>> colNames_;

/// @brief number added to repeating column names to make them unique
_UINT32 colNamePostfix_ = 2;

/// @brief maximum column number assigned when getting new name
_UINT32 colNumber_ = 0;
};
