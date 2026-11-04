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

#include "../Xlsx.h"
#include "../XlsxFlat.h"
#include "../../XlsbFormat/Xlsb.h"

#include "../WritingElement.h"

#include "BookViews.h"
#include "CalcPr.h"
#include "DefinedNames.h"
#include "PreservedSheets.h"
#include "Sheets.h"
#include "WorkbookPr.h"
#include "ExternalReferences.h"

namespace OOX
{
	namespace Drawing
	{
		class COfficeArtExtensionList;
	}

	namespace Spreadsheet
	{
		class CPersonList;
		class CPivotCacheDefinitionFile;
		class CWorksheet;

	//raw child:
		//<customWorkbookViews>
		//<extLst>
		//<fileRecoveryPr>
		//<fileVersion>
		//<functionGroups>
		//<smartTagPr>
		//<smartTagTypes>
		//<webPublishing>
		//<webPublishObjects>

		class CWorkbookPivotCache : public WritingElement
		{
		public:
			WritingElement_AdditionMethods(CWorkbookPivotCache)
			WritingElement_XlsbConstructors(CWorkbookPivotCache)

			CWorkbookPivotCache();
			virtual ~CWorkbookPivotCache();

			virtual void fromXML(XmlUtils::CXmlNode& node);
			virtual std::wstring toXML() const;

			virtual void toXML(NSStringUtils::CStringBuilder& writer) const;
			virtual void fromXML(XmlUtils::CXmlLiteReader& oReader);

			XLS::BaseObjectPtr toBin();
            XLS::BaseObjectPtr toBin14();
			XLS::BaseObjectPtr toXLS();
			void fromBin(XLS::BaseObjectPtr& obj);
            void fromBin14(XLS::BaseObjectPtr& obj);
			virtual EElementType getType() const;

			void ReadAttributes(XLS::BaseObjectPtr& obj);
			void ReadAttributes(XmlUtils::CXmlLiteReader& oReader);

	//----------
			nullable<SimpleTypes::CUnsignedDecimalNumber>	m_oCacheId;
			nullable<SimpleTypes::CRelationshipId>			m_oRid;
		};

		class CWorkbookPivotCaches : public WritingElementWithChilds<CWorkbookPivotCache>
		{
		public:
			WritingElement_AdditionMethods(CWorkbookPivotCaches)
            WritingElement_XlsbConstructors(CWorkbookPivotCaches)
			CWorkbookPivotCaches(OOX::Document *pMain = nullptr);
			virtual ~CWorkbookPivotCaches();

			virtual void fromXML(XmlUtils::CXmlNode& node);
			virtual std::wstring toXML() const;

			virtual void toXML(NSStringUtils::CStringBuilder& writer) const;
			virtual void fromXML(XmlUtils::CXmlLiteReader& oReader);

			XLS::BaseObjectPtr toBin();
            XLS::BaseObjectPtr toBin14();
            void fromBin14(XLS::BaseObjectPtr& obj);
			void fromBin(XLS::BaseObjectPtr& obj);
			virtual EElementType getType() const;

            bool pivotCaches14 = false;
		};

//-----------------------------------------------------------------------------------------------------------

		class CWorkbook : public OOX::File, public OOX::IFileContainer, public WritingElement
		{
		public:
			CWorkbook(OOX::Document* pMain);
			CWorkbook(OOX::Document* pMain, const CPath& oRootPath, const CPath& oPath);
			virtual ~CWorkbook();

			void readBin(const CPath& oPath);
			XLS::BaseObjectPtr WriteBin() const;
			// A <sheet> with the worksheet or chart sheet part its r:id resolves to, or nullptr if it resolves to none.
			typedef std::pair<CSheet*, CWorksheet*> SheetForXLS;
			// A <pivotCache> with its pivot cache definition part.
			typedef std::pair<CWorkbookPivotCache*, CPivotCacheDefinitionFile*> PivotCacheForXLS;

			// The globals substream of an XLS. CXlsx::WriteXLS gets both lists once and writes the sheet substreams and
			// the cache streams from the same lists, so the n-th BoundSheet8 is the n-th sheet substream and the n-th
			// cache in the globals is the n-th cache stream.
			XLS::BaseObjectPtr toXLS(const std::vector<SheetForXLS>& sheets, const std::vector<PivotCacheForXLS>& pivotCaches) const;
			// Every <sheet>, in tab order (see SheetForXLS). A sheet part that no <sheet> points at is not written. A
			// <sheet> without a part still keeps its place (CXlsx::WriteXLS writes it as an empty worksheet): sheet
			// references in formulas and defined names are written as positions in <sheets> (CSheets::AddSheetRef),
			// so leaving it out would move every reference to a later sheet one tab to the left.
			std::vector<SheetForXLS> GetSheetsForXLS() const;
			// The <pivotCache>s that XLS export writes, in order; a cache's position here is its index in the XLS. Left
			// out: a cache without a cacheId or with the cacheId of an earlier written one; one whose XLS stream id (cacheId + 1)
			// does not fit the field a pivot view finds its cache by (SxView::iCache); one whose r:id does not resolve to
			// a pivot cache definition part or whose definition does not parse; a worksheet source without its range.
			// CWorksheet::toXLS writes only the pivot views of the caches listed here, so a cache is written whole or
			// not at all.
			std::vector<PivotCacheForXLS> GetPivotCachesForXLS() const;
			virtual void read(const CPath& oPath);
			virtual void read(const CPath& oRootPath, const CPath& oPath);
			virtual void fromXML(XmlUtils::CXmlNode& node);

			virtual std::wstring toXML() const;
			virtual void toXML(NSStringUtils::CStringBuilder& writer) const;
			virtual void fromXML(XmlUtils::CXmlLiteReader& oReader);
			virtual void write(const CPath& oPath, const CPath& oDirectory, CContentTypes& oContent) const;
			virtual const OOX::FileType type() const;

			virtual const CPath DefaultDirectory() const;
			virtual const CPath DefaultFileName() const;

			virtual EElementType getType () const;
			const CPath& GetReadPath();

			void PrepareToWrite();
			LONG GetActiveSheetIndex();
			XLS::BaseObjectPtr WriteXtiRefs() const;
			XLS::BaseObjectPtr WriteXtiRefsXLS() const;

			CPath											m_oReadPath;

			nullable<OOX::Spreadsheet::CWorkbookProtection>	m_oWorkbookProtection;
			nullable<OOX::Spreadsheet::CBookViews>			m_oBookViews;
            nullable<OOX::Spreadsheet::CDefinedNames>		m_oDefinedNames;
			nullable<OOX::Spreadsheet::CSheets>				m_oSheets;
			nullable<OOX::Spreadsheet::CWorkbookPr>			m_oWorkbookPr;
			nullable<OOX::Spreadsheet::CExternalReferences>	m_oExternalReferences;
            nullable<OOX::Spreadsheet::CCalcPr>				m_oCalcPr;
			nullable_string									m_oAppName;
			nullable<OOX::Drawing::COfficeArtExtensionList> m_oExtLst;
			nullable_string									m_oOleSize;
			nullable<OOX::Spreadsheet::CWorkbookPivotCaches>m_oPivotCaches;
			nullable<std::wstring>							m_oPivotCachesXml;
			nullable<OOX::Spreadsheet::CFileSharing>		m_oFileSharing;

			CPersonList*									m_pPersonList;
			bool											m_bMacroEnabled;

			// sheets the editor cannot show (Excel 4.0 macro sheets), carried through Editor.bin
			std::vector<CPreservedSheet>					m_arPreservedSheets;
		};

	} //Spreadsheet
} // namespace OOX

