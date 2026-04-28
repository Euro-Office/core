#include "OFDFile_Private.h"

#include "../../OfficeUtils/src/OfficeUtils.h"
#include "../../OfficeUtils/src/ZipFolder.h"

#include "../../DesktopEditor/graphics/pro/Fonts.h"
#include "../../DesktopEditor/graphics/IRenderer.h"

#include "Utils/CFontChecker.h"
#include "Utils/Utils.h"

#ifdef BUILDING_WASM_MODULE
#include "../../DesktopEditor/graphics/pro/js/wasm/src/serialize.h"
#endif

COFDFile_Private::COFDFile_Private(NSFonts::IApplicationFonts* pFonts)
	: m_pFolder(nullptr), m_bIsTempDirOwner(false)
{
	m_pFontChecker = new OFD::CFontChecker(pFonts, m_pFolder);
}

COFDFile_Private::~COFDFile_Private()
{
	Close();

	if (m_bIsTempDirOwner && !m_wsTempDir.empty())
		NSDirectory::DeleteDirectory(m_wsTempDir);

	if (nullptr != m_pFontChecker)
		delete m_pFontChecker;
}

void COFDFile_Private::Close()
{
	if (nullptr != m_pFolder)
	{
		delete m_pFolder;
		m_pFolder = nullptr;
	}

	if (nullptr != m_pFontChecker)
		m_pFontChecker->Clear();
}

void COFDFile_Private::SetTempDir(const std::wstring& wsPath)
{
	m_wsTempDir       = wsPath;
	m_bIsTempDirOwner = m_wsTempDir.empty();
}

std::wstring COFDFile_Private::GetTempDir() const
{
	return m_wsTempDir;
}

bool COFDFile_Private::Read()
{
	if (nullptr == m_pFolder)
		return false;

	return m_oBase.Read(m_pFolder);
}

bool COFDFile_Private::LoadFromFile(const std::wstring& wsFilePath)
{
	Close();

	if (wsFilePath.empty())
		return false;

	if (m_wsTempDir.empty())
		m_wsTempDir = NSDirectory::CreateDirectoryWithUniqueName(NSDirectory::GetTempPath());

	COfficeUtils oUtils(NULL);

	if (S_OK != oUtils.ExtractToDirectory(wsFilePath, m_wsTempDir, NULL, 0))
		return false;

	m_pFolder = new CFolderSystem(m_wsTempDir);

	return Read();
}

bool COFDFile_Private::LoadFromMemory(BYTE* pData, DWORD ulLength)
{
	Close();

	if (nullptr == pData || 0 == ulLength)
		return false;

	m_pFolder = new CZipFolderMemory(pData, ulLength);

	return Read();
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
	m_oBase.UpdateFonts(m_pFontChecker);

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

	m_oBase.UpdateFonts(m_pFontChecker);

	m_oBase.DrawPage(pRenderer, nPageIndex);

	pRenderer->SetTransform(dM11, dM12, dM21, dM22, dDx, dDy);
}

NSFonts::IApplicationFonts* COFDFile_Private::GetFonts() const
{
	return (nullptr != m_pFontChecker) ? m_pFontChecker->GetFonts() : nullptr;
}

std::wstring COFDFile_Private::GetInfo() const
{
	std::wstring wsInfo{L"{"};

	wsInfo += m_oBase.GetInfo();

	wsInfo += L"}";

	return wsInfo;
}

unsigned char* COFDFile_Private::GetStructure() const
{
	return nullptr;
}

unsigned char* COFDFile_Private::GetLinks(int nPageIndex) const
{
	return m_oBase.GetLinks(nPageIndex);
}
