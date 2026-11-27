/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

// spreadsheet -> Editor.bin: a workbook converted from an OpenDocument
// spreadsheet carries its source file type (workbook record SourceFileType),
// so the editor can apply the OpenDocument print rules. Other sources do not
// get the record.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace
{
	// c_oSerWorkbookTypes::SourceFileType, item length 4, then the file type as a little-endian LONG
	std::vector<char> SourceFileTypeRecord(int fileType)
	{
		std::vector<char> record = {100, 4, 0, 0, 0};
		for (int i = 0; i < 4; ++i)
			record.push_back(static_cast<char>((fileType >> (8 * i)) & 0xFF));
		return record;
	}

	bool Contains(const std::vector<char>& data, const std::vector<char>& part)
	{
		return std::search(data.begin(), data.end(), part.begin(), part.end()) != data.end();
	}

	// Converts the example file to Editor.bin (no base64) and returns its bytes.
	std::vector<char> ConvertToEditorBin(const std::wstring& name, std::string& error)
	{
		std::vector<char> content;
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR + name;
		if (!NSFile::CFileBinary::Exists(source))
		{
			error = "missing input " + U_TO_UTF8(source);
			return content;
		}
		std::wstring stem = std::filesystem::path(name).stem().wstring();
		std::wstring workDir = GetWorkDir() + FILE_SEPARATOR_STR + L"source_file_type_" + stem;
		std::wstring tempDir = workDir + FILE_SEPARATOR_STR + L"temp";
		std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
		NSDirectory::CreateDirectory(workDir);
		NSDirectory::CreateDirectory(tempDir);

		// The params the document server sends when it opens a file: m_nFormatTo only, no m_nFormatFrom,
		// so x2t detects the source type and stores it back into the params
		int code = ConvertFile(CreateParamsFile(source, bin, tempDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET));
		if (code != 0)
		{
			error = U_TO_UTF8(name) + " -> Editor.bin: x2t returned " + std::to_string(code);
			return content;
		}
		std::ifstream file(std::filesystem::path(bin), std::ios::binary);
		if (!file)
		{
			error = "no Editor.bin for " + U_TO_UTF8(name);
			return content;
		}
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return content;
	}

	// Converts the example file to Editor.bin given the file names only (no m_nFormatFrom, no
	// m_nFormatTo: the direction comes from the extensions) and returns its bytes. m_bIsNoBase64
	// defaults to true, so the file is not base64.
	std::vector<char> ConvertToEditorBinByFileNames(const std::wstring& name, std::string& error)
	{
		std::vector<char> content;
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR + name;
		std::wstring bin = GetWorkDir() + FILE_SEPARATOR_STR L"Editor.bin";
		if (!ConvertTo(source, bin, GetWorkDir(), error))
			return content;
		std::ifstream file(std::filesystem::path(bin), std::ios::binary);
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return content;
	}

	bool ReadBytes(const std::wstring& path, std::vector<char>& content)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		if (!file)
			return false;
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return true;
	}

	// One x2t run with a temp dir of its own: x2t does not clean a temp dir it is given, so a shared
	// one would let a later step zip the parts an earlier step unpacked there.
	bool Convert(const std::wstring& from, const std::wstring& to, int formatFrom, int formatTo, std::string& error)
	{
		int code = ConvertFile(CreateParamsFile(from, to, GetWorkDir(), formatFrom, formatTo));
		if (code != 0 || !NSFile::CFileBinary::Exists(to))
		{
			error = "x2t returned " + std::to_string(code) + " converting " + U_TO_UTF8(from) + " to " + U_TO_UTF8(to);
			return false;
		}
		return true;
	}

	// Saves the workbook of plain.ods to `ext` twice, each time from an Editor.bin as the editor sends
	// it back: once from a binary file with the SourceFileType record (ODS -> Editor.bin), once from
	// one without it (ODS -> XLSX -> Editor.bin). Both results are unpacked into withDir / withoutDir.
	bool SaveWithAndWithoutRecord(int formatTo, const std::wstring& ext, std::wstring& withDir,
								  std::wstring& withoutDir, std::string& error)
	{
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"ods2bin" FILE_SEPARATOR_STR L"plain.ods";
		if (!NSFile::CFileBinary::Exists(source))
		{
			error = "missing input " + U_TO_UTF8(source);
			return false;
		}
		std::wstring workDir = GetWorkDir();
		std::wstring binWith = workDir + FILE_SEPARATOR_STR L"with.bin";
		std::wstring xlsx = workDir + FILE_SEPARATOR_STR L"plain.xlsx";
		std::wstring binWithout = workDir + FILE_SEPARATOR_STR L"without.bin";
		std::wstring savedWith = workDir + FILE_SEPARATOR_STR L"with" + ext;
		std::wstring savedWithout = workDir + FILE_SEPARATOR_STR L"without" + ext;

		if (!Convert(source, binWith, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, error) ||
			!Convert(source, xlsx, 0, AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX, error) ||
			!Convert(xlsx, binWithout, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, error))
			return false;

		std::vector<char> bytesWith, bytesWithout;
		if (!ReadBytes(binWith, bytesWith) || !ReadBytes(binWithout, bytesWithout))
		{
			error = "cannot read the Editor.bin files";
			return false;
		}
		std::vector<char> record = SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS);
		if (!Contains(bytesWith, record) || Contains(bytesWithout, record))
		{
			error = "only the Editor.bin from ODS should carry the record";
			return false;
		}

		if (!Convert(binWith, savedWith, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, formatTo, error) ||
			!Convert(binWithout, savedWithout, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, formatTo, error))
			return false;

		withDir = workDir + FILE_SEPARATOR_STR L"with";
		withoutDir = workDir + FILE_SEPARATOR_STR L"without";
		return Unpack(savedWith, withDir, error) && Unpack(savedWithout, withoutDir, error);
	}

	// x2t counts two kinds of names per process, not per conversion: the numbers in OOXML part names
	// (IFileContainer::m_mapEnumeratedGlobal) and in ODF automatic style names (style_family_counts_ in
	// odf_style_context.cpp). The document server starts one x2t per conversion, but this test converts
	// several times in one process, so the second save writes drawing2.xml and ce4 where the first wrote
	// drawing1.xml and ce1. Number such names in the order they first appear before comparing. Only a
	// whole quoted name ("ce3") or a part path (/drawing1.xml") counts, so cell references (N5, P1:P3,
	// [.T4]) and cell text are left alone; parts with cell references (r="N5") are compared as they are.
	std::string Renumber(const std::vector<char>& bytes)
	{
		static const std::regex name(R"((["/])(ce|co|ro|ta|gr|P|T|N|Mpm|drawing)([0-9]+)(\.xml)?")");
		const std::string xml(bytes.begin(), bytes.end());
		std::map<std::string, std::string> renamed;
		std::map<std::string, int> used;
		std::string result;
		size_t done = 0;
		for (std::sregex_iterator it(xml.begin(), xml.end(), name), end; it != end; ++it)
		{
			const std::smatch& match = *it;
			result.append(xml, done, match.position() - done);
			const std::string key = match.str(2) + match.str(3);
			auto found = renamed.find(key);
			if (found == renamed.end())
				found = renamed.emplace(key, match.str(2) + "#" + std::to_string(++used[match.str(2)])).first;
			result += match.str(1) + found->second + match.str(4) + "\"";
			done = match.position() + match.length();
		}
		result.append(xml, done, std::string::npos);
		return result;
	}

	// [Content_Types].xml lists its Default and Override entries in no fixed order (the macOS build writes
	// them in a different order than the Linux one), so compare the entries as a sorted list.
	std::string SortedContentTypes(const std::string& xml)
	{
		static const std::regex entry(R"(<(Default|Override)\s[^>]*/>)");
		std::vector<std::string> entries;
		for (std::sregex_iterator it(xml.begin(), xml.end(), entry), end; it != end; ++it)
			entries.push_back(it->str());
		std::sort(entries.begin(), entries.end());
		std::string result;
		for (const std::string& item : entries)
			result += item + "\n";
		return result;
	}

	void ExpectSameFile(const std::wstring& withPath, const std::wstring& withoutPath, const std::string& part,
						bool renumber)
	{
		std::vector<char> with, without;
		ASSERT_TRUE(ReadBytes(withPath, with)) << "missing: " << U_TO_UTF8(withPath);
		ASSERT_TRUE(ReadBytes(withoutPath, without)) << "missing: " << U_TO_UTF8(withoutPath);
		EXPECT_FALSE(with.empty()) << part;
		if (part == "[Content_Types].xml")
			EXPECT_EQ(SortedContentTypes(Renumber(with)), SortedContentTypes(Renumber(without)))
				<< part << " differs when the Editor.bin carries the record";
		else if (renumber)
			EXPECT_EQ(Renumber(with), Renumber(without)) << part << " differs when the Editor.bin carries the record";
		else
			EXPECT_EQ(with, without) << part << " differs when the Editor.bin carries the record";
	}

	void ExpectSamePart(const std::wstring& withDir, const std::wstring& withoutDir, const std::wstring& part,
						bool renumber = false)
	{
		ExpectSameFile(withDir + FILE_SEPARATOR_STR + part, withoutDir + FILE_SEPARATOR_STR + part, U_TO_UTF8(part),
					   renumber);
	}

	// The one drawing part of each save, whatever its number
	void ExpectSameDrawing(const std::wstring& withDir, const std::wstring& withoutDir)
	{
		const std::wstring drawings = FILE_SEPARATOR_STR L"xl" FILE_SEPARATOR_STR L"drawings";
		std::vector<std::wstring> with = NSDirectory::GetFiles(withDir + drawings);
		std::vector<std::wstring> without = NSDirectory::GetFiles(withoutDir + drawings);
		ASSERT_EQ(with.size(), 1u);
		ASSERT_EQ(without.size(), 1u);
		ExpectSameFile(with[0], without[0], "the drawing part", false);
	}

	std::string ReadPart(const std::wstring& dir, const std::wstring& part)
	{
		std::vector<char> bytes;
		if (!ReadBytes(dir + FILE_SEPARATOR_STR + part, bytes))
			return "";
		return std::string(bytes.begin(), bytes.end());
	}

	size_t CountOf(const std::string& text, const std::string& what)
	{
		size_t count = 0;
		for (size_t pos = text.find(what); pos != std::string::npos; pos = text.find(what, pos + what.size()))
			++count;
		return count;
	}

	// styles.xml of a workbook saved from an Editor.bin from ODS, with what the record adds taken out again:
	// first-page-number="continue", and the page layout properties written only to hold it
	std::string WithoutContinue(const std::string& xml)
	{
		static const std::regex onlyContinue(
			R"(<style:page-layout ([^>]*)><style:page-layout-properties style:first-page-number="continue"(/>|></style:page-layout-properties>)</style:page-layout>)");
		static const std::regex attribute(R"( style:first-page-number="continue")");
		return std::regex_replace(std::regex_replace(xml, onlyContinue, "<style:page-layout $1/>"), attribute, "");
	}

	// Opens ExampleFiles/ods2bin/<name> as the document server does (-> Editor.bin) and saves that Editor.bin
	// to ODS, unpacked into the returned directory; empty on failure
	std::wstring OpenAndSaveToOds(const std::wstring& name, std::string& error)
	{
		std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"ods2bin" FILE_SEPARATOR_STR + name;
		if (!NSFile::CFileBinary::Exists(source))
		{
			error = "missing input " + U_TO_UTF8(source);
			return L"";
		}
		std::wstring stem = std::filesystem::path(name).stem().wstring();
		std::wstring bin = GetWorkDir() + FILE_SEPARATOR_STR + stem + L".bin";
		std::wstring saved = GetWorkDir() + FILE_SEPARATOR_STR + stem + L"_saved.ods";
		std::wstring dir = GetWorkDir() + FILE_SEPARATOR_STR + stem + L"_saved";
		if (!Convert(source, bin, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, error) ||
			!Convert(bin, saved, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET, AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS, error) ||
			!Unpack(saved, dir, error))
			return L"";
		return dir;
	}
}

