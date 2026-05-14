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

#include "OFDFile_Private.h"

#include "../../OfficeUtils/src/OfficeUtils.h"
#include "Utils/Utils.h"

COFDFile_Private::COFDFile_Private(NSFonts::IApplicationFonts* pFonts)
	: m_pAppFonts(pFonts), m_pFontManager(nullptr), m_pTempFolder(nullptr)
{
	if (nullptr == pFonts)
		return;

	// Create font manager with its own cache
	m_pFontManager = pFonts->GenerateFontManager();
	NSFonts::IFontsCache* pMeasurerCache = NSFonts::NSFontCache::Create();
	pMeasurerCache->SetStreams(pFonts->GetStreams());
	m_pFontManager->SetOwnerCache(pMeasurerCache);
	pMeasurerCache->SetCacheSize(16);
}

COFDFile_Private::~COFDFile_Private()
{
	Close();

	if (nullptr != m_pTempFolder)
		delete m_pTempFolder;

	RELEASEINTERFACE(m_pFontManager);
}

void COFDFile_Private::Close()
{}

void COFDFile_Private::SetTempDir(const std::wstring& wsPath)
{
	if (nullptr != m_pTempFolder)
		delete m_pTempFolder;

	if (!NSDirectory::Exists(wsPath))
		NSDirectory::CreateDirectory(wsPath);

	int nCounter = 0;
	std::wstring wsTempFolder = wsPath + L"/OFD/";

	while (NSDirectory::Exists(wsTempFolder))
	{
		wsTempFolder = wsPath + L"/OFD" + std::to_wstring(nCounter) + L'/';
		nCounter++;
	}

	NSDirectory::CreateDirectory(wsTempFolder);

	m_pTempFolder = new CFolderSystem(wsTempFolder);
}

std::wstring COFDFile_Private::GetTempDir() const
{
	return (nullptr != m_pTempFolder) ? m_pTempFolder->getFullFilePath(L"") : std::wstring();
}

bool COFDFile_Private::Read(IFolder* pFolder)
{
	if (nullptr == pFolder)
		return false;

	return m_oBase.Read(pFolder);
}

bool COFDFile_Private::LoadFromFile(const std::wstring& wsFilePath)
{
	if (wsFilePath.empty() || nullptr == m_pTempFolder)
		return false;

	Close();

	COfficeUtils oUtils(NULL);

	if (S_OK != oUtils.ExtractToDirectory(wsFilePath, m_pTempFolder->getFullFilePath(L""), NULL, 0))
		return false;

	return Read(m_pTempFolder);
}

bool COFDFile_Private::LoadFromMemory(BYTE* pData, DWORD ulLength)
{
	Close();

	if (nullptr != m_pTempFolder)
		delete m_pTempFolder;

	m_pTempFolder = new CZipFolderMemory(pData, ulLength);

	return Read(m_pTempFolder);
}

unsigned int COFDFile_Private::GetPageCount() const
{
	return m_oBase.GetPageCount();
}

void COFDFile_Private::GetPageSize(int nPageIndex, double& dWidth, double& dHeight) const
{
	m_oBase.GetPageSize(nPageIndex, dWidth, dHeight);
}

void COFDFile_Private::DrawPage(IRenderer* pRenderer, int nPageIndex)
{
	m_oBase.DrawPage(pRenderer, nPageIndex);
}

void COFDFile_Private::DrawPage(IRenderer* pRenderer, int nPageIndex, const double& dX, const double& dY, const double& dWidth, const double& dHeight)
{
	if (nullptr == pRenderer)
		return;

	double dPageWidth = 0., dPageHeight = 0.;

	GetPageSize(nPageIndex, dPageWidth, dPageHeight);

	if (OFD::IsZeroValue(dPageWidth) || OFD::IsZeroValue(dPageHeight))
		return;

	double dM11, dM12, dM21, dM22, dDx, dDy;
	pRenderer->GetTransform(&dM11, &dM12, &dM21, &dM22, &dDx, &dDy);

	Aggplus::CMatrix oTransform(dM11, dM12, dM21, dM22, dDx, dDy);

	oTransform.Scale(dWidth / dPageWidth, dHeight / dPageHeight);
	oTransform.Translate(dX, dY);

	pRenderer->SetTransform(oTransform.sx(), oTransform.shy(), oTransform.shx(), oTransform.sy(), oTransform.tx(), oTransform.ty());

	m_oBase.DrawPage(pRenderer, nPageIndex);

	pRenderer->SetTransform(dM11, dM12, dM21, dM22, dDx, dDy);
}

NSFonts::IApplicationFonts* COFDFile_Private::GetFonts()
{
	return m_pAppFonts;
}
