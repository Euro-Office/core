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

#include "../../DocxFormat/File.h"

#include <map>
#include <vector>

namespace OOX
{
	namespace Spreadsheet
	{
		// xl/featurePropertyBag/featurePropertyBag.xml
		// Stores feature property bags; currently the only consumer is the cell checkbox
		// feature: an <xf> extLst references an XFComplements entry which resolves through
		// XFComplement -> XFControls (CellControl) to a bag of type "Checkbox".
		class CFeaturePropertyBagFile : public OOX::File
		{
		public:
			CFeaturePropertyBagFile(OOX::Document* pMain);
			CFeaturePropertyBagFile(OOX::Document* pMain, const CPath& oRootPath, const CPath& oPath);
			virtual ~CFeaturePropertyBagFile();

			virtual void read(const CPath& oPath);
			virtual void read(const CPath& oRootPath, const CPath& oPath);
			virtual void write(const CPath& oPath, const CPath& oDirectory, CContentTypes& oContent) const;

			virtual const OOX::FileType type() const;
			virtual const CPath DefaultDirectory() const;
			virtual const CPath DefaultFileName() const;

			// true if the XFComplements entry at nIndex resolves to a Checkbox cell control
			bool IsCheckboxComplement(int nIndex) const;

		private:
			struct Bag
			{
				std::wstring sType;
				std::map<std::wstring, int> mapBagIds;	// <bagId k="...">N</bagId>
				std::vector<int> arrMappedBagIds;		// <a k="MappedFeaturePropertyBags"><bagId>N</bagId>...</a>
			};
			std::vector<Bag> m_arrBags;
		};
	}
} // namespace OOX
