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
#include "../../../OOXML/Base/Base.h"
#include <functional>
#include <string>
#include <vector>

int ConvertFile(const std::wstring &fileName);
// formatFrom/formatTo are AVS_OFFICESTUDIO_FILE_* values; 0 lets x2t detect the format
// csvEncoding is the code page index CSVReader expects (46 is UTF-8, 48 UTF-16LE)
std::wstring CreateParamsFile(const std::wstring &pathFrom, const std::wstring &pathTo, const std::wstring &TempDir, int formatFrom = 0, int formatTo = 0, int csvEncoding = 46);

// Converts `from` into `to` with x2t (formats by extension). On failure,
// returns false and describes it in `error`.
bool ConvertTo(const std::wstring &from, const std::wstring &to, const std::wstring &tempDir, std::string &error, int csvEncoding = 46);

// Extracts a package into `dir`.
bool Unpack(const std::wstring &file, const std::wstring &dir, std::string &error);

// A new, empty directory next to the test binary; RemoveWorkDirs() deletes
// all of them.
std::wstring GetWorkDir();
void RemoveWorkDirs();


// Unpacks `source`, replaces the part `part` (relative to the package root) with edit(<its bytes>) and zips the
// result to a new .xlsx in `result`. Adds a test failure and returns false if the part is missing or the edit leaves
// it unchanged, so a fixture edit cannot silently become the unmodified file.
bool TransformXlsxPart(const std::wstring &source, const std::wstring &part,
                       const std::function<std::string(const std::string &)> &edit, std::wstring &result);

// The value of the first attribute named exactly `attr` (e.g. "r:id") in `xml` at or after `from`, or "" if there
// is none. A string scan for small fixture parts, not an XML parser.
std::string XmlAttribute(const std::string &xml, const std::string &attr, size_t from = 0);

// The Target of the relationship with Id `id` in a .rels part, or "" if there is none.
std::string RelTarget(const std::string &rels, const std::string &id);

// TransformXlsxPart with std::regex_replace(pattern -> replacement) as the edit; fails if the pattern matches nothing.
bool EditXlsxPart(const std::wstring &source, const std::wstring &part, const std::string &pattern,
                  const std::string &replacement, std::wstring &result);
