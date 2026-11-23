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

#include "common.h"
#include <gtest/gtest.h>
#include <cctype>
#include <iterator>
#include <fstream>
#include <regex>

#include "../../../DesktopEditor/common/Directory.h"
#include "../../../DesktopEditor/common/Path.h"
#include "../../../OfficeUtils/src/OfficeUtils.h"
#include "../../../DesktopEditor/fontengine/ApplicationFontsWorker.h"
#include "../../../Common/OfficeFileFormatChecker.h"
#include "../../../DesktopEditor/common/StringBuilder.h"
#include "../../src/dylib/x2t.h"
#if defined(_WIN32) || defined(_WIN64)
#include "tchar.h"
#endif
#include <cstdlib>
#include <locale>
#include <codecvt>
#include <filesystem>


static std::vector<std::wstring> g_workDirs;

std::wstring GetWorkDir()
{
    std::wstring dir = NSDirectory::CreateDirectoryWithUniqueName(NSFile::GetProcessDirectory());
    g_workDirs.push_back(dir);
    return dir;
}

void RemoveWorkDirs()
{
    for (const std::wstring &dir : g_workDirs)
        NSDirectory::DeleteDirectory(dir);
    g_workDirs.clear();
}

int ConvertFile(const std::wstring &fileName)
{
    x2tchar* args[2];
    args[0] = (x2tchar*)"";
#if defined(_WIN32) || defined(_WIN64)
    args[1] = (x2tchar*)fileName.c_str();
#else
    // X2T_Convert takes narrow UTF-8 char* on non-Windows (see src/dylib/x2t.h).
    // Casting wchar_t* directly truncates the path to its first character.
    std::string narrowPath = std::filesystem::path(fileName).string();
    args[1] = (x2tchar*)narrowPath.c_str();
#endif
    int nResultCode = X2T_Convert(2, args);
    return nResultCode;
}

bool ConvertTo(const std::wstring &from, const std::wstring &to, const std::wstring &tempDir, std::string &error, int csvEncoding)
{
    if (!NSFile::CFileBinary::Exists(from))
    {
        error = "missing input " + U_TO_UTF8(from);
        return false;
    }
    int code = ConvertFile(CreateParamsFile(from, to, tempDir, 0, 0, csvEncoding));
    if (code != 0 || !NSFile::CFileBinary::Exists(to))
    {
        error = "x2t returned " + std::to_string(code) + " converting " + U_TO_UTF8(from) + " to " + U_TO_UTF8(to);
        return false;
    }
    return true;
}

bool Unpack(const std::wstring &file, const std::wstring &dir, std::string &error)
{
    NSDirectory::CreateDirectory(dir);
    COfficeUtils oCOfficeUtils(NULL);
    if (S_OK != oCOfficeUtils.ExtractToDirectory(file, dir, NULL, 0))
    {
        error = "cannot unpack " + U_TO_UTF8(file);
        return false;
    }
    return true;
}