TEST(EditorBinSourceFileType, OdsWorkbookRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"ods2bin" FILE_SEPARATOR_STR L"plain.ods", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS)));
}

TEST(EditorBinSourceFileType, FlatOdsWorkbookRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"ods2bin" FILE_SEPARATOR_STR L"plain.fods", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS_FLAT)));
}

TEST(EditorBinSourceFileType, OdsTemplateWorkbookRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"ods2bin" FILE_SEPARATOR_STR L"plain.ots", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_OTS)));
}

TEST(EditorBinSourceFileType, XlsxWorkbookHasNoSourceRecord)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBin(L"xlsx2xlsb" FILE_SEPARATOR_STR L"simple1.xlsx", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_FALSE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX)));
	EXPECT_FALSE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS)));
}

// x2t given only the file names (x2t in.ods out.bin) detects the type in odf2oot_bin
TEST(EditorBinSourceFileType, OdsByFileNamesRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBinByFileNames(L"ods2bin" FILE_SEPARATOR_STR L"plain.ods", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS)));
}

TEST(EditorBinSourceFileType, OdsTemplateByFileNamesRecordsItsSource)
{
	std::string error;
	std::vector<char> bin = ConvertToEditorBinByFileNames(L"ods2bin" FILE_SEPARATOR_STR L"plain.ots", error);
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_TRUE(Contains(bin, SourceFileTypeRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_OTS)));
}

