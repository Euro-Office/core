/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> xls. Only x2t's command-line mode (two file paths, detect by
// extension) writes XLS; the XML-params mode the server uses returns 88.
//
// The output is checked by reading it back with x2t's own XLS reader
// (xls -> xlsx) and, where that reader hides a defect (it drops a pivot view
// whose cache is missing), by walking the Workbook stream record by record.
// Searching the file for record ids does not work: ids also occur at offsets
// that are not record starts, so a search reports records that were never
// written.

#include "common.h"

#include "../../../OOXML/Base/Base.h"
#include "../../src/dylib/x2t.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"
#include "../../../OfficeUtils/src/OfficeUtils.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
	// Convert in command-line mode: X2T_Convert(3, {"", from, to}) takes the
	// two-file branch in main.cpp, which detects XLS from the .xls extension.
	int ConvertCmdLine(const std::wstring& from, const std::wstring& to)
	{
#if defined(_WIN32) || defined(_WIN64)
		x2tchar* args[3] = {(x2tchar*)L"", (x2tchar*)from.c_str(), (x2tchar*)to.c_str()};
		return X2T_Convert(3, args);
#else
		std::string nfrom = std::filesystem::path(from).string();
		std::string nto = std::filesystem::path(to).string();
		x2tchar* args[3] = {(x2tchar*)"", (x2tchar*)nfrom.c_str(), (x2tchar*)nto.c_str()};
		return X2T_Convert(3, args);
#endif
	}

	std::string ReadAll(const std::wstring& path)
	{
		std::ifstream file(std::filesystem::path(path), std::ios::binary);
		return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	}

	// A compound (OLE2) file starts with this signature.
	bool IsOle2(const std::string& bytes)
	{
		static const char magic[8] = {'\xD0', '\xCF', '\x11', '\xE0', '\xA1', '\xB1', '\x1A', '\xE1'};
		return bytes.size() >= 8 && std::equal(magic, magic + 8, bytes.begin());
	}

	std::wstring ExampleFile(const wchar_t* name)
	{
		return L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR + std::wstring(name);
	}

	const std::wstring kWorkbook = L"xl" FILE_SEPARATOR_STR L"workbook.xml";
	const std::wstring kWorkbookRels = L"xl" FILE_SEPARATOR_STR L"_rels" FILE_SEPARATOR_STR L"workbook.xml.rels";

	const uint16_t kBOF = 0x0809;         // starts a substream
	const uint16_t kEOF = 0x000A;         // ends one
	const uint16_t kBoundSheet8 = 0x0085; // a sheet entry in the globals; lbPlyPos is its substream's BOF offset
	const uint16_t kSXStreamID = 0x00D5;  // a pivot cache in the globals
	const uint16_t kSXView = 0x00B0;      // a pivot view in a worksheet

	// Little-endian unsigned value of `width` bytes at `at`; bytes past the end read as 0.
	uint32_t Le(const std::string& bytes, size_t at, int width)
	{
		uint32_t value = 0;
		for (int i = width - 1; i >= 0; --i)
			value = (value << 8) | (at + i < bytes.size() ? static_cast<unsigned char>(bytes[at + i]) : 0u);
		return value;
	}

	// The "Workbook" stream of a compound file, or "" if it has none. Just enough of [MS-CFB] for the small files
	// these tests write: FAT and mini FAT chains, with every FAT sector listed in the header.
	std::string WorkbookStream(const std::string& file)
	{
		if (!IsOle2(file) || file.size() < 512)
			return std::string();
		const size_t sectorSize = size_t(1) << Le(file, 30, 2);
		const size_t miniSectorSize = size_t(1) << Le(file, 32, 2);
		const uint32_t miniCutoff = Le(file, 56, 4);
		const size_t all = std::string::npos;

		std::function<std::string(uint32_t)> sector = [&](uint32_t s)
		{
			size_t at = 512 + s * sectorSize;
			return at < file.size() ? file.substr(at, sectorSize) : std::string();
		};
		auto table = [](const std::string& bytes)
		{
			std::vector<uint32_t> entries;
			for (size_t at = 0; at + 4 <= bytes.size(); at += 4)
				entries.push_back(Le(bytes, at, 4));
			return entries;
		};
		// a chain of sectors; stops at ENDOFCHAIN (any value past the table) and on a loop
		auto chain = [](uint32_t start, const std::vector<uint32_t>& next,
						const std::function<std::string(uint32_t)>& read, size_t size)
		{
			std::string out;
			for (size_t s = start, steps = 0; s < next.size() && steps <= next.size() && out.size() < size;
				 s = next[s], ++steps)
				out += read(static_cast<uint32_t>(s));
			return out.substr(0, std::min(size, out.size()));
		};

		std::vector<uint32_t> fat;
		for (size_t i = 0; i < 109; ++i)
		{
			uint32_t s = Le(file, 76 + 4 * i, 4);
			if (s >= 0xFFFFFFFA)
				continue;
			std::vector<uint32_t> part = table(sector(s));
			fat.insert(fat.end(), part.begin(), part.end());
		}
		std::string directory = chain(Le(file, 48, 4), fat, sector, all);
		// entry 0 is the root; its stream is the mini stream that holds the streams below the cutoff size
		std::string ministream = chain(Le(directory, 116, 4), fat, sector, Le(directory, 120, 4));
		std::vector<uint32_t> minifat = table(chain(Le(file, 60, 4), fat, sector, all));
		std::function<std::string(uint32_t)> miniSector = [&](uint32_t s)
		{
			size_t at = s * miniSectorSize;
			return at < ministream.size() ? ministream.substr(at, miniSectorSize) : std::string();
		};

		static const std::string name("W\0o\0r\0k\0b\0o\0o\0k\0", 16); // UTF-16LE, without the terminator
		for (size_t at = 0; at + 128 <= directory.size(); at += 128)
		{
			if (directory.compare(at, name.size(), name) != 0 || Le(directory, at + 64, 2) != name.size() + 2
				|| directory[at + 66] != 2 /* stream */)
				continue;
			uint32_t start = Le(directory, at + 116, 4);
			uint32_t size = Le(directory, at + 120, 4);
			return size < miniCutoff ? chain(start, minifat, miniSector, size) : chain(start, fat, sector, size);
		}
		return std::string();
	}

	// Walks a BIFF stream record by record (id, size, body) and calls `visit` with each record's offset, id and body.
	void ForEachRecord(const std::string& stream, const std::function<void(size_t, uint16_t, const std::string&)>& visit)
	{
		for (size_t at = 0; at + 4 <= stream.size(); at += 4 + Le(stream, at + 2, 2))
			visit(at, static_cast<uint16_t>(Le(stream, at, 2)), stream.substr(at + 4, Le(stream, at + 2, 2)));
	}

	int CountRecords(const std::string& stream, uint16_t id)
	{
		int count = 0;
		ForEachRecord(stream, [&](size_t, uint16_t recordId, const std::string&) { count += recordId == id; });
		return count;
	}

	// Every BoundSheet8 must point at the BOF of a sheet substream. The writer pairs the n-th BoundSheet8 with the
	// n-th substream, so a sheet entry without a substream points at offset 0 (the globals) instead.
	void ExpectSheetEntriesPointAtSheets(const std::string& workbook)
	{
		std::set<uint32_t> bofs; // top-level substreams after the globals; an embedded chart nests its own BOF/EOF
		std::vector<uint32_t> entries;
		int depth = 0;
		ForEachRecord(workbook, [&](size_t at, uint16_t id, const std::string& body)
		{
			if (id == kBOF)
			{
				if (depth == 0 && at != 0)
					bofs.insert(static_cast<uint32_t>(at));
				++depth;
			}
			else if (id == kEOF)
				--depth;
			else if (id == kBoundSheet8)
				entries.push_back(Le(body, 0, 4));
		});
		ASSERT_FALSE(entries.empty()) << "no BoundSheet8 in the Workbook stream";
		for (uint32_t lbPlyPos : entries)
			EXPECT_EQ(bofs.count(lbPlyPos), 1u) << "a BoundSheet8 points at offset " << lbPlyPos << ", not at a sheet";
		EXPECT_EQ(bofs.size(), entries.size()) << "sheet substreams without a sheet entry";
	}

	// Converts `xlsx` to `xls`, checks the sheet entries against the substreams, then reads the XLS back to xlsx
	// with x2t and unpacks it into `dir`.
	void ExportAndReadBack(const std::wstring& xlsx, std::wstring& xls, std::wstring& dir)
	{
		std::wstring workDir = GetWorkDir();
		xls = workDir + FILE_SEPARATOR_STR + L"out.xls";
		int code = ConvertCmdLine(xlsx, xls);
		ASSERT_EQ(code, 0) << "xlsx -> xls: x2t returned " << code;
		std::string bytes = ReadAll(xls);
		ASSERT_TRUE(IsOle2(bytes)) << "the XLS output is not a compound file";
		std::string workbook = WorkbookStream(bytes);
		ASSERT_FALSE(workbook.empty()) << "the XLS output has no Workbook stream";
		ASSERT_NO_FATAL_FAILURE(ExpectSheetEntriesPointAtSheets(workbook));

		std::wstring back = workDir + FILE_SEPARATOR_STR + L"back.xlsx";
		code = ConvertCmdLine(xls, back);
		ASSERT_EQ(code, 0) << "reading the XLS back: x2t returned " << code;
		dir = workDir + FILE_SEPARATOR_STR + L"back";
		std::string error;
		ASSERT_TRUE(Unpack(back, dir, error)) << error;
	}

	bool HasPart(const std::wstring& dir, const std::wstring& part)
	{
		return NSFile::CFileBinary::Exists(dir + FILE_SEPARATOR_STR + part);
	}

	// fmla.xlsx has three sheets, "Лист1" to "Лист3" (UTF-8 below), each told apart by its content: Лист1 holds a
	// row of values up to L1, Лист2 has A1 = Лист1!A1 and Лист3 has A1 = SUM(Лист1:Лист2!A1).
	const std::string kSheet = "\xD0\x9B\xD0\xB8\xD1\x81\xD1\x82";

	// The XML of cell A1 up to its </c>, or "" if the sheet has no A1.
	std::string A1Cell(const std::string& sheetXml)
	{
		size_t a1 = sheetXml.find("<c r=\"A1\"");
		size_t end = a1 == std::string::npos ? a1 : sheetXml.find("</c>", a1);
		return a1 == std::string::npos || end == std::string::npos ? std::string() : sheetXml.substr(a1, end - a1);
	}

	int FmlaSheetOf(const std::string& sheetXml)
	{
		if (sheetXml.find("<c r=\"L1\"") != std::string::npos)
			return 1;
		std::string cell = A1Cell(sheetXml);
		if (cell.find("<f>SUM(") != std::string::npos)
			return 3;
		if (cell.find("<f>") != std::string::npos)
			return 2;
		return 0;
	}

	// The tabs of an unpacked read-back, in order: each <sheet> name with its sheet part ("" if it has none).
	std::vector<std::pair<std::string, std::string>> ReadBackTabs(const std::wstring& dir)
	{
		std::string workbook = ReadAll(dir + FILE_SEPARATOR_STR + kWorkbook);
		std::string rels = ReadAll(dir + FILE_SEPARATOR_STR + kWorkbookRels);
		std::vector<std::pair<std::string, std::string>> tabs;
		for (size_t at = 0; (at = workbook.find("<sheet ", at)) != std::string::npos; ++at)
		{
			std::string element = workbook.substr(at, workbook.find('>', at) - at);
			std::string target = RelTarget(rels, XmlAttribute(element, "r:id"));
			std::string sheetXml = target.empty() ? std::string() : ReadAll(dir + FILE_SEPARATOR_STR + L"xl" +
				FILE_SEPARATOR_STR + std::filesystem::path(target).wstring());
			tabs.emplace_back(XmlAttribute(element, "name"), sheetXml);
		}
		return tabs;
	}

	// The tabs of an unpacked fmla.xlsx read-back, in order: each name with the source sheet (1-3) whose content
	// it holds, or 0 if the content is none of them.
	std::vector<std::pair<std::string, int>> FmlaTabs(const std::wstring& dir)
	{
		std::vector<std::pair<std::string, int>> tabs;
		for (const auto& tab : ReadBackTabs(dir))
			tabs.emplace_back(tab.first, FmlaSheetOf(tab.second));
		return tabs;
	}

	// '$' removed: the XLS writer turns relative references into absolute ones (an older, separate defect), and these
	// tests check which sheets a reference points at, not that.
	std::string WithoutDollars(std::string text)
	{
		text.erase(std::remove(text.begin(), text.end(), '$'), text.end());
		return text;
	}

	// The formula in A1 of the read-back tab `name`, without '$', or "" if there is none.
	std::string A1Formula(const std::wstring& dir, const std::string& name)
	{
		for (const auto& tab : ReadBackTabs(dir))
		{
			if (tab.first != name)
				continue;
			std::string cell = A1Cell(tab.second);
			size_t begin = cell.find("<f>");
			size_t end = begin == std::string::npos ? begin : cell.find("</f>", begin);
			return end == std::string::npos ? std::string() : WithoutDollars(cell.substr(begin + 3, end - begin - 3));
		}
		return std::string();
	}

	// The reference of the workbook-wide defined name `name` in the read-back, without '$', or "" if there is none.
	std::string DefinedName(const std::wstring& dir, const std::string& name)
	{
		std::string workbook = ReadAll(dir + FILE_SEPARATOR_STR + kWorkbook);
		std::string start = "<definedName name=\"" + name + "\">";
		size_t begin = workbook.find(start);
		size_t end = begin == std::string::npos ? begin : workbook.find("</definedName>", begin);
		return end == std::string::npos ? std::string()
										: WithoutDollars(workbook.substr(begin + start.size(), end - begin - start.size()));
	}

	std::pair<std::string, int> Tab(int n) { return {kSheet + std::to_string(n), n}; }

	// fmla.xlsx with its second <sheet> made unresolvable by `pattern` -> `replacement` in `part`, plus a
	// workbook-wide name that refers to Лист3. Exported and read back, Лист2 must still be there, as an empty tab in
	// its place: sheet references in formulas and defined names are written as sheet positions, so leaving the tab
	// out would silently move every reference to a later sheet one tab to the left.
	void ExpectUnresolvedSecondSheetKeepsItsPlace(const std::wstring& part, const std::string& pattern,
												 const std::string& replacement)
	{
		std::wstring corrupt, named, xls, dir;
		ASSERT_TRUE(EditXlsxPart(ExampleFile(L"fmla.xlsx"), part, pattern, replacement, corrupt));
		// "$$" is a literal '$' in a regex replacement
		ASSERT_TRUE(EditXlsxPart(corrupt, kWorkbook, "</sheets>",
								 "</sheets><definedNames><definedName name=\"toSheet3\">" + kSheet +
									 "3!$$A$$1</definedName></definedNames>",
								 named));
		ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(named, xls, dir));
		EXPECT_EQ(FmlaTabs(dir), (std::vector<std::pair<std::string, int>>{Tab(1), {kSheet + "2", 0}, Tab(3)}));
		EXPECT_EQ(A1Formula(dir, kSheet + "3"), "SUM(" + kSheet + "1:" + kSheet + "2!A1)")
			<< "the 3-D reference of " << kSheet << "3 points at other sheets";
		EXPECT_EQ(DefinedName(dir, "toSheet3"), kSheet + "3!A1") << "the defined name points at another sheet";
	}
}

