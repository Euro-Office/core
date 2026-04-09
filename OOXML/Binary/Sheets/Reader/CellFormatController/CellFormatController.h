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
#include "../../../../XlsxFormat/Worksheets/Worksheet.h"
#include "../../../../XlsxFormat/Styles/Styles.h"
#include "DateReader.h"
#include "DigitReader.h"


/// @brief class that determines and corrects data type for table cell values
class CellFormatController
{

public:
    /// @brief constructor
    /// @param styles styles from the table
    CellFormatController(OOX::Spreadsheet::CStyles *styles, _INT32 lcid);

    /// @brief processes data inserted into table cell, converting to required type, and fills the cell
    /// @param pCell pointer to cell
    /// @param value data to insert in string format
    int ProcessCellType(OOX::Spreadsheet::CCell *pCell, const std::wstring &value, bool bIsWrap = false);


	/// @brief pointer to document worksheet
	OOX::Spreadsheet::CWorksheet *m_pWorksheet = nullptr;

private:
    bool isFormula(const std::wstring& formula);
    std::wstring ConvertFormulaArguments(const std::wstring& formula);
    /// @brief create style for specified format
    /// @param format value format
    void createFormatStyle(const std::wstring &format);

	/// @brief add custom column width for cell
	/// @param pCell cell for which width will be added
	/// @param width width value to set for column if it's greater than current
	void addCustomColWidth(OOX::Spreadsheet::CCell *pCell, double width);

    /// @brief pointer to cell being worked with
    OOX::Spreadsheet::CCell *pCell_;

    /// @brief map with data formats
    std::map<std::wstring, unsigned int> mapDataNumber_;

    /// @brief pointer to document styles
	OOX::Spreadsheet::CStyles *m_pStyles;

    /// @brief pointer to received string value
    const std::wstring *value_;

    /// @brief locale identifier
    _INT32 lcid_;

    DigitReader digitReader_;
    DateReader dateReader_;
};
