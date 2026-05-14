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
#include "../../../Image.h"

namespace MetaFile
{
	class CMetaFile : public IMetaFile
	{
	public:
		CMetaFile(NSFonts::IApplicationFonts *pAppFonts) : IMetaFile(pAppFonts) {}
		virtual ~CMetaFile() {}

		virtual void SetImageSize(int nWidth, int nHeight) {}
		virtual bool LoadFromFile(const wchar_t* wsFilePath) { return false; }
		virtual bool LoadFromBuffer(BYTE* pBuffer, unsigned int unSize) { return false; }
		virtual bool LoadFromString(const std::wstring& data) { return false; }
		virtual bool DrawOnRenderer(IRenderer* pRenderer, double dX, double dY, double dWidth, double dHeight) { return false; }
		virtual void Close() {}
		virtual void GetBounds(double* pdX, double* pdY, double* pdW, double* pdH) {}
		virtual int GetType() { return 0; }
		virtual void ConvertToRaster(const wchar_t* wsOutFilePath, unsigned int unFileType, int nWidth, int nHeight = -1) {}
		virtual NSFonts::IFontManager* get_FontManager() { return NULL; }

		virtual std::wstring ConvertToSvg(unsigned int unWidth = 0, unsigned int unHeight = 0) { return L""; }
		virtual void SetTempDirectory(const std::wstring& dir) {}

		virtual void ConvertToXml(const wchar_t* wsFilePath) {}
		virtual void ConvertToXmlAndRaster(const wchar_t *wsXmlFilePath, const wchar_t* wsOutFilePath, unsigned int unFileType, int nWidth, int nHeight = -1) {}

		virtual bool LoadFromXmlFile(const wchar_t* wsFilePath) { return false; }
		virtual void ConvertToEmf(const wchar_t *wsFilePath) {}
	};
	
	IMetaFile* Create(NSFonts::IApplicationFonts *pAppFonts)
	{
		return new CMetaFile(pAppFonts);
	}
}