// A pivot workbook exports to XLS, and the pivot table, its cache and the cache records come back. This used to
// crash in PIVOTLI::saveContent (a bool function without a return). The record counts also show that the record
// walk finds records that are there, so its zero counts in SkippedCacheLeavesNoPivotView mean something.
TEST(XlsxToXls, PivotWorkbook)
{
	std::wstring xls, dir;
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(ExampleFile(L"simple2.xlsx"), xls, dir));
	std::string workbook = WorkbookStream(ReadAll(xls));
	EXPECT_EQ(CountRecords(workbook, kSXStreamID), 1) << "the pivot cache is not in the globals";
	EXPECT_EQ(CountRecords(workbook, kSXView), 1) << "the pivot view is not in the sheet";
	EXPECT_TRUE(HasPart(dir, L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml"))
		<< "the pivot table was not written";
	EXPECT_TRUE(HasPart(dir, L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheDefinition1.xml"))
		<< "the pivot cache was not written";
	EXPECT_TRUE(HasPart(dir, L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheRecords1.xml"))
		<< "the pivot cache records were not written";
}

// A workbook without pivot tables: every tab keeps its name and its own content.
TEST(XlsxToXls, PlainWorkbook)
{
	std::wstring xls, dir;
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(ExampleFile(L"fmla.xlsx"), xls, dir));
	EXPECT_EQ(FmlaTabs(dir), (std::vector<std::pair<std::string, int>>{Tab(1), Tab(2), Tab(3)}));
}

// The tab order (<sheets>) differs from the order of the worksheet relationships, as after moving a tab in Excel.
// The sheet names and the sheet data must be paired by r:id, not by position.
TEST(XlsxToXls, TabOrderDiffersFromRelationshipOrder)
{
	std::wstring reordered, xls, dir;
	ASSERT_TRUE(EditXlsxPart(ExampleFile(L"fmla.xlsx"), kWorkbook,
							 "(<sheet [^>]*r:id=\"rId1\"/>)(<sheet [^>]*r:id=\"rId2\"/>)(<sheet [^>]*r:id=\"rId3\"/>)",
							 "$3$1$2", reordered));
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(reordered, xls, dir));
	EXPECT_EQ(FmlaTabs(dir), (std::vector<std::pair<std::string, int>>{Tab(3), Tab(1), Tab(2)}));
}

// A <sheet> whose r:id resolves to no sheet part used to dereference a null pointer in CWorkbook::toXLS. Its data is
// not reachable, so it is written as an empty worksheet with its name, in its place (see
// ExpectUnresolvedSecondSheetKeepsItsPlace). Two shapes: the r:id points nowhere while its old relationship is still
// there (an orphan worksheet part), and the relationship is gone.
TEST(XlsxToXls, SheetWithUnknownRelationshipId)
{
	ExpectUnresolvedSecondSheetKeepsItsPlace(kWorkbook, "r:id=\"rId2\"", "r:id=\"rId99\"");
}

TEST(XlsxToXls, SheetWithoutRelationship)
{
	ExpectUnresolvedSecondSheetKeepsItsPlace(kWorkbookRels, "<Relationship Id=\"rId2\"[^>]*/>", "");
}

// With no <sheet> that resolves there is no sheet data at all, so x2t reports an error instead of writing a workbook
// of empty tabs.
TEST(XlsxToXls, NoSheetResolves)
{
	std::wstring corrupt;
	ASSERT_TRUE(EditXlsxPart(ExampleFile(L"fmla.xlsx"), kWorkbook, "r:id=\"rId[123]\"", "r:id=\"rId99\"", corrupt));
	std::wstring out = GetWorkDir() + FILE_SEPARATOR_STR + L"out.xls";
	// x2t reports a convert error (exit 80, AVS_FILEUTILS_ERROR_CONVERT) and writes no file.
	EXPECT_EQ(ConvertCmdLine(corrupt, out), 80);
	EXPECT_FALSE(NSFile::CFileBinary::Exists(out)) << "x2t wrote an XLS with no resolved sheets";
}

// A worksheet cache source without <worksheetSource> used to dereference a null range. Such a cache has no source
// range to write, so it is skipped with its pivot views, like any other cache the writer cannot write.
TEST(XlsxToXls, WorksheetCacheSourceWithoutRange)
{
	std::wstring corrupt, xls, dir;
	ASSERT_TRUE(EditXlsxPart(ExampleFile(L"simple2.xlsx"),
							 L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheDefinition1.xml",
							 "<worksheetSource[^>]*/>", "", corrupt));
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(corrupt, xls, dir));
	std::string workbook = WorkbookStream(ReadAll(xls));
	EXPECT_EQ(CountRecords(workbook, kSXStreamID), 0) << "the cache without a source range was written";
	EXPECT_EQ(CountRecords(workbook, kSXView), 0) << "a pivot view of the skipped cache was written";
}

// While the XLS is written, a pivot view finds its cache through SxView::iCache, a signed 16-bit field that holds
// cacheId + 1, so a cacheId of 32767 or more does not fit. Such a cache is skipped with its views, like any other
// cache the writer cannot write, instead of writing the cache and silently dropping the view. 32766 still fits.
TEST(XlsxToXls, CacheIdTooLargeForXls)
{
	for (const auto& [cacheId, written] : {std::pair<std::string, int>{"32766", 1}, {"32767", 0}})
	{
		SCOPED_TRACE("cacheId " + cacheId);
		std::wstring inWorkbook, inBoth, xls, dir;
		ASSERT_TRUE(EditXlsxPart(ExampleFile(L"simple2.xlsx"), kWorkbook, "cacheId=\"0\"",
								 "cacheId=\"" + cacheId + "\"", inWorkbook));
		ASSERT_TRUE(EditXlsxPart(inWorkbook, L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml",
								 "cacheId=\"0\"", "cacheId=\"" + cacheId + "\"", inBoth));
		ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(inBoth, xls, dir));
		std::string workbook = WorkbookStream(ReadAll(xls));
		EXPECT_EQ(CountRecords(workbook, kSXStreamID), written) << "pivot caches in the globals";
		EXPECT_EQ(CountRecords(workbook, kSXView), written) << "pivot views in the sheet";
	}
}

// A pivot table whose cacheId has no <pivotCache> gets no view. With cacheId 65536 a 16-bit copy of cacheId + 1 is
// 1, the XLS stream id of the workbook's cache 0, so a check on that copy kept the view and bound it to cache 0.
TEST(XlsxToXls, ViewOfMissingCacheIsNotBoundToAnother)
{
	std::wstring corrupt, xls, dir;
	ASSERT_TRUE(EditXlsxPart(ExampleFile(L"simple2.xlsx"),
							 L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml",
							 "cacheId=\"0\"", "cacheId=\"65536\"", corrupt));
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(corrupt, xls, dir));
	std::string workbook = WorkbookStream(ReadAll(xls));
	EXPECT_EQ(CountRecords(workbook, kSXStreamID), 1) << "cache 0 is valid and must still be written";
	EXPECT_EQ(CountRecords(workbook, kSXView), 0) << "the view of the missing cache was bound to another cache";
}

