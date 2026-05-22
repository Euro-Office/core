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

#include "Fonts.h"
#include "../../common/File.h"
#include "../../common/StringExt.h"
#include "../../fontengine/ApplicationFonts.h"
#include "./pro_base.cpp"

NSFonts::IFontsMemoryStorage* g_global_fonts_memory_storage = NULL;

namespace NSFonts
{
	CLibrary::CLibrary()
	{
		m_internal = new CLibrary_private();
		m_internal->m_library = NULL;
	}
	CLibrary::~CLibrary()
	{
		RELEASEOBJECT(m_internal);
	}
}

namespace NSFonts
{
	IFontPath::IFontPath() : NSBase::CBaseRefCounter() {}
	IFontPath::~IFontPath() {}
	namespace NSFontPath
	{
		IFontPath* Create()
		{
			return new CFontPath();
		}
	}

	IFontStream::IFontStream() : NSBase::CBaseRefCounter() {}
	IFontStream::~IFontStream() {}
	namespace NSStream
	{
		IFontStream* Create()
		{
			return new CFontStream();
		}
	}

	IApplicationFontStreams::IApplicationFontStreams() : NSBase::CBaseRefCounter() {}
	IApplicationFontStreams::~IApplicationFontStreams() {}

	IFontsMemoryStorage::IFontsMemoryStorage() {}
	IFontsMemoryStorage::~IFontsMemoryStorage() {}

	namespace NSApplicationFontStream
	{
		IApplicationFontStreams* Create()
		{
			return new CApplicationFontStreams();
		}

		// default global memory storage
		class CDefaultGlobalFontsMemoryStorage : public IFontsMemoryStorage
		{
		private:
			std::map<std::wstring, IFontStream*> m_mapStreams;
			int m_nInternalCounter;

		public:
			CDefaultGlobalFontsMemoryStorage()
			{
				m_nInternalCounter = 1;
			}
			virtual ~CDefaultGlobalFontsMemoryStorage()
			{
				Clear();
			}

			virtual bool Add(const std::wstring& id, BYTE* data, LONG size, bool bClear = false)
			{
				std::wstring sFile = id;
				NSStringExt::Replace(sFile, L"\\", L"/");
				std::map<std::wstring, IFontStream*>::iterator it = m_mapStreams.find(sFile);
				if (it != m_mapStreams.end())
					return false;

				IFontStream* pStream = NSFonts::NSStream::Create();
				pStream->CreateFromMemory(data, size, bClear);
				m_mapStreams.insert(std::pair<std::wstring, IFontStream*>(sFile, pStream));
				return true;
			}
			virtual bool Remove(const std::wstring& id)
			{
				std::wstring sFile = id;
				NSStringExt::Replace(sFile, L"\\", L"/");
				std::map<std::wstring, IFontStream*>::iterator it = m_mapStreams.find(sFile);
				if (it == m_mapStreams.end())
					return false;

				RELEASEINTERFACE(it->second);
				m_mapStreams.erase(it);
				return true;
			}
			virtual void Clear()
			{
				for (std::map<std::wstring, IFontStream*>::iterator it = m_mapStreams.begin(); it != m_mapStreams.end(); it++)
					RELEASEINTERFACE(it->second);
				m_mapStreams.clear();
			}

			virtual IFontStream* Get(const std::wstring& id)
			{
				std::wstring sFile = id;
				NSStringExt::Replace(sFile, L"\\", L"/");
				std::map<std::wstring, IFontStream*>::iterator it = m_mapStreams.find(sFile);
				return it != m_mapStreams.end() ? it->second : NULL;
			}

			virtual std::wstring GenerateId()
			{
				std::wstring sKey;
				do
				{
					sKey = L"storage_internal_" + std::to_wstring(m_nInternalCounter++);
				} while (m_mapStreams.find(sKey) != m_mapStreams.end());
				return sKey;
			}
		};

		GRAPHICS_DECL IFontsMemoryStorage* CreateDefaultGlobalMemoryStorage()
		{
			return new CDefaultGlobalFontsMemoryStorage();
		}

		GRAPHICS_DECL IFontsMemoryStorage* GetGlobalMemoryStorage()
		{
			return g_global_fonts_memory_storage;
		}
		GRAPHICS_DECL void SetGlobalMemoryStorage(IFontsMemoryStorage* pStorage)
		{
			if (g_global_fonts_memory_storage != pStorage)
				RELEASEINTERFACE(g_global_fonts_memory_storage);

			g_global_fonts_memory_storage = pStorage;
		}
	}

	IFontFile::IFontFile() : NSBase::CBaseRefCounter() {}
	IFontFile::~IFontFile() {}
	namespace NSFontFile
	{
		IFontFile* Create()
		{
			return new CFontFile();
		}
	}

	IFontsCache::IFontsCache() : NSBase::CBaseRefCounter() {}
	IFontsCache::~IFontsCache() {}
	namespace NSFontCache
	{
		IFontsCache* Create()
		{
			return new CFontsCache();
		}
	}

	IFontManager::IFontManager() : NSBase::CBaseRefCounter() {}
	IFontManager::~IFontManager() {}

	void IFontManager::CreateOwnerCache(const int& nCacheSize)
	{
		NSFonts::IFontsCache* pCache = NSFonts::NSFontCache::Create();
		pCache->SetStreams(GetApplication()->GetStreams());
		pCache->SetCacheSize(nCacheSize);
		SetOwnerCache(pCache);
	}

	namespace NSFontManager
	{
		IFontManager* Create()
		{
			return new CFontManager();
		}
	}

	IFontList::IFontList() : NSBase::CBaseRefCounter() {}
	IFontList::~IFontList() {}

	IApplicationFonts::IApplicationFonts() : NSBase::CBaseRefCounter() {}
	IApplicationFonts::~IApplicationFonts() {}
	namespace NSApplication
	{
		IApplicationFonts* Create()
		{
			return new CApplicationFonts();
		}
	}
}
