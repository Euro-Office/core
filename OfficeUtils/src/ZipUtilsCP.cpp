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
#include "ZipUtilsCP.h"
#include <mz.h>
#include <mz_zip.h>
#include <memory>
#include <algorithm>
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/Path.h"

#if !defined(_WIN32) && !defined (_WIN64)
#include <unistd.h>
#include <sys/stat.h>
#endif

#define WRITEBUFFERSIZE 8192
#define READBUFFERSIZE 8192

unzFile unzOpenHelp(const wchar_t* filename);

namespace ZLibZipUtils
{
	// internal class for C unzFile struct
	class CunzFileWrapped
	{
	public:
		CunzFileWrapped() = default;
		~CunzFileWrapped();

		// NOTE: calls Close() first
		void Open(const wchar_t* filename);

		// NOTE: calls Close() first
		void Open(BYTE* data, size_t len);

		int Close();

		// NOTE: unzfile will be released after Close() or ~CunzFileWrapped()!
		// return value will be NULL, if Open(...) fails
		unzFile Get();

	private:
		unzFile m_unzFile {nullptr};
		BUFFER_IO* m_buffer {nullptr};
	};

	CunzFileWrapped::~CunzFileWrapped()
	{
		Close();
	}

	void CunzFileWrapped::Open(const wchar_t* filename)
	{
		Close();
		if(filename != nullptr)
		{
			m_unzFile = unzOpenHelp(filename);
		}
	}

	void CunzFileWrapped::Open(BYTE* data, size_t len)
	{
		Close();
		m_buffer = new BUFFER_IO;
		if ((data != nullptr) && (len != 0))
		{
			m_buffer->buffer = data;
			m_buffer->nSize  = len;

			zlib_filefunc_def ffunc;
			fill_buffer_filefunc(&ffunc, m_buffer);
			m_unzFile = unzOpen2(nullptr, &ffunc);
		}
	}
	int CunzFileWrapped::Close()
	{
		int err = 0;
		if(m_unzFile)
			err = unzClose(m_unzFile);
		m_unzFile = nullptr;

		RELEASEOBJECT(m_buffer)
		return err;
	}

	unzFile CunzFileWrapped::Get()
	{
		return m_unzFile;
	}

	zipFile zipOpenHelp(const wchar_t* filename)
	{
#ifdef _IOS
		std::string filePath = NSFile::NSIOS::GetFileSystemRepresentation(filename);
		zipFile zf = filePath.empty() ? NULL : zipOpen(filePath.c_str(), APPEND_STATUS_CREATE);
#else
		BYTE* pUtf8 = nullptr;
		LONG lLen = 0;
		NSFile::CUtf8Converter::GetUtf8StringFromUnicode(filename, wcslen(filename), pUtf8, lLen, false);
		zipFile zf = zipOpen( (char*)pUtf8, APPEND_STATUS_CREATE );
		delete [] pUtf8;
#endif
		// Write CRC and sizes into the seekable output's local headers.
		if (zf)
			mz_zip_set_data_descriptor(zipGetHandle_MZ(zf), 0);
		return zf;
	}
	unzFile unzOpenHelp(const wchar_t* filename)
	{
#ifdef _IOS
		std::string filePath = NSFile::NSIOS::GetFileSystemRepresentation(filename);
		unzFile uf = filePath.empty() ? NULL : unzOpen(filePath.c_str());
#else
		BYTE* pUtf8 = nullptr;
		LONG lLen = 0;
		NSFile::CUtf8Converter::GetUtf8StringFromUnicode(filename, wcslen(filename), pUtf8, lLen, false);
		unzFile uf = unzOpen( (char*)pUtf8 );
		delete [] pUtf8;
#endif
		return uf;
	}