// Two <pivotCache>s with the same cacheId: only the first is written, so each cache stream is written once and the
// cache indexes follow the order of the streams. (Which cache a table meant stays ambiguous; it gets the first.)
TEST(XlsxToXls, RepeatedCacheIdIsWrittenOnce)
{
	std::wstring corrupt, xls, dir;
	ASSERT_TRUE(EditXlsxPart(ExampleFile(L"simple2.xlsx"), kWorkbook, "(<pivotCache [^>]*/>)", "$1$1", corrupt));
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(corrupt, xls, dir));
	std::string workbook = WorkbookStream(ReadAll(xls));
	EXPECT_EQ(CountRecords(workbook, kSXStreamID), 1) << "the repeated cacheId was written twice";
	EXPECT_EQ(CountRecords(workbook, kSXView), 1) << "pivot views in the sheet";
}

// A cache the writer skips (its r:id points at a worksheet, not a cache definition) must not leave a pivot view
// behind that refers to a cache stream which was never written. x2t's XLS reader drops such a view, so reading the
// file back cannot show it: the records are counted instead.
TEST(XlsxToXls, SkippedCacheLeavesNoPivotView)
{
	std::wstring corrupt, xls, dir;
	ASSERT_TRUE(EditXlsxPart(ExampleFile(L"simple2.xlsx"), kWorkbook, "(<pivotCache [^>]*r:id=\")rId4", "$1rId2", corrupt));
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(corrupt, xls, dir));
	std::string workbook = WorkbookStream(ReadAll(xls));
	EXPECT_EQ(CountRecords(workbook, kSXStreamID), 0) << "the skipped cache was written";
	EXPECT_EQ(CountRecords(workbook, kSXView), 0) << "a pivot view of the skipped cache was written";
}

