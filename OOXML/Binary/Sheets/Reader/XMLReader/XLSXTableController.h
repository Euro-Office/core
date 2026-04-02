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

#include "../CellFormatController/CellFormatController.h"

#include "../../../../Base/Base.h"
#include "../../../../XlsxFormat/Worksheets/Worksheet.h"
#include "../../../../XlsxFormat/Xlsx.h"

#include <string>
#include <vector>

/// @brief wrapper class that allows adding cells to a table and creating a document from created cells
class XLSXTableController
{

public:
    /// @brief object fields initialization
    /// @param book object that will be filled with data using FormBook method
    /// @param lcid locale identifier
    XLSXTableController(OOX::Spreadsheet::CXlsx &book, _INT32 lcid);

    /// @brief add cell
    /// @param sText text to insert
    /// @param nRow row number
    /// @param nCol column number
    /// @param bIsWrap wrap flag
    void AddCell(const std::wstring &sText, INT nRow, INT nCol);

    /// @brief get xlsx document
    void FormBook();

private:
    /// @brief add row
    /// @param pRow pointer to row
    /// @param pWorkSheet pointer to worksheet
    /// @param nRow row number
    _UINT32 addRow(OOX::Spreadsheet::CRow *pRow, OOX::Spreadsheet::CWorksheet *pWorkSheet,  INT nRow);

    /// @brief add page
    /// @param page pointer to worksheet
    /// @param pageNumber page number
    void addPage(OOX::Spreadsheet::CWorksheet *page, INT pageNumber);

    /// @brief xlsx document
    OOX::Spreadsheet::CXlsx *book_;

    /// @brief vector with table rows
    std::vector<OOX::Spreadsheet::CRow*> tableRows_;

    /// @brief format controller
    std::shared_ptr<CellFormatController> formates_;
};