	static std::wstring ascii_to_unicode(const char *src)
	{
		// TODO: check codepage of system (for "bad" archive)
		std::string sAnsi(src);
		return std::wstring(sAnsi.begin(), sAnsi.end());
	}
	static std::string unicode_to_ascii(const wchar_t *src)
	{
		std::wstring sUnicode(src);
		return std::string(sUnicode.begin(), sUnicode.end());
	}
	/*This static functions are copies from ZLib miniunz.c with some changes.*/
	static std::wstring codepage_issue_fixFromOEM( const char* sVal)
	{
#if defined(_WIN32) || defined (_WIN64)
		int nBufferSize = MultiByteToWideChar( CP_OEMCP, 0, sVal, -1, NULL, 0 );
		wchar_t* pBuffer = new wchar_t[nBufferSize];
		MultiByteToWideChar( CP_OEMCP, 0, sVal, -1, pBuffer, nBufferSize );
		//If this parameter is -1, the function processes the entire input string, including the terminating null character.
		//Therefore, the resulting Unicode string has a terminating null character, and the length returned by the function includes this character.
		std::wstring sRes(pBuffer, nBufferSize - 1);
		RELEASEARRAYOBJECTS(pBuffer);
		return sRes;
#else
		return ascii_to_unicode(sVal);
#endif
	}
	static std::string codepage_issue_fixToOEM(const std::wstring& sVal)
	{
#if defined(_WIN32) || defined (_WIN64)
		const wchar_t* buffer = sVal.c_str();
		int nBufferSize = WideCharToMultiByte( CP_OEMCP, 0, buffer, -1, NULL, 0, NULL, NULL );
		char* pBuffer = new char[nBufferSize];
		WideCharToMultiByte( CP_OEMCP, 0, buffer, -1, pBuffer, nBufferSize, NULL, NULL );
		std::string sRes(pBuffer, nBufferSize);
		RELEASEARRAYOBJECTS(pBuffer);
		return sRes;
#else
		return unicode_to_ascii(sVal.c_str());
#endif
	}
	static void change_file_date( const wchar_t *filename, uLong dosdate, tm_unz tmu_date );
	static int do_extract_currentfile( unzFile uf, const wchar_t* unzip_dir, const int* popt_extract_without_path, int* popt_overwrite, const char* password, bool is_replace_slash = false );
	static int do_extract( unzFile uf, const wchar_t* unzip_dir, int opt_extract_without_path, int opt_overwrite, const char* password, const OnProgressCallback* progress );

	static bool is_file_in_archive(unzFile uf, const wchar_t *filename);
	static bool current_file_is_find(unzFile uf, const wchar_t *filename);
	static bool get_file_in_archive(unzFile uf, const wchar_t *filePathInZip, BYTE** fileInBytes, ULONG& nFileSize);

	static unsigned int get_files_count( const WCHAR* dirname );

	/*========================================================================================================*/

	static void replace_all(std::string& subject, const std::string& search, const std::string& replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}

	static void replace_all_w(std::wstring& subject, const std::wstring& search, const std::wstring& replace)
	{
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::wstring::npos)
		{
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}

	// A zip entry name is attacker controlled text, so it may not be allowed to
	// decide where a file is written. Refuse a name that is absolute, carries a
	// drive or UNC prefix, or holds a ".." segment. The test walks the segments
	// instead of looking at the beginning of the string, because a name that
	// starts with a separator ("//.././../etc") reaches the parent directory all
	// the same once it is appended to the extraction directory.
	static bool is_entry_name_within_target(const std::wstring& strName)
	{
		if (strName.empty())
			return false;

		if (L'/' == strName[0] || L'\\' == strName[0])
			return false;

		// A "C:" prefix is refused as a whole, not only "C:\" or "C:/", because on
		// Windows a drive-relative name resolves next to the current directory of
		// that drive, which is outside the extraction directory. The cost is that
		// a name like "x:y" is refused too; no archive in use carries such a name.
		if (strName.length() >= 2 && L':' == strName[1])
		{
			WCHAR cDrive = strName[0];
			if ((cDrive >= L'A' && cDrive <= L'Z') || (cDrive >= L'a' && cDrive <= L'z'))
				return false;
		}

		size_t nStart = 0;
		while (nStart <= strName.length())
		{
			size_t nEnd = strName.find_first_of(L"/\\", nStart);
			if (std::wstring::npos == nEnd)
				nEnd = strName.length();

			if (2 == nEnd - nStart && L'.' == strName[nStart] && L'.' == strName[nStart + 1])
				return false;

			nStart = nEnd + 1;
		}

		return true;
	}

	// A name that starts with a separator is read relative to the archive root,
	// as unzip does: the separators are dropped before the name is checked, so
	// "/word/document.xml" from a lenient producer still lands in the target,
	// while "/../x" is left as "../x" and refused.
	static std::wstring strip_leading_separators(const std::wstring& strName)
	{
		std::wstring::size_type nPos = strName.find_first_not_of(L"/\\");
		return (std::wstring::npos == nPos) ? std::wstring() : strName.substr(nPos);
	}