namespace
{
	const std::wstring kChartSheet = L"xl" FILE_SEPARATOR_STR L"chartsheets" FILE_SEPARATOR_STR L"sheet1.xml";
	const std::wstring kChartSheetRels =
		L"xl" FILE_SEPARATOR_STR L"chartsheets" FILE_SEPARATOR_STR L"_rels" FILE_SEPARATOR_STR L"sheet1.xml.rels";
	const std::wstring kWorksheet = L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet1.xml";
	const std::wstring kChartSheetDrawing = L"xl" FILE_SEPARATOR_STR L"drawings" FILE_SEPARATOR_STR L"drawing2.xml";
	const std::wstring kChart = L"xl" FILE_SEPARATOR_STR L"charts" FILE_SEPARATOR_STR L"chart1.xml";

	// simple2.xlsx: Лист1 has a drawing (rId1, a slicer frame; rId2 in its rels is a VML drawing), and the chart sheet
	// "Диаграмма1" has drawing2.xml (rId1) with one anchor, a frame holding chart1.xml (rId1): a clustered bar chart
	// with two series, the first on Лист1!$C$6:$C$8.
	const std::string kChartSheetName = "\xD0\x94\xD0\xB8\xD0\xB0\xD0\xB3\xD1\x80\xD0\xB0\xD0\xBC\xD0\xBC\xD0\xB0" "1";

