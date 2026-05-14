/*
 * Copyright (C) Ascensio System SIA, 2009-2026
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation, together with the
 * additional terms provided in the LICENSE file.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For
 * details, see the GNU AGPL at: https://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA by email at info@onlyoffice.com
 * or by postal mail at 20A-6 Ernesta Birznieka-Upisha Street, Riga,
 * LV-1050, Latvia, European Union.
 *
 * The interactive user interfaces in modified versions of the Program
 * are required to display Appropriate Legal Notices in accordance with
 * Section 5 of the GNU AGPL version 3.
 *
 * No trademark rights are granted under this License.
 *
 * All non-code elements of the Product, including illustrations,
 * icon sets, and technical writing content, are licensed under the
 * Creative Commons Attribution-ShareAlike 4.0 International License:
 * https://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 * This license applies only to such non-code elements and does not
 * modify or replace the licensing terms applicable to the Program's
 * source code, which remains licensed under the GNU Affero General
 * Public License v3.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "Types.h"

#include "Utils.h"

namespace OFD
{
TBox::TBox()
	: m_dX(0.), m_dY(0.), m_dWidth(0.), m_dHeight(0.)
{}

bool TBox::Empty() const
{
	return m_dWidth < OFD_EPSILON || m_dHeight < OFD_EPSILON;
}

bool TBox::Read(const std::string& sValue)
{
	const std::vector<std::string> arValues{Split(sValue, ' ')};

	if (4 > arValues.size())
		return false;

	if (!StringToDouble(arValues[0], m_dX)     ||
	    !StringToDouble(arValues[1], m_dY)     ||
	    !StringToDouble(arValues[2], m_dWidth) ||
	    !StringToDouble(arValues[3], m_dHeight))
		return false;

	return true;
}

TMatrix::TMatrix()
	: m_dM11(1.), m_dM12(0.), m_dM21(0.), m_dM22(1.), m_dDx(0.), m_dDy(0.)
{}

bool TMatrix::Read(const std::string& sValue)
{
	const std::vector<std::string> arValues{Split(sValue, ' ')};

	if (6 > arValues.size())
		return false;

	if (!StringToDouble(arValues[0], m_dM11) ||
	    !StringToDouble(arValues[1], m_dM12) ||
	    !StringToDouble(arValues[2], m_dM21) ||
	    !StringToDouble(arValues[3], m_dM22) ||
	    !StringToDouble(arValues[4], m_dDx)  ||
	    !StringToDouble(arValues[5], m_dDy))
		return false;

	return true;
}

}