	static int do_extract_currentfile( unzFile uf, const wchar_t* unzip_dir, const int* popt_extract_without_path, int* /*popt_overwrite*/, const char* password, bool is_replcace_slash )
	{
		char filename_inzipA[4096];
		int err = UNZ_OK;
		unz_file_info file_info;

		err = unzGetCurrentFileInfo(uf,&file_info,filename_inzipA,sizeof(filename_inzipA),nullptr,0,nullptr,0);
		if (err!=UNZ_OK)
			return err;

		std::wstring filenameW;
		if (file_info.flag & 2048 /*11 bit*/)
			filenameW = NSFile::CUtf8Converter::GetUnicodeStringFromUTF8((BYTE*)filename_inzipA, strlen(filename_inzipA));
		else
			filenameW = codepage_issue_fixFromOEM(filename_inzipA);

#ifdef _WIN32
		for(std::wstring::size_type i = 0, len = filenameW.length(); i < len; ++i)
			if(filenameW[i] == L'/')
				filenameW[i] = L'\\';
#else
		if (is_replcace_slash)
			for (std::wstring::size_type i = 0, len = filenameW.length(); i < len; ++i)
				if(filenameW[i] == L'\\')
					filenameW[i] = L'/';

#endif

		// The name is read from the archive, so it must not be able to move the
		// destination outside of the extraction directory. This is tested before
		// any path is built from it, which covers the directory branch below too.
		if (unzip_dir)
		{
			filenameW = strip_leading_separators(filenameW);
			if (!is_entry_name_within_target(filenameW))
				return UNZ_INTERNALERROR;
		}

		std::wstring filenameW_withoutpath = L"";
		std::wstring::size_type posSeparator = filenameW.find_last_of(FILE_SEPARATOR_STR);
		if (posSeparator == std::wstring::npos)
			filenameW_withoutpath = filenameW;
		else if (posSeparator != (filenameW.length() - 1))
			filenameW_withoutpath = filenameW.substr(posSeparator + 1);

		std::wstring output             = std::wstring(unzip_dir) + FILE_SEPARATOR_STR + filenameW;
		std::wstring output_withoutpath = std::wstring(unzip_dir) + FILE_SEPARATOR_STR + filenameW_withoutpath;

		if (filenameW_withoutpath.empty())
		{
			if ((*popt_extract_without_path)==0)
			{
				NSDirectory::CreateDirectory(output);
			}
		}
		else
		{
			const wchar_t* write_filename;
			int skip=0;

			if ((*popt_extract_without_path)==0)
				write_filename = output.c_str();
			else
				write_filename = output_withoutpath.c_str();

			err = unzOpenCurrentFilePassword(uf, password);

			//-------------------------------------------------------------------------------------------------
			NSFile::CFileBinary oFile;
			FILE *fout = nullptr;

			if ((skip==0) && (err==UNZ_OK))
			{
				if(oFile.CreateFileW(write_filename))
					fout = oFile.GetFileNative();

				// some zipfile don't contain directory alone before file
				if ((fout == nullptr) && ((*popt_extract_without_path)==0) &&
						(filenameW_withoutpath!=filenameW))
				{
					std::wstring folder = NSDirectory::GetFolderPath(write_filename);
					NSDirectory::CreateDirectories(folder);

					if(oFile.CreateFileW(write_filename))
					{
						fout = oFile.GetFileNative();
					}
				}
			}

			uInt size_buf = WRITEBUFFERSIZE;
			void* buf = malloc(size_buf);

			if (buf == nullptr)
			{
				return UNZ_INTERNALERROR;
			}

			if (fout != nullptr)
			{
				do
				{
					err = unzReadCurrentFile(uf, buf, size_buf);
					if (err<0)
					{
						break;
					}
					if (err>0)
						if (fwrite(buf, err, 1, fout) != 1)
						{
							err=UNZ_ERRNO;
							break;
						}
				}
				// ?
				while (err>0);
				if (err==0)
				{
					oFile.CloseFile();

					struct tm time;
					memset(&time, 0, sizeof(struct tm));
					time.tm_sec = file_info.tmu_date.tm_sec;
					time.tm_min = file_info.tmu_date.tm_min;
					time.tm_hour = file_info.tmu_date.tm_hour;
					time.tm_mday = file_info.tmu_date.tm_mday;
					time.tm_mon = file_info.tmu_date.tm_mon + 1;
					time.tm_year = file_info.tmu_date.tm_year;

					NSFile::CFileBinary::SetTime(write_filename, &time);
				}
			}

			if (err == UNZ_OK)
			{
				err = unzCloseCurrentFile (uf);
			}
			else
				unzCloseCurrentFile(uf); // don't lose the error

			free(buf);
		}

		return err;
	}