	// A shape anchor for drawing2.xml.
	const std::string kShapeAnchor =
		"<xdr:absoluteAnchor><xdr:pos x=\"0\" y=\"0\"/><xdr:ext cx=\"100000\" cy=\"100000\"/>"
		"<xdr:sp macro=\"\" textlink=\"\"><xdr:nvSpPr><xdr:cNvPr id=\"3\" name=\"Rect\"/><xdr:cNvSpPr/></xdr:nvSpPr>"
		"<xdr:spPr><a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom></xdr:spPr></xdr:sp><xdr:clientData/>"
		"</xdr:absoluteAnchor>";

	// All chart parts of an unpacked read-back, one after the other.
	std::string ReadBackCharts(const std::wstring& dir)
	{
		std::string charts;
		std::filesystem::path chartDir = std::filesystem::path(dir) / "xl" / "charts";
		if (!std::filesystem::is_directory(chartDir))
			return charts;
		for (const auto& entry : std::filesystem::directory_iterator(chartDir))
		{
			std::string name = entry.path().filename().string();
			if (entry.is_regular_file() && name.rfind("chart", 0) == 0 && entry.path().extension() == ".xml")
				charts += ReadAll(entry.path().wstring());
		}
		return charts;
	}

	// Whether the read-back holds simple2.xlsx's bar chart: x2t's XLS reader writes chart parts for an empty chart
	// sheet too, so the chart is recognised by its series range, not by its part.
	bool HasBarChart(const std::wstring& dir)
	{
		std::string charts = ReadBackCharts(dir);
		return charts.find("<c:barChart>") != std::string::npos && charts.find("$C$6:$C$8") != std::string::npos;
	}

