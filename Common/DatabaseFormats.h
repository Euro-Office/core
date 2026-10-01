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
#pragma once

#include <string>
#include <vector>

// Single authoritative source of supported database file extensions (FR-005).
// COfficeFileFormatChecker::GetFormatByExtension/isOfficeFile/isOnlyOfficeFormatFile
// and OOXML::DatabaseReader::Read all resolve database extensions through this
// list instead of each maintaining their own.
struct DatabaseExtensionEntry
{
	std::wstring extension;    // lower-case, leading '.', e.g. L".mdb"
	int formatConstant;        // one of AVS_OFFICESTUDIO_FILE_SPREADSHEET_*
	bool requiresContentSniff; // true only for ".db" (ambiguous extension)
};

const std::vector<DatabaseExtensionEntry>& GetSupportedDatabaseExtensions();

// Three-way content sniff for an ambiguous ".db" extension (FR-004): returns
// AVS_OFFICESTUDIO_FILE_SPREADSHEET_SQLITE, _MDB, or _BDB based on the file's
// actual header bytes, not its extension.
int DetectAmbiguousDbFormat(const std::wstring& sFilePath);
