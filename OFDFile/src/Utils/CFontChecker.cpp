#include "CFontChecker.h"

#include "../Types/Font.h"

#include "../../../OfficeUtils/src/ZipFolder.h"

#ifdef BUILDING_WASM_MODULE
#include "../../../DesktopEditor/graphics/pro/Fonts.h"
#include "../../../PdfFile/SrcReader/FontsWasm.h"
#define FONTS_USE_ONLY_MEMORY_STREAMS
#endif

namespace OFD
{
CFontChecker::CFontChecker(NSFonts::IApplicationFonts* pApplicationFonts, IFolder*& pFolder)
	: m_pFontManager(nullptr), m_pFolder(pFolder)
{
	if (nullptr == pApplicationFonts)
		return;

	#ifdef BUILDING_WASM_MODULE
	m_pFontManager = pApplicationFonts->GenerateFontManager();

	if (nullptr == m_pFontManager)
		return;

	NSFonts::IFontsCache* pMeasurerCache = NSFonts::NSFontCache::Create();

	if (nullptr != pMeasurerCache)
	{
		pMeasurerCache->SetStreams(pApplicationFonts->GetStreams());
		m_pFontManager->SetOwnerCache(pMeasurerCache);
		pMeasurerCache->SetCacheSize(16);
	}
	#endif
}

void CFontChecker::Clear()
{
	m_arUpdatedFonts.clear();
}

bool CFontChecker::UpdateFont(CFont* pFont)
{
	#ifdef FONTS_USE_ONLY_MEMORY_STREAMS
	if (nullptr == pFont || !pFont->m_wsSelectedFont.empty())
		return false;

	if ((!m_arUpdatedFonts.empty() &&
	      m_arUpdatedFonts.cend() != std::find_if(m_arUpdatedFonts.cbegin(), m_arUpdatedFonts.cend(),
	                                              [pFont](const TUpdatedFont& oUpdatedFont)
	                                                      { if (oUpdatedFont.m_bLoadFromMemory) return
	                                                             pFont->m_wsFilePath == oUpdatedFont.m_pFont->m_wsFilePath;
	                                                         return
	                                                             pFont->m_wsFontName == oUpdatedFont.m_pFont->m_wsFontName &&
	                                                             pFont->m_bBold      == oUpdatedFont.m_pFont->m_bBold      &&
	                                                             pFont->m_bItalic    == oUpdatedFont.m_pFont->m_bItalic ; })))
		return false;



	// Embedded font
	if (!pFont->m_wsFilePath.empty() && nullptr != m_pFolder && NSFonts::NSApplicationFontStream::GetGlobalMemoryStorage())
	{
		const std::wstring wsTempFileName{NSFonts::NSApplicationFontStream::GetGlobalMemoryStorage()->GenerateId()};

		IFolder::CBuffer *pBuffer{nullptr};

		if (m_pFolder->exists(pFont->m_wsFilePath) && m_pFolder->read(pFont->m_wsFilePath, pBuffer) && nullptr != pBuffer)
		{
			bool bResult{false};

			if (NSFonts::NSApplicationFontStream::GetGlobalMemoryStorage()->Add(wsTempFileName, pBuffer->Buffer, (LONG)pBuffer->Size, true))
			{
				pFont->m_wsSelectedFont = wsTempFileName;

				m_arUpdatedFonts.push_back({true, pFont});

				bResult = true;
			}

			delete pBuffer;

			if (bResult)
				return true;
		}
	}

	if (nullptr == m_pFontManager)
		return false;

	// Font selection
	NSFonts::CFontSelectFormat oFormat;
	oFormat.wsName  = new std::wstring(pFont->m_wsFontName);
	oFormat.bBold   = new INT(pFont->m_bBold   ? TRUE : FALSE);
	oFormat.bItalic = new INT(pFont->m_bItalic ? TRUE : FALSE);
	NSFonts::CFontInfo* pFontInfo = m_pFontManager->GetFontInfoByParams(oFormat);

	if (nullptr != pFontInfo && !pFontInfo->m_wsFontPath.empty())
	{
		pFont->m_wsSelectedFont = pFontInfo->m_wsFontPath;

		if (NSWasm::IsJSEnv())
			pFont->m_wsSelectedFont = pFontInfo->m_wsFontName;

		if (!pFont->m_wsSelectedFont.empty())
		{
			pFont->m_wsSelectedFont = NSWasm::LoadFont(pFont->m_wsFilePath, pFontInfo->m_bBold, pFontInfo->m_bItalic);
			if (pFont->m_wsSelectedFont.empty())
			{
				// The font isn't ready yet, which means it's not being put to pRenderer at all. Later, when the font is loaded, the page will be redrawn.
				return false;
			}
		}

		m_arUpdatedFonts.push_back({false, pFont});

		return true;
	}
	#endif
	return false;
}

NSFonts::IApplicationFonts* CFontChecker::GetFonts() const
{
	#ifdef FONTS_USE_ONLY_MEMORY_STREAMS
	return (nullptr != m_pFontManager) ? m_pFontManager->GetApplication() : nullptr;
	#else
	return nullptr;
	#endif
}
}