	std::vector<std::string> TabNames(const std::wstring& dir)
	{
		std::vector<std::string> names;
		for (const auto& tab : ReadBackTabs(dir))
			names.push_back(tab.first);
		return names;
	}

	// simple2.xlsx edited by `pattern` -> `replacement` in `part`, exported and read back. All three tabs must come
	// back in order, and the bar chart must come back exactly when `chartKept`.
	void ExpectChartExport(const std::wstring& part, const std::string& pattern, const std::string& replacement,
						   bool chartKept, std::wstring& dir)
	{
		std::wstring corrupt, xls;
		ASSERT_TRUE(EditXlsxPart(ExampleFile(L"simple2.xlsx"), part, pattern, replacement, corrupt));
		ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(corrupt, xls, dir));
		EXPECT_EQ(TabNames(dir), (std::vector<std::string>{kSheet + "1", kSheet + "2", kChartSheetName}));
		EXPECT_EQ(HasBarChart(dir), chartKept) << (chartKept ? "the chart was lost" : "a chart that does not resolve was written");
	}

	void ExpectChartExport(const std::wstring& part, const std::string& pattern, const std::string& replacement,
						   bool chartKept)
	{
		std::wstring dir;
		ExpectChartExport(part, pattern, replacement, chartKept, dir);
	}
}

