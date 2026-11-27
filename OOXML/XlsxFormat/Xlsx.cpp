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
#include "Xlsx.h"

#include "../DocxFormat/App.h"
#include "../DocxFormat/Core.h"
#include "../DocxFormat/FileTypes.h"

#include "Workbook/Workbook.h"
#include "SharedStrings/SharedStrings.h"
#include "Styles/Styles.h"
#include "Worksheets/Worksheet.h"
#include "CalcChain/CalcChain.h"
#include "ExternalLinks/ExternalLinks.h"
#include "ExternalLinks/ExternalLinkPath.h"
#include "Pivot/PivotTable.h"
#include "Pivot/PivotCacheDefinition.h"
#include "Pivot/PivotCacheRecords.h"

#include "Styles/Borders.h"
#include "Styles/CellStyles.h"
#include "Styles/Xfs.h"
#include "Styles/Colors.h"
#include "Styles/dxf.h"
#include "Styles/Fills.h"
#include "Styles/Fonts.h"
#include "Styles/NumFmts.h"
#include "Styles/TableStyles.h"

#include "ComplexTypes_Spreadsheet.h"
#include "../../DesktopEditor/common/SystemUtils.h"
#include "../Binary/XlsbFormat/FileTypes_SpreadsheetBin.h"

#include "../../MsBinaryFile/XlsFile/Converter/xls_writer.h"
#include "../../MsBinaryFile/XlsFile/Format/Logic/WorkbookStreamObject.h"
#include "../../MsBinaryFile/XlsFile/Format/Logic/GlobalsSubstream.h"
#include "../../MsBinaryFile/XlsFile/Format/Logic/Biff_unions/PIVOTCACHE.h"
#include <algorithm>
#include <set>

#include <iomanip>

