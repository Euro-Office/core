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
#ifndef _PDF_READER_FONT_H
#define _PDF_READER_FONT_H

#include <vector>
#include <string>
#include <map>

#include "../lib/xpdf/PDFDoc.h"
#include "../lib/xpdf/AcroForm.h"

#include "../../DesktopEditor/graphics/pro/Fonts.h"

#include "RendererOutputDev.h"
#include "PdfAnnot.h"

namespace PdfReader
{
struct CAnnotFontInfo
{
	std::wstring wsFontName;
	std::wstring wsFontPath;
	std::wstring wsActualFontName;
	bool bBold   = false;
	bool bItalic = false;
};

std::string GetRCFromDS(const std::string& sDS, Object* pContents, const std::vector<double>& arrCFromDA);
bool IsNeedCMap(PDFDoc* pDoc);
bool IsBaseFont(const std::wstring& wsName);
std::map<std::wstring, std::wstring> GetAllFonts(PDFDoc* pdfDoc, NSFonts::IFontManager* pFontManager, CPdfFontList* pFontList, bool bIsNeedCMap);
std::wstring GetFontData(PDFDoc* pdfDoc, NSFonts::IFontManager* pFontManager, CPdfFontList *pFontList, Object* oFontRef, std::string& sFontName, std::string& sActualFontName, bool& bBold, bool& bItalic, bool bIsNeedCMap = false);
bool GetFontFromAP(PDFDoc* pdfDoc, AcroFormField* pField, Object* oFontRef, std::string& sFontKey);
std::vector<CAnnotFontInfo> GetAnnotFontInfos(PDFDoc* pdfDoc, NSFonts::IFontManager* pFontManager, CPdfFontList* pFontList, Object* oAnnotRef);
std::map<std::wstring, std::wstring> GetFreeTextFont(PDFDoc* pdfDoc, NSFonts::IFontManager* pFontManager, CPdfFontList* pFontList, Object* oAnnotRef, std::vector<CAnnotMarkup::CFontData*>& arrRC);
bool FindFonts(Object* oStream, int nDepth, Object* oResFonts);
int CollectFontWidths(GfxFont* gfxFont, Dict* pFontDict, std::map<unsigned int, unsigned int>& mGIDToWidth);
void CheckFontStylePDF(std::wstring& sName, bool& bBold, bool& bItalic);
bool EraseSubsetTag(std::wstring& sFontName);
}

#endif // _PDF_READER_FONT_H