	/*========================================================================================================*/

	static int do_extract( unzFile uf, const wchar_t* unzip_dir, int opt_extract_without_path, int opt_overwrite, const char* password, const OnProgressCallback* progress )
	{
		uLong i, number_extract = 0;
		unz_global_info gi;
		int err;
		FILE* fout=nullptr;

		// [Content-Types.xml] check
		// if found - it is a office file, so we can replace '\' to '/' for non-windows
		// fixes bad zips
		const char* content_types = "[Content_Types].xml";
		bool is_office = UNZ_OK == unzLocateFile(uf, content_types, true);
		unzGoToFirstFile(uf);

		err = unzGetGlobalInfo (uf,&gi);

		for (i = 0; i < gi.number_entry; i++)
		{
			if (do_extract_currentfile(uf, unzip_dir, &opt_extract_without_path,
									   &opt_overwrite,
									   password, is_office) == UNZ_OK)
			{
				number_extract++;
			}

			if ( progress != nullptr )
			{
				short cancel = 0;
				long progressValue = ( 1000000 / gi.number_entry * i );
				if(nullptr != progress)
					(*progress)( UTILS_ONPROGRESSEVENT_ID, progressValue, &cancel );

				if ( cancel != 0 )
				{
					return err;
				}
			}

			if ((i + 1) < gi.number_entry)
			{
				err = unzGoToNextFile(uf);
				if (err!=UNZ_OK)
				{
					break;
				}
			}
		}

		if (number_extract < 1)
		{
			err = -1;
		}

		if ( progress != nullptr )
		{
			short cancel = 0;
			long progressValue = 1000000;
			if(nullptr != progress)
				(*progress)( UTILS_ONPROGRESSEVENT_ID, progressValue, &cancel );
		}

		return err;
	}

	/*========================================================================================================*/

	static bool is_file_in_archive(unzFile uf, const wchar_t *filename)
	{
		uLong i;
		unz_global_info gi;
		int err;

		unzGetGlobalInfo (uf,&gi);

		for (i = 0; i < gi.number_entry; i++)
		{
			if (current_file_is_find(uf, filename))
				return true;

			if ((i + 1) < gi.number_entry)
			{
				err = unzGoToNextFile(uf);
				if (err != UNZ_OK)
					break;
			}
		}
		return false;
	}

	/*========================================================================================================*/

	static bool current_file_is_find(unzFile uf, const wchar_t *filename)
	{
		char filename_inzip[256];

		unz_file_info file_info;

		unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);

		std::wstring filenameW = codepage_issue_fixFromOEM(filename_inzip);