OOX::Spreadsheet::CXlsx::CXlsx() : OOX::IFileContainer(dynamic_cast<OOX::Document*>(this))
{
	init();
}
OOX::Spreadsheet::CXlsx::CXlsx(const CPath& oFilePath) : OOX::IFileContainer(dynamic_cast<OOX::Document*>(this))
{
	init();

	Read( oFilePath );
}
OOX::Spreadsheet::CXlsx::~CXlsx()
{
    if(bDeleteWorkbook)			RELEASEOBJECT(m_pWorkbook);
    if(bDeleteSharedStrings)	RELEASEOBJECT(m_pSharedStrings);
    if(bDeleteStyles)			RELEASEOBJECT(m_pStyles);
    if(bDeleteCalcChain)		RELEASEOBJECT(m_pCalcChain);
	
	for (size_t i = 0; i < m_arWorksheets.size(); ++i)
    {
        if (bDeleteWorksheets && m_arWorksheets[i])
		{
            delete m_arWorksheets[i];
		}
    }
	m_arWorksheets.clear();
	m_mapWorksheets.clear();
}	
void OOX::Spreadsheet::CXlsx::init()
{
    m_bSpreadsheets		= true;

    m_pApp				= nullptr;
    m_pCore				= nullptr;

    m_pWorkbook			= nullptr;
    m_pSharedStrings	= nullptr;
    m_pStyles			= nullptr;
    m_pCalcChain		= nullptr;
    m_pVbaProject		= nullptr;
    m_pJsaProject		= nullptr;
    m_pWorkbookComments = nullptr;
    m_pXlsbWriter       = nullptr;
    m_nLastReadRow      = 0;
    m_nLastReadCol      = -1;
    m_bNeedCalcChain    = true;

    bDeleteWorkbook			= false;
    bDeleteSharedStrings	= false;
    bDeleteStyles			= false;
    bDeleteCalcChain		= false;
    bDeleteWorksheets		= false;
    bDeleteVbaProject		= false;
    bDeleteJsaProject		= false;
}
bool OOX::Spreadsheet::CXlsx::ReadNative(const CPath& oFilePath)
{
	m_sDocumentPath = oFilePath.GetPath();

	OOX::CRels oRels(oFilePath / FILE_SEPARATOR_STR);
	IFileContainer::Read(oRels, oFilePath, oFilePath);

	if (!m_pWorkbook) return false;

	CXlsb* xlsb = dynamic_cast<CXlsb*>(this);
	if (xlsb)
	{
		xlsb->PrepareSi();
		xlsb->PrepareTableFormula();
	}

	return true;
}
bool OOX::Spreadsheet::CXlsx::Read(const CPath& oFilePath)
{
	_CP_LOG << L"start read and parsing: " << oFilePath.GetFilename() << std::endl;
	ReadNative(oFilePath);

	_CP_LOG << L"end read ... " << std::endl;

	if (!m_pWorkbook) return false;

	for (size_t i = 0; i < m_arWorksheets.size(); ++i)
	{
		if (m_arWorksheets[i])
			m_arWorksheets[i]->PrepareAfterRead();
	}
	return true;
}
bool OOX::Spreadsheet::CXlsx::WriteNative(const CPath& oDirPath, OOX::CContentTypes &oContentTypes)
{
    if (nullptr == m_pWorkbook)
		return false;

	IFileContainer::Write(oDirPath / L"", OOX::CPath(_T("")), oContentTypes);

	oContentTypes.Write(oDirPath);
	return true;
}
bool OOX::Spreadsheet::CXlsx::Write(const CPath& oDirPath, OOX::CContentTypes &oContentTypes)
{
    PrepareToWrite();
  
	if(nullptr == m_pWorkbook ||  m_arWorksheets.empty ())
        return false;

//CApp
	if(nullptr == m_pApp)
	{
		OOX::CApp* pApp = new OOX::CApp(this);
		pApp->SetDefaults();
		smart_ptr<OOX::File> pAppFile(pApp);
		Add(pAppFile);
	}

//CCore
	if(nullptr == m_pCore)
	{
		OOX::CCore* pCore = new OOX::CCore(this);
		pCore->SetDefaults();
		smart_ptr<OOX::File> pCoreFile(pCore);
		Add(pCoreFile);
	}

//xl
    CPath oXlPath = oDirPath / m_pWorkbook->DefaultDirectory();
    WriteWorkbook(oXlPath);

	IFileContainer::Write(oDirPath / L"" , OOX::CPath(_T("")), oContentTypes);

    oContentTypes.Write(oDirPath);
    return true;
}
bool OOX::Spreadsheet::CXlsx::WriteXLS(const CPath& oFilePath)
{
	PrepareToWrite();
	if(nullptr == m_pWorkbook ||  m_arWorksheets.empty ())
		return false;
	// Both lists are made once here; the globals (CWorkbook::toXLS), the sheet substreams and the cache streams are
	// all written from them, so they always agree.
	// The sheets in tab order, paired by r:id.
	const auto sheets = m_pWorkbook->GetSheetsForXLS();
	// with no <sheet> that resolves there is no sheet data at all: report an error, not a workbook of empty tabs
	if(std::none_of(sheets.begin(), sheets.end(), [](const CWorkbook::SheetForXLS& sheet) { return sheet.second != nullptr; }))
		return false;
	// The pivot caches that are written, in the order of their cache streams.
	const auto pivotCaches = m_pWorkbook->GetPivotCachesForXLS();
	auto workbookStream = new XLS::WorkbookStreamObject;
	auto workbookPtr = XLS::BaseObjectPtr(workbookStream);
	XlsWriter writer;
	writer.globalInfoPtr.reset(new XLS::GlobalWorkbookInfo(XLS::WorkbookStreamObject::DefaultCodePage, nullptr));
	// A pivot view refers to its cache by stream id (cacheId + 1, see CWorkbookPivotCache::toXLS) until SxView
	// replaces it with the cache's index through this map. Only the views of these caches are written.
	std::set<unsigned int> pivotCacheIds;
	for(size_t index = 0; index < pivotCaches.size(); ++index)
	{
		const unsigned int cacheId = pivotCaches[index].first->m_oCacheId->GetValue();
		pivotCacheIds.insert(cacheId);
		writer.globalInfoPtr->mapPivotCacheIndex.emplace(cacheId + 1, static_cast<int>(index));
	}
	workbookStream->m_GlobalsSubstream = m_pWorkbook->toXLS(sheets, pivotCaches);
	auto CastedGlobalsStram = static_cast<XLS::GlobalsSubstream*>(workbookStream->m_GlobalsSubstream.get());
	CastedGlobalsStram->global_info_ = writer.globalInfoPtr;
	for(const auto& sheet : sheets)
	{
		XLS::BaseObjectPtr sheetBin;
		if(sheet.second)
			sheetBin = sheet.second->toXLS(pivotCacheIds);
		else
		{
			// a <sheet> whose r:id resolves to no sheet part: written as an empty worksheet so that it keeps its
			// place (see CWorkbook::GetSheetsForXLS). Not attached to this document, so it is not added to m_arWorksheets.
			CWorksheet emptySheet(nullptr);
			sheetBin = emptySheet.toXLS(pivotCacheIds);
		}
		workbookStream->m_arWorksheetSubstream.push_back(sheetBin);
	}
	CastedGlobalsStram->m_arSUPBOOK.push_back(m_pWorkbook->WriteXtiRefsXLS());
	if(m_pSharedStrings != nullptr)
		m_pSharedStrings->toXLS(workbookStream->m_GlobalsSubstream);
	auto themesPtr = m_pWorkbook->Find(OOX::FileTypes::Theme);
	if(!(themesPtr->type() == OOX::FileTypes::Unknown))
	{	auto counter = 0;
		auto theme = static_cast<PPTX::Theme*>(themesPtr.GetPointer());
		for(auto themeColor : theme->themeElements.clrScheme.Scheme)
		{
			if(themeColor.first == L"lt1")
				counter = 0;
			else if(themeColor.first == L"dk1")
				counter = 1;
			else if(themeColor.first == L"lt2")
				counter = 2;
			else if(themeColor.first == L"dk2")
				counter = 3;
			else if(themeColor.first == L"accent1")
				counter = 4;
			else if(themeColor.first == L"accent2")
				counter = 5;
			else if(themeColor.first == L"accent3")
				counter = 6;
			else if(themeColor.first == L"accent4")
				counter = 7;
			else if(themeColor.first == L"accent5")
				counter = 8;
			else if(themeColor.first == L"accent6")
				counter = 9;
			else if(themeColor.first == L"hlink")
				counter = 10;
			else if(themeColor.first == L"folHlink")
				counter = 11;
			else continue;
			auto tempcolor = themeColor.second;
			std::wstringstream sStream;
			sStream << std::hex  << std::setw(6) << std::setfill(L'0')<< tempcolor.Color->GetARGB(0);
			writer.globalInfoPtr->RegisterPaletteColor(counter, sStream.str());
		}
	}
	if(m_pStyles != nullptr)
		m_pStyles->toXLS(workbookStream->m_GlobalsSubstream);

	writer.Open(oFilePath.GetPath());
	writer.WriteWorkbook(workbookPtr);

	if(!pivotCaches.empty())
	{
		for(const auto& cache : pivotCaches)
		{
			auto CachePtr = cache.second;
			const unsigned int streamId = cache.first->m_oCacheId->GetValue() + 1;
			auto XLSBinCache = CachePtr->m_oPivotCashDefinition->toXLS(streamId);
			auto castedCache = static_cast<XLS::PIVOTCACHE*>(XLSBinCache.get());
			auto cacheRecordsPtr = CachePtr->Find(OOX::SpreadsheetBin::FileTypes::PivotCacheRecordsBin);
			if(!(cacheRecordsPtr->type() == OOX::FileTypes::Unknown))
			{
				auto castedRecords = static_cast<CPivotCacheRecordsFile*>(cacheRecordsPtr.GetPointer());
				if(!castedRecords->m_oPivotCacheRecords.IsInit() && castedRecords->m_pData!= nullptr)
				{
					castedRecords->m_oPivotCacheRecords.Init();
					XmlUtils::CXmlLiteReader reader;
					reader.FromStringA((char*)castedRecords->m_pData, castedRecords->m_nDataLength);
					reader.ReadNextNode();
					castedRecords->m_oPivotCacheRecords->fromXML(reader);
				}
				if(castedRecords->m_oPivotCacheRecords.IsInit())
				for(auto CacheRecord : castedRecords->m_oPivotCacheRecords->m_arrItems)
				{
					castedCache->m_arDBB.push_back(CacheRecord->toXLS());
				}
			}


			writer.WritePivotCache(XLSBinCache, streamId);
		}
	}
	return true;
}
bool OOX::Spreadsheet::CXlsx::WriteWorkbook(const CPath&  /*oDirPath*/)
{
//Theme
	if (m_pTheme.IsInit())
	{
        smart_ptr<OOX::File> pThemeFile = m_pTheme.smart_dynamic_cast<OOX::File>();
		m_pWorkbook->Add(pThemeFile);
	}
//SharedStrings
    if(m_pSharedStrings && m_pSharedStrings->m_nCount > 0)
    {
        smart_ptr<OOX::File> pSharedStringsFile(m_pSharedStrings);
        bDeleteSharedStrings = false;
        m_pWorkbook->Add(pSharedStringsFile);
    }
//Styles
    if(m_pStyles)
    {
        smart_ptr<OOX::File> pStylesFile(m_pStyles);
        bDeleteStyles = false;
        m_pWorkbook->Add(pStylesFile);
    }

//Workbook
    smart_ptr<OOX::File> pWorkbookFile(m_pWorkbook);
    bDeleteWorkbook = false;
    Add(pWorkbookFile);
    return true;
}
void OOX::Spreadsheet::CXlsx::PrepareToWrite()
{
    if( m_pWorkbook)	m_pWorkbook->PrepareToWrite();
    if( m_pStyles)		m_pStyles->PrepareToWrite();

    for (size_t i = 0; i < m_arWorksheets.size(); ++i)
    {
        if (m_arWorksheets[i])
			m_arWorksheets[i]->PrepareToWrite();
    }
}

