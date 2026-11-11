/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
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
			int m_nComplements = -1;	// index of the XFComplements bag
		};
	}
} // namespace OOX