		//TODO is there a need to reduce everything to the bottom???
		return wcscmp(filename, filenameW.c_str()) == 0;
	}

	/*========================================================================================================*/

	static const std::wstring get_filename_from_unzfile(unzFile unzip_file_handle)
	{
		char filename_inzip[256];

		if (UNZ_OK == unzGetCurrentFileInfo(unzip_file_handle, nullptr, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0))
			return codepage_issue_fixFromOEM(filename_inzip);

		return std::wstring(L"");
	}

	/*========================================================================================================*/

	static bool get_file(unzFile unzip_file_handle, BYTE* arr, uInt array_size)
	{
		if(UNZ_OK == unzOpenCurrentFile(unzip_file_handle))
		{
			int data_read_size = unzReadCurrentFile(unzip_file_handle, arr, array_size);

			unzCloseCurrentFile(unzip_file_handle);
			return data_read_size == array_size;
		}
		return false;
	}
	/*========================================================================================================*/

	static bool get_file_in_archive(unzFile unzip_file_handle, const wchar_t *filePathInZip, BYTE** fileInBytes, ULONG& nFileSize)
	{
		if(nullptr == fileInBytes)
			return false;
		(*fileInBytes) = nullptr;
		do
		{
			if (current_file_is_find(unzip_file_handle, filePathInZip))
			{
				unz_file_info file_info;
				unzGetCurrentFileInfo(unzip_file_handle, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);

				if (nFileSize > 0)
					nFileSize = (std::min)(nFileSize, static_cast<ULONG>(file_info.uncompressed_size));
				else
					nFileSize = static_cast<ULONG>(file_info.uncompressed_size);

				(*fileInBytes) = new BYTE[nFileSize];
				get_file(unzip_file_handle, (*fileInBytes), nFileSize);
				return true;
			}
			// else just skip the erroneous file
		} while (UNZ_OK == unzGoToNextFile(unzip_file_handle));

		return false;
	}

	/*========================================================================================================*/

	int oneZipFile(zipFile & zf, std::wstring & file_name, std::wstring & zip_file_name, int method, int compressionLevel, bool bDateTime)
	{
		int err = -1;
		NSFile::CFileBinary oFile;

		zip_fileinfo zinfo;
		zinfo.dosDate = 0;
		zinfo.external_fa = 0;
		zinfo.internal_fa = 0;

		if (bDateTime)
		{
			struct tm edited;
			bool ok = NSFile::CFileBinary::GetTime(file_name, &edited);
			if (ok)
			{
				zinfo.tmz_date.tm_sec = edited.tm_sec;
				zinfo.tmz_date.tm_min = edited.tm_min;
				zinfo.tmz_date.tm_hour = edited.tm_hour;
				zinfo.tmz_date.tm_mday = edited.tm_mday;
				zinfo.tmz_date.tm_mon = edited.tm_mon - 1;
				zinfo.tmz_date.tm_year = edited.tm_year;
			}
		}
		zip_fileinfo* zi_new = bDateTime ? &zinfo : nullptr;

		if (oFile.OpenFile(file_name))
		{
			DWORD dwSizeRead;
			BYTE* pData = new BYTE[oFile.GetFileSize()];
			if (oFile.ReadFile(pData, oFile.GetFileSize(), dwSizeRead))
			{
				std::string zipFileNameA = codepage_issue_fixToOEM(zip_file_name);

				(void)zipOpenNewFileInZip( zf, zipFileNameA.c_str(), zi_new, nullptr, 0, nullptr, 0, nullptr, method, compressionLevel );
				(void)zipWriteInFileInZip( zf, pData, dwSizeRead );
				err = zipCloseFileInZip( zf );
			}
			RELEASEARRAYOBJECTS(pData);
		}
		return err;
	}
	int ZipDir( const WCHAR* dir, const WCHAR* outputFile, const OnProgressCallback* progress, bool sorted, int method, int compressionLevel, bool bDateTime )
	{
		if ( ( dir != nullptr ) && ( outputFile != nullptr ) )
		{
			deque<wstring> StringDeque;
			deque<wstring> zipDeque;
			StringDeque.push_back( wstring( dir ) );

			wstring zipDir;
			wstring file;
			wstring zipFileName;
			wstring szText;

			zipFile zf = zipOpenHelp(outputFile);
			if (!zf) return -1;

			unsigned int filesCount = get_files_count( dir );
			unsigned int currentFileIndex = 0;

			while ( !StringDeque.empty() )
			{
				szText = StringDeque.front();
				StringDeque.pop_front();
				if(zipDeque.size() > 0)
				{
					zipDir = zipDeque.front() + wstring( L"/" );
					zipDeque.pop_front();
				}

				std::vector<std::wstring> aCurFiles			= NSDirectory::GetFiles(szText);
				std::vector<std::wstring> aCurDirectories	= NSDirectory::GetDirectories(szText);

				for(size_t i = 0; i < aCurDirectories.size(); ++i)
				{
					std::wstring sDirName = NSSystemPath::GetFileName(aCurDirectories[i]);

					if (sorted)
					{
						if (sDirName == L"ppt" || sDirName == L"xl" ||
								sDirName == L"word" || sDirName == L"_rels")
						{
							StringDeque.push_front(aCurDirectories[i]);
							zipDeque.push_front(zipDir + sDirName);
						}
						else
						{
							StringDeque.push_back(aCurDirectories[i]);
							zipDeque.push_back(zipDir + sDirName);
						}
					}
					else
					{
						StringDeque.push_back( aCurDirectories[i] );
						zipDeque.push_back( zipDir + sDirName );
					}
				}

				for (size_t i = 0; i < aCurFiles.size(); ++i)
				{
					std::wstring cFileName = NSSystemPath::GetFileName(aCurFiles[i]);

                    if (    std::wstring::npos != cFileName.find(L"mimetype") ||
                            std::wstring::npos != cFileName.find(L"[Content_Types]") ||
                            cFileName == L".rels")
                    {
						file = NSSystemPath::Combine(szText, cFileName);
						zipFileName = zipDir + cFileName;

						oneZipFile(zf, file, zipFileName, 0, compressionLevel, bDateTime);

						aCurFiles.erase(aCurFiles.begin() + i, aCurFiles.begin() + i + 1);
						break;
					}
				}

				for (size_t i = 0; i < aCurFiles.size(); ++i)
				{
					std::wstring cFileName = NSSystemPath::GetFileName(aCurFiles[i]);
					file = NSSystemPath::Combine(szText, cFileName);
					zipFileName = zipDir + cFileName;

					oneZipFile(zf, file, zipFileName, method, compressionLevel, bDateTime);

					if ( progress != nullptr )
					{
						short cancel = 0;
						long progressValue = filesCount ? (1000000L / static_cast<long>(filesCount) * static_cast<long>(currentFileIndex)) : 0;

						if(nullptr != progress)
							(*progress)( UTILS_ONPROGRESSEVENT_ID, progressValue, &cancel );

						if ( cancel != 0 )
						{
							zipClose( zf, nullptr );
						}
					}
					currentFileIndex++;
				}
			}
			zipClose( zf, nullptr );

			if ( progress != nullptr )
			{
				short cancel = 0;
				long progressValue = 1000000;
				if(nullptr != progress)
					(*progress)( UTILS_ONPROGRESSEVENT_ID, progressValue, &cancel );
			}
		}
		return 0;
	}

	/*========================================================================================================*/

	int ZipFile( const WCHAR* inputFile, const WCHAR* outputFile, int method, int compressionLevel, bool bDateTime )
	{
		int err = -1;

		if (( inputFile != nullptr) && (outputFile != nullptr))
		{
			NSFile::CFileBinary oFile;

			zip_fileinfo zinfo;
			zinfo.dosDate = 0;
			zinfo.external_fa = 0;
			zinfo.internal_fa = 0;

			if (bDateTime)
			{
				struct tm edited;
				bool ok = NSFile::CFileBinary::GetTime(inputFile, &edited);
				if (ok)
				{
					zinfo.tmz_date.tm_sec = edited.tm_sec;
					zinfo.tmz_date.tm_min = edited.tm_min;
					zinfo.tmz_date.tm_hour = edited.tm_hour;
					zinfo.tmz_date.tm_mday = edited.tm_mday;
					zinfo.tmz_date.tm_mon = edited.tm_mon - 1;
					zinfo.tmz_date.tm_year = edited.tm_year;
				}
			}
			zip_fileinfo* zi_new = bDateTime ? &zinfo : nullptr;

			if (oFile.OpenFile(inputFile))
			{
				DWORD dwSizeRead;
				BYTE* pData = new BYTE[oFile.GetFileSize()];
				if(oFile.ReadFile(pData, oFile.GetFileSize(), dwSizeRead))
				{
					zipFile zf = zipOpenHelp(outputFile);
					if (zf)
					{
						wstring zipFileName = NSFile::GetFileName(inputFile);
						std::string zipFileNameA = codepage_issue_fixToOEM(zipFileName);

						(void)zipOpenNewFileInZip( zf, zipFileNameA.c_str(), zi_new, nullptr, 0, nullptr, 0, nullptr, method, compressionLevel );
						(void)zipWriteInFileInZip( zf, pData, dwSizeRead );
						(void)zipCloseFileInZip( zf );
						err = zipClose( zf, nullptr );
					}
				}
				RELEASEARRAYOBJECTS(pData);
			}
		}

		return err;
	}

	/*========================================================================================================*/

	bool ClearDirectory( const WCHAR* dir, bool delDir )
	{
		bool result = false;

		if ( dir != nullptr )
		{
			unsigned int size = (unsigned int)wcslen( dir );
			WCHAR* _dir = new WCHAR[size + 2];
			wcsncpy( _dir, dir, size );
			_dir[size++] = L'\0';
			_dir[size] = L'\0';

			NSDirectory::DeleteDirectory(_dir);

			if ( !delDir )
			{
				NSDirectory::GetDirectories(dir);
			}

			if ( _dir != nullptr )
			{
				delete []_dir;
				_dir = nullptr;
			}

			result = true;
		}
		else
		{
			result = false;
		}

		return result;
	}

	/*========================================================================================================*/

	int UnzipToDir( const WCHAR* zipFile, const WCHAR* unzipDir, const OnProgressCallback* progress, const WCHAR* password, bool opt_extract_without_path, bool clearOutputDirectory )
	{
		CunzFileWrapped ufw;
		ufw.Open(zipFile);
		int err = UnzipToDir(ufw.Get(), unzipDir, progress, password, opt_extract_without_path, clearOutputDirectory);

		// call Close() instead ~...() because of error code
		if(err == UNZ_OK)
			err = ufw.Close();
		return err;
	}

	/*========================================================================================================*/

	int UnzipToDir(BYTE* data, size_t len, const WCHAR* unzipDir, const OnProgressCallback* progress, const WCHAR* password, bool opt_extract_without_path, bool clearOutputDirectory )
	{
		CunzFileWrapped ufw;
		ufw.Open(data, len);
		int err = UnzipToDir(ufw.Get(), unzipDir, progress, password, opt_extract_without_path, clearOutputDirectory);

		// call Close() instead ~...() because of error code
		if(err == UNZ_OK)
			err = ufw.Close();
		return err;
	}

	/*========================================================================================================*/

	int UnzipToDir(unzFile uf, const WCHAR* unzipDir, const OnProgressCallback* progress, const WCHAR* password, bool opt_extract_without_path, bool clearOutputDirectory )
	{
		int err = -1;

		if ( uf != nullptr && unzipDir != nullptr )
		{
			if (NSDirectory::Exists(unzipDir))
				err = 0;

			if ( clearOutputDirectory )
			{
				ClearDirectory( unzipDir );
			}

			if ( err == 0 )
			{
				if(nullptr != password)
				{
					std::string passwordA = codepage_issue_fixToOEM(password);
					err = do_extract( uf, unzipDir, opt_extract_without_path, 1, passwordA.c_str(), progress );
				}
				else
					err = do_extract( uf, unzipDir, opt_extract_without_path, 1, nullptr, progress );
			}
		}
		return err;
	}

	/*========================================================================================================*/

	int UncompressBytes( BYTE* destBuf, ULONG* destSize, const BYTE* sourceBuf, ULONG sourceSize )
	{
		int err = -1;

		err = uncompress( destBuf, destSize, sourceBuf, sourceSize );

		return err;
	}

	/*========================================================================================================*/

	int CompressBytes( BYTE* destBuf, ULONG* destSize, const BYTE* sourceBuf, ULONG sourceSize, SHORT level )
	{
		int err = -1;

		err = compress2( destBuf, destSize, sourceBuf, sourceSize, level );

		return err;
	}

	/*========================================================================================================*/

	bool IsArchive(const WCHAR* filename)
	{
		CunzFileWrapped ufw;
		ufw.Open(filename);
		return ufw.Get() != nullptr;
	}

	/*========================================================================================================*/

	bool IsArchive(BYTE* data, size_t len)
	{
		CunzFileWrapped ufw;
		ufw.Open(data, len);
		return ufw.Get() != nullptr;
	}

	/*========================================================================================================*/

	bool IsFileExistInArchive(const WCHAR* zipFile, const WCHAR* filePathInZip)
	{
		CunzFileWrapped ufw;
		ufw.Open(zipFile);
		unzFile uf = ufw.Get();

		return filePathInZip != nullptr && uf != nullptr && is_file_in_archive(uf, filePathInZip);
	}

	/*========================================================================================================*/

	bool IsFileExistInArchive(BYTE* data, size_t len, const WCHAR* filePathInZip)
	{
		CunzFileWrapped ufw;
		ufw.Open(data, len);
		unzFile uf = ufw.Get();

		return filePathInZip != nullptr && uf != nullptr && is_file_in_archive(uf, filePathInZip);
	}

	/*========================================================================================================*/

	bool LoadFileFromArchive(const WCHAR* zipFile, const WCHAR* filePathInZip, BYTE** fileInBytes, ULONG& nFileSize)
	{
		CunzFileWrapped ufw;
		ufw.Open(zipFile);
		unzFile uf = ufw.Get();

		return filePathInZip != nullptr && uf != nullptr && get_file_in_archive( uf, filePathInZip, fileInBytes, nFileSize);
	}

	/*========================================================================================================*/

	bool LoadFileFromArchive(BYTE* data, size_t len, const WCHAR* filePathInZip, BYTE** fileInBytes, ULONG& nFileSize)
	{
		CunzFileWrapped ufw;
		ufw.Open(data, len);
		unzFile uf = ufw.Get();

		return filePathInZip != nullptr && uf != nullptr && get_file_in_archive( uf, filePathInZip, fileInBytes, nFileSize);
	}

	/*========================================================================================================*/

	bool ExtractFiles(const wchar_t* zip_file_path, const ExtractedFileCallback& callback, void* pParam)
	{
		unzFile unzip_file_handle = unzOpenHelp(zip_file_path);
		if ( unzip_file_handle != nullptr )
		{
			do
			{
				unz_file_info file_info;
				unzGetCurrentFileInfo(unzip_file_handle, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);

				BYTE* pData = new BYTE[file_info.uncompressed_size];
				const std::wstring strEntryName = strip_leading_separators(get_filename_from_unzfile(unzip_file_handle));
				// Names that would leave the archive root are not offered to the
				// caller either, for the same reason do_extract_currentfile refuses
				// them.
				if ((file_info.uncompressed_size == 0 || get_file(unzip_file_handle, pData, file_info.uncompressed_size)) &&
					is_entry_name_within_target(strEntryName))
				{
					callback(strEntryName, pData, file_info.uncompressed_size, pParam);
				}
				RELEASEARRAYOBJECTS(pData);
				// else just skip the erroneous file
			} while (UNZ_OK == unzGoToNextFile(unzip_file_handle));

			unzClose( unzip_file_handle );
			return true;
		}
		return false;
	}

	/*========================================================================================================*/

	bool CompressFiles(const wchar_t* zip_file_path, const RequestFileCallback& callback, void* pParam, int compression_level)
	{
		zipFile zip_file_handle = zipOpenHelp(zip_file_path);

		if(nullptr != zip_file_handle)
		{
			BYTE* pData = nullptr;
			long nSize;
			std::wstring in_zip_filename;
			while(callback(in_zip_filename, pData, nSize, pParam))
			{
				std::string in_zip_filenameA = codepage_issue_fixToOEM(in_zip_filename);
				if (ZIP_OK != zipOpenNewFileInZip( zip_file_handle, in_zip_filenameA.c_str(), nullptr, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, compression_level ) ||
						ZIP_OK != zipWriteInFileInZip(zip_file_handle, pData, nSize) ||
						ZIP_OK != zipCloseFileInZip(zip_file_handle))
				{
					zipClose(zip_file_handle, nullptr);
					return false;
				}
			}
			zipClose(zip_file_handle, nullptr);

			return true;
		}
		return false;
	}

	bool GetFilesSize(const WCHAR*  zip_file_path, const std::wstring& searchPattern, ULONG64& nCommpressed, ULONG64& nUncommpressed)
	{
		nCommpressed = 0;
		nUncommpressed = 0;
		unzFile unzip_file_handle = unzOpenHelp(zip_file_path);
		if (unzip_file_handle != nullptr)
		{
			//todo implement true pattern
			bool isAny = false;
			std::wstring searchExt;
			if (0 == searchPattern.length() || searchPattern == L"*")
			{
				isAny = true;
			}
			else if (searchPattern.length() > 1)
			{
				searchExt = searchPattern.substr(2);
			}
			do
			{
				char filename_inzip[256];
				unz_file_info file_info;
				unzGetCurrentFileInfo(unzip_file_handle, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
				std::wstring filenameW = codepage_issue_fixFromOEM(filename_inzip);
				std::transform(filenameW.begin(), filenameW.end(), filenameW.begin(), ::tolower);
				if (isAny || NSFile::GetFileExtention(filenameW) == searchExt)
				{
					nCommpressed += file_info.compressed_size;
					nUncommpressed += file_info.uncompressed_size;
				}
				// else just skip the erroneous file
			} while (UNZ_OK == unzGoToNextFile(unzip_file_handle));

			unzClose (unzip_file_handle);
			return true;
		}
		return false;
	}

	/*========================================================================================================*/

	static unsigned int get_files_count( const WCHAR* dirname )
	{
		unsigned int filescount = 0;

		if ( dirname != nullptr )
		{
			deque<wstring> StringDeque;
			StringDeque.push_back( dirname );

			wstring szText;

			while ( !StringDeque.empty() )
			{
				szText = StringDeque.front();

				StringDeque.pop_front();

				std::vector<std::wstring> aCurFiles = NSDirectory::GetFiles(szText);

				filescount += aCurFiles.size();
				std::vector<std::wstring> aCurDirectories = NSDirectory::GetDirectories(szText);

				for(size_t i = 0; i < aCurDirectories.size(); ++i)
				{
					std::wstring sCurDirectory = aCurDirectories[i];
					StringDeque.push_back( sCurDirectory );
				}
			}
		}

		return filescount;
	}
}