// The save direction: saving a workbook whose Editor.bin carries the record gives the same XLSX as
// saving one without it. The Editor.bin here is x2t's own, where the record is the last workbook item.
TEST(EditorBinSourceFileType, SaveToXlsxSkipsTheRecord)
{
	std::wstring withDir, withoutDir;
	std::string error;
	ASSERT_TRUE(SaveWithAndWithoutRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX, L".xlsx", withDir, withoutDir, error))
		<< error;
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"workbook.xml");
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet1.xml");
	ExpectSamePart(withDir, withoutDir, L"xl" FILE_SEPARATOR_STR L"styles.xml");
	ExpectSamePart(withDir, withoutDir,
				   L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"_rels" FILE_SEPARATOR_STR L"sheet1.xml.rels",
				   true);
	ExpectSamePart(withDir, withoutDir, L"[Content_Types].xml", true);
	ExpectSameDrawing(withDir, withoutDir);
}

// Saving to ODS: the editor numbers the printed pages of a workbook from ODS on across the sheets, so the
// page layout of each sheet gets first-page-number="continue"; without it LibreOffice would start every
// sheet at 1. Apart from that the file is the one saved from an Editor.bin without the record (a workbook
// from XLSX, numbered per sheet), which gets no first page number.
TEST(EditorBinSourceFileType, SaveToOdsContinuesThePageNumbers)
{
	std::wstring withDir, withoutDir;
	std::string error;
	ASSERT_TRUE(SaveWithAndWithoutRecord(AVS_OFFICESTUDIO_FILE_SPREADSHEET_ODS, L".ods", withDir, withoutDir, error))
		<< error;
	ExpectSamePart(withDir, withoutDir, L"content.xml", true);

	const std::string with = ReadPart(withDir, L"styles.xml");
	const std::string without = ReadPart(withoutDir, L"styles.xml");
	ASSERT_FALSE(with.empty());
	ASSERT_FALSE(without.empty());
	EXPECT_EQ(CountOf(with, "style:first-page-number=\"continue\""), 1u) << "the one sheet continues";
	EXPECT_EQ(CountOf(without, "style:first-page-number"), 0u) << "a workbook from XLSX gets no first page number";
	const std::string withAsWithout = WithoutContinue(with);
	EXPECT_EQ(Renumber(std::vector<char>(withAsWithout.begin(), withAsWithout.end())),
			  Renumber(std::vector<char>(without.begin(), without.end())))
		<< "styles.xml differs in more than first-page-number=\"continue\"";
}

// A sheet with a first page number of its own keeps it; the sheets without one continue. Sheet 2 of
// page_numbers.fods starts at 5, sheets 1 and 3 have first-page-number="continue".
TEST(EditorBinSourceFileType, SaveToOdsKeepsAFirstPageNumber)
{
	std::string error;
	std::wstring dir = OpenAndSaveToOds(L"page_numbers.fods", error);
	ASSERT_FALSE(dir.empty()) << error;
	const std::string styles = ReadPart(dir, L"styles.xml");
	ASSERT_FALSE(styles.empty());
	EXPECT_EQ(CountOf(styles, "style:first-page-number=\"5\""), 1u);
	EXPECT_EQ(CountOf(styles, "style:first-page-number=\"continue\""), 2u);
	EXPECT_EQ(CountOf(styles, "style:first-page-number="), 3u);
}