void OOX::Spreadsheet::CXlsx::CreateWorkbook ()
{
	if(bDeleteWorkbook)
		RELEASEOBJECT(m_pWorkbook);
	m_pWorkbook = new CWorkbook(nullptr);
	bDeleteWorkbook = true;
}

void OOX::Spreadsheet::CXlsx::CreateSharedStrings ()
{
	if(bDeleteSharedStrings)
		RELEASEOBJECT(m_pSharedStrings);
	m_pSharedStrings = new CSharedStrings(nullptr);
	bDeleteSharedStrings = true;
}

void OOX::Spreadsheet::CXlsx::CreateStyles ()
{
	if(bDeleteStyles)
		RELEASEOBJECT(m_pStyles);
	m_pStyles = new CStyles(nullptr);
	bDeleteStyles = true;
}
bool OOX::Spreadsheet::CXlsx::hasPivot()
{
	if (!m_pWorkbook) return false;
	
	smart_ptr<OOX::File> filePivotCacheDefinition = m_pWorkbook->Find(FileTypes::PivotCacheDefinition.RelationType());
	return filePivotCacheDefinition.IsInit();
}
PPTX::Theme* OOX::Spreadsheet::CXlsx::GetTheme () const
{
	return (PPTX::Theme  *)(m_pTheme.GetPointer());
}