std::wstring CreateParamsFile(const std::wstring &pathFrom, const std::wstring &pathTo, const std::wstring &TempDir, int formatFrom, int formatTo, int csvEncoding)
{
    NSStringUtils::CStringBuilder oBuilder;

	oBuilder.WriteString(L"<?xml version=\"1.0\" encoding=\"utf-8\"?>");
	oBuilder.WriteString(L"<TaskQueueDataConvert>");

	// main
	oBuilder.WriteString(L"<m_sFileFrom>");
	oBuilder.WriteEncodeXmlString(pathFrom);
	oBuilder.WriteString(L"</m_sFileFrom>");

	oBuilder.WriteString(L"<m_sFileTo>");
	oBuilder.WriteEncodeXmlString(pathTo);
	oBuilder.WriteString(L"</m_sFileTo>");

	if (formatFrom)
	{
		oBuilder.WriteString(L"<m_nFormatFrom>");
		oBuilder.WriteString(std::to_wstring(formatFrom));
		oBuilder.WriteString(L"</m_nFormatFrom>");
	}
	if (formatTo)
	{
		oBuilder.WriteString(L"<m_nFormatTo>");
		oBuilder.WriteString(std::to_wstring(formatTo));
		oBuilder.WriteString(L"</m_nFormatTo>");
	}
	// changes
	oBuilder.WriteString(L"<m_bFromChanges>false</m_bFromChanges>");

	oBuilder.WriteString(L"<m_bDontSaveAdditional>true</m_bDontSaveAdditional>");

	// temp directory
	oBuilder.WriteString(L"<m_sTempDir>");
	oBuilder.WriteEncodeXmlString(TempDir);
	oBuilder.WriteString(L"</m_sTempDir>");

	// txt/csv
	oBuilder.WriteString(L"<m_nCsvTxtEncoding>");
	oBuilder.WriteEncodeXmlString(std::to_wstring(csvEncoding));
	oBuilder.WriteString(L"</m_nCsvTxtEncoding>");

	oBuilder.WriteString(L"<m_nCsvDelimiter>");
	oBuilder.WriteEncodeXmlString(L"4");
	oBuilder.WriteString(L"</m_nCsvDelimiter>");

	oBuilder.WriteString(L"</TaskQueueDataConvert>");

    auto xml = TempDir + FILE_SEPARATOR_STR + L"params.xml";
    // writing xml data into file
	if(NSFile::CFileBinary::Exists(xml))
		NSFile::CFileBinary::Remove(xml);

	NSFile::CFileBinary xml_file;
	xml_file.CreateFile(xml);
	xml_file.WriteStringUTF8(oBuilder.GetData());
	xml_file.CloseFile();

    return xml;
}


bool TransformXlsxPart(const std::wstring& source, const std::wstring& part,
					   const std::function<std::string(const std::string&)>& edit, std::wstring& result)
{
	std::wstring dir = GetWorkDir();
	std::string error;
	if (!Unpack(source, dir + FILE_SEPARATOR_STR + L"pkg", error))
	{
		ADD_FAILURE() << error;
		return false;
	}
	std::wstring partPath = dir + FILE_SEPARATOR_STR + L"pkg" + FILE_SEPARATOR_STR + part;
	std::string xml;
	{
		std::ifstream in(std::filesystem::path(partPath), std::ios::binary);
		if (!in)
		{
			ADD_FAILURE() << U_TO_UTF8(part) << " is not in " << U_TO_UTF8(source);
			return false;
		}
		xml.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	}
	std::string edited = edit(xml);
	if (edited == xml)
	{
		ADD_FAILURE() << "the edit left " << U_TO_UTF8(part) << " unchanged";
		return false;
	}
	{
		std::ofstream out(std::filesystem::path(partPath), std::ios::binary | std::ios::trunc);
		out << edited;
	}
	result = dir + FILE_SEPARATOR_STR + L"edited.xlsx";
	COfficeUtils zip(NULL);
	if (S_OK != zip.CompressFileOrDirectory(dir + FILE_SEPARATOR_STR + L"pkg", result))
	{
		ADD_FAILURE() << "cannot zip the edited copy";
		return false;
	}
	return true;
}

std::string XmlAttribute(const std::string& xml, const std::string& attr, size_t from)
{
	std::string needle = attr + "=\"";
	for (size_t at = xml.find(needle, from); at != std::string::npos; at = xml.find(needle, at + 1))
	{
		// a whole attribute name: "Id" must not match the end of "sheetId"
		if (at == 0 || !std::isspace(static_cast<unsigned char>(xml[at - 1])))
			continue;
		size_t begin = at + needle.size();
		size_t end = xml.find('"', begin);
		if (end == std::string::npos)
			return std::string();
		return xml.substr(begin, end - begin);
	}
	return std::string();
}

std::string RelTarget(const std::string& rels, const std::string& id)
{
	size_t at = 0;
	while ((at = rels.find("<Relationship", at)) != std::string::npos)
	{
		size_t close = rels.find('>', at);
		if (close == std::string::npos)
			break;
		std::string element = rels.substr(at, close - at);
		if (XmlAttribute(element, "Id") == id)
			return XmlAttribute(element, "Target");
		at = close;
	}
	return std::string();
}

bool EditXlsxPart(const std::wstring& source, const std::wstring& part, const std::string& pattern,
				  const std::string& replacement, std::wstring& result)
{
	return TransformXlsxPart(source, part, [&](const std::string& xml)
	{
		return std::regex_replace(xml, std::regex(pattern), replacement);
	}, result);
}