// The unedited workbook: the chart sheet's chart comes back. The tests below rely on HasBarChart finding it.
TEST(XlsxToXls, ChartSheet)
{
	std::wstring xls, dir;
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(ExampleFile(L"simple2.xlsx"), xls, dir));
	EXPECT_EQ(TabNames(dir), (std::vector<std::string>{kSheet + "1", kSheet + "2", kChartSheetName}));
	EXPECT_TRUE(HasBarChart(dir)) << "the chart sheet's chart was not written";
}

// A chart sheet <drawing> whose r:id resolves to no drawing part used to dereference a null (or wrongly typed)
// pointer in CWorksheet::toXLS. The chart sheet is written without a chart, in its place. Three shapes: the r:id
// points nowhere, the relationship is gone, and the relationship points at a part that is not a drawing.
TEST(XlsxToXls, ChartSheetDrawingWithUnknownRelationshipId)
{
	ExpectChartExport(kChartSheet, "<drawing r:id=\"rId1\"/>", "<drawing r:id=\"rId99\"/>", false);
}

TEST(XlsxToXls, ChartSheetDrawingWithoutRelationship)
{
	ExpectChartExport(kChartSheetRels, "<Relationship Id=\"rId1\"[^>]*/>", "", false);
}

TEST(XlsxToXls, ChartSheetDrawingIsNotADrawing)
{
	ExpectChartExport(kChartSheetRels, "relationships/drawing\" Target=\"../drawings/drawing2.xml\"",
					  "relationships/vmlDrawing\" Target=\"../drawings/vmlDrawing1.vml\"", false);
}

// The same for a worksheet <drawing>: the sheet is written without it, and the chart sheet's chart is not affected.
TEST(XlsxToXls, WorksheetDrawingWithUnknownRelationshipId)
{
	ExpectChartExport(kWorksheet, "<drawing r:id=\"rId1\"/>", "<drawing r:id=\"rId99\"/>", true);
}

TEST(XlsxToXls, WorksheetDrawingIsNotADrawing)
{
	// rId2 is the sheet's VML drawing
	ExpectChartExport(kWorksheet, "<drawing r:id=\"rId1\"/>", "<drawing r:id=\"rId2\"/>", true);
}

// A chart frame whose chart r:id resolves to no chart part used to dereference a null pointer in
// CDrawing::toXLSChart. The chart sheet is written without the chart.
TEST(XlsxToXls, ChartWithUnknownRelationshipId)
{
	ExpectChartExport(kChartSheetDrawing, "r:id=\"rId1\"", "r:id=\"rId99\"", false);
}

// A shape next to the chart on a chart sheet, before or after it, is valid. CDrawing::toXLSChart cast every anchor to
// a graphic frame and crashed on the shape. The shape is not written; the chart is.
TEST(XlsxToXls, ChartSheetWithShapeBeforeChart)
{
	ExpectChartExport(kChartSheetDrawing, "(<xdr:wsDr[^>]*>)", "$1" + kShapeAnchor, true);
}

