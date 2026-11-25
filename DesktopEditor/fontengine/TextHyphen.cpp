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

#include "./TextHyphen.h"

#include <map>

#include "hyphen/hyphen.h"

#ifndef HYPHEN_ENGINE_DISABLE_FILESYSTEM
#include "../common/Directory.h"
#endif

namespace NSHyphen
{
	class CEngine_private
	{
	public:
		static std::wstring m_sDirectory;

		int m_nCacheSize;
		std::map<int, HyphenDict*> m_mapDicts;

		int         m_nLastLang;
		HyphenDict* m_pLastDict;

		// Always work within the same memory block
		char* m_pHyphenVector;
		size_t m_nHyphenVectorSize;

		CEngine_private()
		{
			m_nHyphenVectorSize = 100;

			m_pHyphenVector = new char[m_nHyphenVectorSize];
			memset(m_pHyphenVector, 0, m_nHyphenVectorSize);

			m_nLastLang = 0;
			m_pLastDict = nullptr;

#ifndef HYPHEN_ENGINE_DISABLE_FILESYSTEM
			m_nCacheSize = 5;
#else
			m_nCacheSize = -1;
#endif
		}

		~CEngine_private()
		{
			delete [] m_pHyphenVector;
		}

		void AddLast()
		{
			if (m_pLastDict)
			{
				if (m_nCacheSize > 0 && m_mapDicts.size() == static_cast<size_t>(m_nCacheSize))
				{
					std::map<int, HyphenDict*>::iterator first = m_mapDicts.begin();
					delete first->second;
					m_mapDicts.erase(first);
				}
				m_mapDicts.insert(std::pair<int, HyphenDict*>{m_nLastLang, m_pLastDict});
			}
		}

		int LoadDictionary(const int& lang)
		{
			m_nLastLang = lang;
			m_pLastDict = nullptr;

#ifndef HYPHEN_ENGINE_DISABLE_FILESYSTEM
			std::wstring sFilePath = GetDictionaryPath(m_nLastLang);
			if (m_sDirectory.empty())
				m_sDirectory = NSFile::GetProcessDirectory() + L"/dictionaries";

			if (NSFile::CFileBinary::Exists(sFilePath))
			{
				FILE* f = NSFile::CFileBinary::OpenFileNative(sFilePath, L"r");
				if (f == nullptr)
					return 1;

				m_pLastDict = hnj_hyphen_load_file(f);
				fclose(f);
			}
#endif

			AddLast();

			return (nullptr == m_pLastDict) ? 1 : 0;
		}
		int LoadDictionary(const int& lang, const unsigned char* data, const unsigned int& data_len)
		{
			m_nLastLang = lang;
			m_pLastDict = nullptr;

			m_pLastDict = hnj_hyphen_load_data(reinterpret_cast<const char*>(data), data_len);
			AddLast();

			return (nullptr == m_pLastDict) ? 1 : 0;
		}
#ifndef HYPHEN_ENGINE_DISABLE_FILESYSTEM
		std::wstring GetDictionaryPath(const int&  /*lang*/)
		{
			if (m_sDirectory.empty())
				m_sDirectory = NSFile::GetProcessDirectory() + L"/dictionaries";

			for (int i = 0; i < NSTextLanguages::DictionaryRec_count; ++i)
			{
				if (m_nLastLang == NSTextLanguages::Dictionaries[i].m_lang)
				{
					const char* sNameStr = NSTextLanguages::Dictionaries[i].m_name;
					std::wstring sNameU = NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)sNameStr, (LONG)(strlen(sNameStr)));
					std::wstring sFilePath = m_sDirectory + L"/" + sNameU + L"/hyph_" + sNameU + L".dic";
					return sFilePath;
				}
			}

			return L"";
		}
#endif
		char* Process(const int& lang, const char* word, const int& len)
		{
			// resize 2x
			if (static_cast<size_t>(len + 5) > m_nHyphenVectorSize)
			{
				delete[] m_pHyphenVector;
				m_nHyphenVectorSize = static_cast<size_t>(2) * (len + 5);
				m_pHyphenVector = new char[m_nHyphenVectorSize];

				memset(m_pHyphenVector, 0, m_nHyphenVectorSize);
			}
			else
			{
				// reset after last use
				char* mem = m_pHyphenVector;
				while (*mem)
					*mem++ = 0;
			}

			if (m_nLastLang != lang)
			{
				std::map<int, HyphenDict*>::iterator find = m_mapDicts.find(lang);
				if (find != m_mapDicts.end())
				{
					m_nLastLang = lang;
					m_pLastDict = find->second;
				}
				else
				{
#ifndef HYPHEN_ENGINE_DISABLE_FILESYSTEM
					LoadDictionary(lang);
#else
					m_nLastLang = lang;
					m_pLastDict = NULL;
#endif
				}
			}

			if (m_pLastDict)
			{
				char **rep = nullptr;
				int *pos = nullptr;
				int *cut = nullptr;

				hnj_hyphen_hyphenate2(m_pLastDict, word, len, m_pHyphenVector, nullptr, &rep, &pos, &cut);
				return m_pHyphenVector;
			}

			return nullptr;
		}
	};

	std::wstring CEngine_private::m_sDirectory = L"";

	CEngine::CEngine()
	{
		m_internal = new CEngine_private();
	}
	CEngine::~CEngine()
	{
		delete m_internal;
	}

	void CEngine::Init(const std::wstring& directory)
	{
		CEngine_private::m_sDirectory = directory;
	}

	void CEngine::SetCacheSize(const int& size)
	{
#ifndef HYPHEN_ENGINE_DISABLE_FILESYSTEM
		if (0 == m_internal->m_mapDicts.size())
			m_internal->m_nCacheSize = size;
#endif
	}

	int CEngine::LoadDictionary(const int& lang)
	{
		return m_internal->LoadDictionary(lang);
	}
	int CEngine::LoadDictionary(const int& lang, const unsigned char* data, const unsigned int& data_len)
	{
		return m_internal->LoadDictionary(lang, data, data_len);
	}
	char* CEngine::Process(const int& lang, const char* word, const int& len)
	{
		return m_internal->Process(lang, word, (len == -1) ? strlen(word) : len);
	}
	bool CEngine::IsDictionaryExist(const int& lang)
	{
		for (int i = 0; i < NSTextLanguages::DictionaryRec_count; ++i)
		{
			if (lang == NSTextLanguages::Dictionaries[i].m_lang)
				return true;
		}
		return false;
	}
}