void OOX::Spreadsheet::CXlsx::PrepareWorkbook()
{
	IFileContainer::m_mapEnumeratedGlobal.clear();
	if(!m_pWorkbook)
	{
		m_pWorkbook = new OOX::Spreadsheet::CWorkbook(this);
		m_pWorkbook->m_oWorkbookPr.Init();
		m_pWorkbook->m_oWorkbookPr->m_oDefaultThemeVersion.Init();
		m_pWorkbook->m_oWorkbookPr->m_oDefaultThemeVersion->SetValue(124226);
		m_pWorkbook->m_oBookViews.Init();
		
		OOX::Spreadsheet::CWorkbookView* pWorkbookView = new OOX::Spreadsheet::CWorkbookView();
		
		pWorkbookView->m_oXWindow.Init();
		pWorkbookView->m_oXWindow->SetValue(480);
		pWorkbookView->m_oYWindow.Init();
		pWorkbookView->m_oYWindow->SetValue(120);
		pWorkbookView->m_oWindowWidth.Init();
		pWorkbookView->m_oWindowWidth->SetValue(27795);
		pWorkbookView->m_oWindowHeight.Init();
		pWorkbookView->m_oWindowHeight->SetValue(12585);
		
		m_pWorkbook->m_oBookViews->m_arrItems.push_back(pWorkbookView);
	}
	//add sheet if there is none
	if (m_arWorksheets.empty())
	{
		OOX::Spreadsheet::CWorksheet* pWorksheet = new OOX::Spreadsheet::CWorksheet(this);
		
		pWorksheet->m_oDimension.Init();
		pWorksheet->m_oDimension->m_oRef = L"A1";
		pWorksheet->m_oSheetViews.Init();
		
		OOX::Spreadsheet::CSheetView* pSheetView = new OOX::Spreadsheet::CSheetView();
		
		pSheetView->m_oTabSelected.Init();
		pSheetView->m_oTabSelected->FromBool(true);
		pSheetView->m_oWorkbookViewId.Init();
		pSheetView->m_oWorkbookViewId->SetValue(0);
		
		pWorksheet->m_oSheetViews->m_arrItems.push_back(pSheetView);
		pWorksheet->m_oSheetFormatPr.Init();
		pWorksheet->m_oSheetFormatPr->m_oDefaultRowHeight = 15.;
		
		pWorksheet->m_oPageMargins.Init();
		pWorksheet->m_oPageMargins->m_oLeft.Init();
		pWorksheet->m_oPageMargins->m_oLeft->FromInches(0.7);
		pWorksheet->m_oPageMargins->m_oTop.Init();
		pWorksheet->m_oPageMargins->m_oTop->FromInches(0.7);
		pWorksheet->m_oPageMargins->m_oRight.Init();
		pWorksheet->m_oPageMargins->m_oRight->FromInches(0.7);
		pWorksheet->m_oPageMargins->m_oBottom.Init();
		pWorksheet->m_oPageMargins->m_oBottom->FromInches(0.7);
		pWorksheet->m_oPageMargins->m_oHeader.Init();
		pWorksheet->m_oPageMargins->m_oHeader->FromInches(0.3);
		pWorksheet->m_oPageMargins->m_oFooter.Init();
		pWorksheet->m_oPageMargins->m_oFooter->FromInches(0.3);
		// the constructor has put it into m_arWorksheets already

		smart_ptr<OOX::File> pWorksheetFile(pWorksheet);
		OOX::RId oRId = this->Add(pWorksheetFile);
		// the Editor.bin writer finds worksheets by rId, as for the CSV reader's sheet
		m_mapWorksheets.insert(std::make_pair(oRId.ToString(), pWorksheet));

		m_pWorkbook->m_oSheets.Init();
		
		OOX::Spreadsheet::CSheet* pSheet = new OOX::Spreadsheet::CSheet();
		pSheet->m_oName = L"Sheet1";
		pSheet->m_oSheetId.Init();
		pSheet->m_oSheetId->SetValue(1);
		pSheet->m_oRid.Init();
		pSheet->m_oRid->SetValue(oRId.ToString());
		
		m_pWorkbook->m_oSheets->m_arrItems.push_back(pSheet);
	}
	//Make sure that there are always zero styles and the first font always has the font and size
	if( m_pStyles )
	{
		//Fonts
		if(!m_pStyles->m_oFonts.IsInit())
			m_pStyles->m_oFonts.Init();
		
		if(m_pStyles->m_oFonts->m_arrItems.empty())
			m_pStyles->m_oFonts->AddFont(new OOX::Spreadsheet::CFont());
		
		OOX::Spreadsheet::CFont* pFont = m_pStyles->m_oFonts->m_arrItems.front();
		
		if(!pFont->m_oRFont.IsInit())
		{
			pFont->m_oRFont.Init();
			pFont->m_oRFont->m_sVal = _T("Calibri");

			if(!pFont->m_oScheme.IsInit())
			{
				pFont->m_oScheme.Init();
				pFont->m_oScheme->m_oFontScheme.Init();
				pFont->m_oScheme->m_oFontScheme->SetValue(SimpleTypes::Spreadsheet::fontschemeMinor);
			}
		}

		if(!pFont->m_oSz.IsInit() || !pFont->m_oSz->m_oVal.IsInit())
		{
			pFont->m_oSz.Init();
			pFont->m_oSz->m_oVal.Init();
			pFont->m_oSz->m_oVal->SetValue(11.0);
		}
		//Fills
		if(!m_pStyles->m_oFills.IsInit())
			m_pStyles->m_oFills.Init();
		
		if(m_pStyles->m_oFills->m_arrItems.empty())
			m_pStyles->m_oFills->m_arrItems.push_back(new OOX::Spreadsheet::CFill());

		OOX::Spreadsheet::CFill* pFill = m_pStyles->m_oFills->m_arrItems.front();
		if(!pFill->m_oGradientFill.IsInit())
		{
			if(!pFill->m_oPatternFill.IsInit())
				pFill->m_oPatternFill.Init();
			if(!pFill->m_oPatternFill->m_oPatternType.IsInit())
				pFill->m_oPatternFill->m_oPatternType.Init();
			pFill->m_oPatternFill->m_oPatternType->SetValue(SimpleTypes::Spreadsheet::patterntypeNone);
		}
		if(!m_pStyles->m_oBorders.IsInit())
			m_pStyles->m_oBorders.Init();
		if(m_pStyles->m_oBorders->m_arrItems.size() == 0)
			m_pStyles->m_oBorders->m_arrItems.push_back(new OOX::Spreadsheet::CBorder());

		//Xfs
		if(!m_pStyles->m_oCellXfs.IsInit())
			m_pStyles->m_oCellXfs.Init();
		
		if(m_pStyles->m_oCellXfs->m_arrItems.empty())
			m_pStyles->m_oCellXfs->m_arrItems.push_back(new OOX::Spreadsheet::CXfs());

		OOX::Spreadsheet::CXfs* pXfs = m_pStyles->m_oCellXfs->m_arrItems.front();
		if(!pXfs->m_oBorderId.IsInit())
		{
			pXfs->m_oBorderId.Init();
			pXfs->m_oBorderId->SetValue(0);
		}
		if(!pXfs->m_oFillId.IsInit())
		{
			pXfs->m_oFillId.Init();
			pXfs->m_oFillId->SetValue(0);
		}
		if(!pXfs->m_oFontId.IsInit())
		{
			pXfs->m_oFontId.Init();
			pXfs->m_oFontId->SetValue(0);
		}
		if(!pXfs->m_oNumFmtId.IsInit())
		{
			pXfs->m_oNumFmtId.Init();
			pXfs->m_oNumFmtId->SetValue(0);
		}
	}
	//Transfer <is> tags and cells with type str to sharedString if they weren't transferred when read
	for (size_t i = 0; i < m_arWorksheets.size(); ++i)
	{
		PrepareWorksheet(m_arWorksheets[i]);
	}
	//todo parses dates in iso 8601 format
}
void OOX::Spreadsheet::CXlsx::PrepareWorksheet(CWorksheet* pWorksheet)
{
	if (!pWorksheet) return;
	if (pWorksheet->m_bPrepareForBinaryWriter) return;

	pWorksheet->m_bPrepareForBinaryWriter = true;

	if(pWorksheet->m_oSheetData.IsInit())
	{
        std::vector<OOX::Spreadsheet::CRow*>& aRows = pWorksheet->m_oSheetData->m_arrItems;
		
        for(size_t i = 0; i < aRows.size(); ++i)
		{
            OOX::Spreadsheet::CRow* pRow = aRows[i];
			
            std::vector<OOX::Spreadsheet::CCell*> & aCells = pRow->m_arrItems;
			
            for(size_t j = 0; j < aCells.size(); ++j)
			{
                OOX::Spreadsheet::CCell* pCell = aCells[j];
				if (!pCell)continue;

				if(pCell->m_oType.IsInit())
				{
					if(SimpleTypes::Spreadsheet::celltypeInlineStr == pCell->m_oType->GetValue())
					{
						if(!m_pSharedStrings) CreateSharedStrings();
						
						OOX::Spreadsheet::CSi* pSi = pCell->m_oRichText.GetPointerEmptyNullable();
						if(nullptr != pSi)
						{
							int nIndex = m_pSharedStrings->AddSi(pSi);
							//change the cell value
							pCell->m_oValue.Init();
                            pCell->m_oValue->m_sText = std::to_wstring(nIndex);
							//change cell type
							pCell->m_oType.Init();
							pCell->m_oType->SetValue(SimpleTypes::Spreadsheet::celltypeSharedString);
						}
					}
					else if(SimpleTypes::Spreadsheet::celltypeStr == pCell->m_oType->GetValue() || SimpleTypes::Spreadsheet::celltypeError == pCell->m_oType->GetValue())
					{
						if (pCell->m_oValue.IsInit())
						{
							if(!m_pSharedStrings) CreateSharedStrings();

							//add to SharedStrings
							CSi* pSi = new CSi();
							CText* pText =  new CText();
							pText->m_sText = pCell->m_oValue->ToString();
							pSi->m_arrItems.push_back(pText);

							int nIndex = m_pSharedStrings->AddSi(pSi);
							//change the cell value
							pCell->m_oValue.Init();
							pCell->m_oValue->m_sText = std::to_wstring(nIndex);
							//change cell type
							if(SimpleTypes::Spreadsheet::celltypeStr == pCell->m_oType->GetValue())
							{
								pCell->m_oType.Init();
								pCell->m_oType->SetValue(SimpleTypes::Spreadsheet::celltypeSharedString);
							}
						}
						else
						{
							pCell->m_oValue.reset();
							pCell->m_oType.reset();
						}
					}
					else if(SimpleTypes::Spreadsheet::celltypeBool == pCell->m_oType->GetValue())
					{
						//Usually it is written 1/0, but sometimes it is written true/false
						if(pCell->m_oValue.IsInit())
						{
							SimpleTypes::COnOff oOnOff;
                            std::wstring sVal = pCell->m_oValue->ToString();
                            oOnOff.FromString(sVal.c_str());
							pCell->m_oValue.Init();
							if(oOnOff.ToBool())
								pCell->m_oValue->m_sText = _T("1");
							else
								pCell->m_oValue->m_sText = _T("0");
						}
					}
				}
				
			}
		}
	}
}