TEST(XlsxToXls, ChartSheetWithShapeAfterChart)
{
	ExpectChartExport(kChartSheetDrawing, "</xdr:wsDr>", kShapeAnchor + "</xdr:wsDr>", true);
}

// A chart title linked to a cell (<c:strRef>) has no rich text; toXLSChart read the rich text and crashed. The title
// is written with the cell's cached value, and without a cached value the chart is written without title text.
TEST(XlsxToXls, ChartTitleLinkedToCell)
{
	std::wstring dir;
	ASSERT_NO_FATAL_FAILURE(ExpectChartExport(kChart, "<c:title><c:overlay",
											  "<c:title><c:tx><c:strRef><c:f>" + kSheet + "1!$$A$$1</c:f><c:strCache>"
											  "<c:ptCount val=\"1\"/><c:pt idx=\"0\"><c:v>CachedTitle</c:v></c:pt>"
											  "</c:strCache></c:strRef></c:tx><c:overlay",
											  true, dir));
	EXPECT_NE(ReadBackCharts(dir).find("CachedTitle"), std::string::npos) << "the title text was lost";

	ExpectChartExport(kChart, "<c:title><c:overlay",
					  "<c:title><c:tx><c:strRef><c:f>" + kSheet + "1!$$A$$1</c:f></c:strRef></c:tx><c:overlay", true);
}

// A rich text title still comes back with its text.
TEST(XlsxToXls, ChartTitleRichText)
{
	std::wstring dir;
	ASSERT_NO_FATAL_FAILURE(ExpectChartExport(kChart, "<c:title><c:overlay",
											  "<c:title><c:tx><c:rich><a:bodyPr/><a:lstStyle/><a:p><a:r>"
											  "<a:t>RichTitle</a:t></a:r></a:p></c:rich></c:tx><c:overlay",
											  true, dir));
	EXPECT_NE(ReadBackCharts(dir).find("RichTitle"), std::string::npos) << "the title text was lost";
}

// Two more broken chart parts toXLSChart dereferenced without a check: an anchor <xdr:ext> without cy, and a chart
// part without <c:chart>. The first keeps its chart; the second has no chart to write.
TEST(XlsxToXls, ChartAnchorWithoutHeight)
{
	ExpectChartExport(kChartSheetDrawing, "<xdr:ext cx=\"8667479\" cy=\"6290299\"/>", "<xdr:ext cx=\"8667479\"/>", true);
}

TEST(XlsxToXls, ChartWithoutChartElement)
{
	ExpectChartExport(kChart, "<c:chart>.*</c:chart>", "", false);
}

// A line series with a data point that only sets a marker (<c:dPt> without <c:spPr>) is valid. CT_LineSer::GetXLSFormat
// checked the series' shape properties and then read the data point's, and crashed. The chart is written.
TEST(XlsxToXls, LineChartDataPointWithoutShapeProperties)
{
	// simple2.xlsx's clustered bar chart as a line chart, each series with a marker-only data point
	auto toLineChart = [](const std::string& xml)
	{
		std::string out = std::regex_replace(xml, std::regex("<c:barChart><c:barDir val=\"col\"/><c:grouping val=\"clustered\"/>"),
											 "<c:lineChart><c:grouping val=\"standard\"/>");
		out = std::regex_replace(out, std::regex("</c:barChart>"), "</c:lineChart>");
		out = std::regex_replace(out, std::regex("<c:gapWidth [^>]*/>|<c:overlap [^>]*/>"), "");
		out = std::regex_replace(out, std::regex("</c:spPr><c:invertIfNegative val=\"0\"/>"),
								 "</c:spPr><c:dPt><c:idx val=\"1\"/><c:marker><c:symbol val=\"circle\"/></c:marker>"
								 "<c:bubble3D val=\"0\"/></c:dPt>");
		return out;
	};
	std::wstring edited, xls, dir;
	ASSERT_TRUE(TransformXlsxPart(ExampleFile(L"simple2.xlsx"), kChart, toLineChart, edited));
	ASSERT_NO_FATAL_FAILURE(ExportAndReadBack(edited, xls, dir));
	EXPECT_EQ(TabNames(dir), (std::vector<std::string>{kSheet + "1", kSheet + "2", kChartSheetName}));
	std::string charts = ReadBackCharts(dir);
	EXPECT_NE(charts.find("<c:lineChart>"), std::string::npos) << "the line chart was not written";
	EXPECT_NE(charts.find("$C$6:$C$8"), std::string::npos) << "the line chart's series was lost";
}
