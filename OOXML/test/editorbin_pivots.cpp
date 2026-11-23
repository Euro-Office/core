/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> Editor.bin -> xlsx. x2t keeps the pivot parts as raw bytes on this
// path, so they must come back byte for byte; rebuilding them from the object
// model drops content it does not keep.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace
{
	class XlsxEditorBinPivots : public ::testing::Test
	{
	public:
		static void SetUpTestSuite()
		{
			workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"simple2.xlsx";
			std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
			std::wstring result = workDir + FILE_SEPARATOR_STR + L"result.xlsx";

			// One x2t temp dir per step. x2t does not clean a temp dir it is given, and both steps use
			// <temp>/xlsx_unpacked: with a shared dir, step 2 would zip the source parts that step 1
			// unpacked there, so a pivot part dropped by the save step would still be in result.xlsx.
			std::wstring openTempDir = workDir + FILE_SEPARATOR_STR + L"open";
			std::wstring saveTempDir = workDir + FILE_SEPARATOR_STR + L"save";
			NSDirectory::CreateDirectory(openTempDir);
			NSDirectory::CreateDirectory(saveTempDir);

			if (!NSFile::CFileBinary::Exists(source))
			{
				error = "missing input " + U_TO_UTF8(source);
				return;
			}
			int code = ConvertFile(CreateParamsFile(source, bin, openTempDir, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET));
			if (code != 0)
			{
				error = "xlsx -> Editor.bin: x2t returned " + std::to_string(code);
				return;
			}
			code = ConvertFile(CreateParamsFile(bin, result, saveTempDir, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
												AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX));
			if (code != 0)
			{
				error = "Editor.bin -> xlsx: x2t returned " + std::to_string(code);
				return;
			}
			resultDir = workDir + FILE_SEPARATOR_STR + L"result";
			sourceDir = workDir + FILE_SEPARATOR_STR + L"source";
			if (!Unpack(result, resultDir, error))
				return;
			Unpack(source, sourceDir, error);
		}

		void SetUp() override
		{
			ASSERT_TRUE(error.empty()) << error;
		}

		// Compares one part of the round-tripped file with the source.
		static void ExpectSamePart(const std::wstring& part)
		{
			std::vector<char> actual, expected;
			ASSERT_TRUE(ReadPart(resultDir, part, actual)) << "missing in result: " << U_TO_UTF8(part);
			ASSERT_TRUE(ReadPart(sourceDir, part, expected)) << "missing in source: " << U_TO_UTF8(part);
			EXPECT_EQ(actual, expected) << U_TO_UTF8(part) << " changed in the round trip";
		}

		// Names (relative to the unpacked root) of every file under xl/pivotTables and xl/pivotCache,
		// including their _rels. A part the save step drops or invents shows up as a difference.
		static std::set<std::string> PivotPartNames(const std::wstring& dir)
		{
			std::set<std::string> names;
			for (const wchar_t* folder : {L"pivotTables", L"pivotCache"})
			{
				std::filesystem::path root(dir + FILE_SEPARATOR_STR + L"xl" + FILE_SEPARATOR_STR + folder);
				std::error_code ec;
				if (!std::filesystem::exists(root, ec))
					continue;
				for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec))
					if (entry.is_regular_file())
						names.insert(entry.path().lexically_relative(std::filesystem::path(dir)).generic_string());
			}
			return names;
		}

		static std::set<std::string> ResultPartNames() { return PivotPartNames(resultDir); }
		static std::set<std::string> SourcePartNames() { return PivotPartNames(sourceDir); }

		// The round-tripped file. The cacheId link is what this save path recomputes (GetCacheId), so it is
		// checked by value, not by a byte compare of the rewritten .rels.
		static std::string ResultPart(const std::wstring& part)
		{
			std::vector<char> content;
			if (!ReadPart(resultDir, part, content))
				return std::string();
			return std::string(content.begin(), content.end());
		}

	private:
		static bool ReadPart(const std::wstring& dir, const std::wstring& part, std::vector<char>& content)
		{
			std::ifstream file(std::filesystem::path(dir + FILE_SEPARATOR_STR + part), std::ios::binary);
			if (!file)
				return false;
			content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
			return true;
		}

		static std::wstring workDir;
		static std::wstring resultDir;
		static std::wstring sourceDir;
		static std::string error;
	};

	std::wstring XlsxEditorBinPivots::workDir;
	std::wstring XlsxEditorBinPivots::resultDir;
	std::wstring XlsxEditorBinPivots::sourceDir;
	std::string XlsxEditorBinPivots::error;
}

TEST_F(XlsxEditorBinPivots, PivotTable)
{
	ExpectSamePart(L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml");
}

TEST_F(XlsxEditorBinPivots, PivotCacheDefinition)
{
	ExpectSamePart(L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheDefinition1.xml");
}

TEST_F(XlsxEditorBinPivots, PivotCacheRecords)
{
	ExpectSamePart(L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheRecords1.xml");
}

TEST_F(XlsxEditorBinPivots, SamePartsAsSource)
{
	const std::set<std::string> source = SourcePartNames();
	ASSERT_FALSE(source.empty()) << "the fixture has no pivot parts, so this test would check nothing";
	EXPECT_EQ(ResultPartNames(), source) << "the set of pivot parts changed in the round trip";
}

// The pivot table must stay linked to its cache: a wrong cacheId (GetCacheId) detaches the table from the
// cache or drops it on save, and a byte compare of the rewritten .rels would miss it. Checked by value on
// the round-tripped file.
TEST_F(XlsxEditorBinPivots, CacheLinkResolves)
{
	std::string table = ResultPart(L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml");
	std::string workbook = ResultPart(L"xl" FILE_SEPARATOR_STR L"workbook.xml");
	std::string workbookRels =
		ResultPart(L"xl" FILE_SEPARATOR_STR L"_rels" FILE_SEPARATOR_STR L"workbook.xml.rels");
	std::string tableRels = ResultPart(L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"_rels"
										FILE_SEPARATOR_STR L"pivotTable1.xml.rels");
	ASSERT_FALSE(table.empty() || workbook.empty() || workbookRels.empty() || tableRels.empty()) << "pivotTable1.xml, workbook.xml or one of their .rels missing in the result";

	// the table's cacheId
	std::string cacheId = XmlAttribute(table, "cacheId");
	ASSERT_FALSE(cacheId.empty()) << "pivotTable1.xml has no cacheId";

	// a <pivotCache cacheId="..." r:id="..."/> in workbook.xml with that cacheId
	std::string rid;
	size_t at = 0;
	while ((at = workbook.find("<pivotCache", at)) != std::string::npos)
	{
		size_t close = workbook.find('>', at);
		if (close == std::string::npos)
			break;
		std::string element = workbook.substr(at, close - at);
		if (XmlAttribute(element, "cacheId") == cacheId)
		{
			rid = XmlAttribute(element, "r:id");
			break;
		}
		at = close;
	}
	ASSERT_FALSE(rid.empty()) << "workbook.xml has no <pivotCache> with cacheId " << cacheId;

	// that r:id resolves to the cache definition part
	EXPECT_EQ(RelTarget(workbookRels, rid), "pivotCache/pivotCacheDefinition1.xml")
		<< "workbook.xml.rels " << rid << " does not point to the cache definition";

	// and the table's own relationship points to the same part
	std::string tableRid = XmlAttribute(table, "r:id");
	if (tableRid.empty()) // the cache relationship may be the only one
		tableRid = XmlAttribute(tableRels, "Id");
	EXPECT_EQ(RelTarget(tableRels, tableRid), "../pivotCache/pivotCacheDefinition1.xml")
		<< "pivotTable1.xml.rels does not point to the cache definition";
}

namespace
{
	const std::wstring kSimple2 = L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"simple2.xlsx";
	const std::wstring kPivotTable = L"xl" FILE_SEPARATOR_STR L"pivotTables" FILE_SEPARATOR_STR L"pivotTable1.xml";
	const std::wstring kCacheDefinition =
		L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheDefinition1.xml";
	const std::wstring kCacheRecords = L"xl" FILE_SEPARATOR_STR L"pivotCache" FILE_SEPARATOR_STR L"pivotCacheRecords1.xml";

	// The bytes of one part of a package, or false (with a test failure) if the package or the part is missing.
	bool ReadPackagePart(const std::wstring& package, const std::wstring& part, std::string& bytes)
	{
		std::string error;
		std::wstring dir = GetWorkDir();
		if (!Unpack(package, dir, error))
		{
			ADD_FAILURE() << error;
			return false;
		}
		std::ifstream file(std::filesystem::path(dir + FILE_SEPARATOR_STR + part), std::ios::binary);
		if (!file)
		{
			ADD_FAILURE() << U_TO_UTF8(part) << " is not in " << U_TO_UTF8(package);
			return false;
		}
		bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		return true;
	}

	// xlsx -> Editor.bin -> xlsx, one x2t temp dir per step (see XlsxEditorBinPivots). Returns the result package.
	bool RoundTripThroughEditorBin(const std::wstring& source, std::wstring& result)
	{
		std::wstring workDir = GetWorkDir();
		std::wstring bin = workDir + FILE_SEPARATOR_STR + L"Editor.bin";
		std::wstring openTemp = workDir + FILE_SEPARATOR_STR + L"open";
		std::wstring saveTemp = workDir + FILE_SEPARATOR_STR + L"save";
		NSDirectory::CreateDirectory(openTemp);
		NSDirectory::CreateDirectory(saveTemp);
		result = workDir + FILE_SEPARATOR_STR + L"result.xlsx";
		int code = ConvertFile(CreateParamsFile(source, bin, openTemp, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET));
		if (code != 0)
		{
			ADD_FAILURE() << "xlsx -> Editor.bin: x2t returned " << code;
			return false;
		}
		code = ConvertFile(CreateParamsFile(bin, result, saveTemp, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
											AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX));
		if (code != 0)
		{
			ADD_FAILURE() << "Editor.bin -> xlsx: x2t returned " << code;
			return false;
		}
		return true;
	}

	// A UTF-8 part re-encoded as UTF-16LE with a byte order mark, its XML declaration changed to match. OPC allows
	// both encodings for XML parts. `utf8` is a fixture part, so it is valid UTF-8.
	std::string ToUtf16Part(const std::string& utf8)
	{
		std::string xml = std::regex_replace(utf8, std::regex("encoding=\"UTF-8\"", std::regex::icase),
											 "encoding=\"UTF-16\"");
		std::string out("\xFF\xFE", 2);
		auto put = [&](uint32_t unit)
		{
			out += static_cast<char>(unit & 0xFF);
			out += static_cast<char>((unit >> 8) & 0xFF);
		};
		for (size_t i = 0; i < xml.size();)
		{
			unsigned char lead = static_cast<unsigned char>(xml[i]);
			int extra = lead < 0x80 ? 0 : lead < 0xE0 ? 1 : lead < 0xF0 ? 2 : 3;
			uint32_t codePoint = extra == 0 ? lead : (lead & (0x3F >> extra));
			for (int k = 1; k <= extra && i + k < xml.size(); ++k)
				codePoint = (codePoint << 6) | (static_cast<unsigned char>(xml[i + k]) & 0x3F);
			i += extra + 1;
			if (codePoint >= 0x10000)
			{
				codePoint -= 0x10000;
				put(0xD800 + (codePoint >> 10));
				put(0xDC00 + (codePoint & 0x3FF));
			}
			else
				put(codePoint);
		}
		return out;
	}
}

// GetCacheId reads only the root start tag of the pivot table. The root tag may follow a comment that holds '>' and
// '<', and '>' is legal inside an attribute value; a scan that does not skip the comment or stops at a quoted '>'
// hands libxml2 a cut-off tag, GetCacheId fails and the save step drops the table.
TEST(XlsxEditorBinPivotsRootTag, CommentAndGreaterThanInAttribute)
{
	std::wstring withComment, edited, result;
	ASSERT_TRUE(EditXlsxPart(kSimple2, kPivotTable, "(<pivotTableDefinition)", "<!-- a > <b -->$1", withComment));
	ASSERT_TRUE(EditXlsxPart(withComment, kPivotTable, "(<pivotTableDefinition [^>]*name=\")[^\"]*\"", "$1a>b\"", edited));
	ASSERT_TRUE(RoundTripThroughEditorBin(edited, result));

	std::string table;
	ASSERT_TRUE(ReadPackagePart(result, kPivotTable, table)) << "the pivot table was dropped on save";
	EXPECT_NE(table.find("cacheId=\""), std::string::npos);
}

// A processing instruction before the root tag ends at "?>", and may hold '>' and '<' before that. A scan that ends
// it at the first '>' takes the '<' after it for the root tag: GetCacheId fails and the save step drops the pivot
// table, or the cache's root check fails and the cache records are dropped. Both parts are kept as raw bytes, so they
// come back unchanged.
TEST(XlsxEditorBinPivotsRootTag, ProcessingInstructionBeforePivotTable)
{
	std::wstring edited, result;
	ASSERT_TRUE(EditXlsxPart(kSimple2, kPivotTable, "(<pivotTableDefinition)", "<?eo-test a>b<c?>$1", edited));
	ASSERT_TRUE(RoundTripThroughEditorBin(edited, result));

	std::string expected, actual;
	ASSERT_TRUE(ReadPackagePart(edited, kPivotTable, expected));
	ASSERT_TRUE(ReadPackagePart(result, kPivotTable, actual)) << "the pivot table was dropped on save";
	EXPECT_EQ(actual, expected) << "the pivot table changed in the round trip";
}

TEST(XlsxEditorBinPivotsRootTag, ProcessingInstructionBeforeCacheDefinition)
{
	std::wstring edited, result;
	ASSERT_TRUE(EditXlsxPart(kSimple2, kCacheDefinition, "(<pivotCacheDefinition)", "<?eo-test a>b<c?>$1", edited));
	ASSERT_TRUE(RoundTripThroughEditorBin(edited, result));

	std::string expected, actual;
	ASSERT_TRUE(ReadPackagePart(kSimple2, kCacheRecords, expected));
	ASSERT_TRUE(ReadPackagePart(result, kCacheRecords, actual)) << "the cache records were dropped on save";
	EXPECT_EQ(actual, expected) << "the cache records changed in the round trip";
}

// A UTF-16 pivot table part. The root-tag scan reads bytes as ASCII, which a UTF-16 part is not; GetCacheId must still
// get the cacheId, or the save step drops the table. The part is kept as raw bytes, so it comes back unchanged.
TEST(XlsxEditorBinPivotsRootTag, Utf16PivotTable)
{
	std::wstring edited, result;
	ASSERT_TRUE(TransformXlsxPart(kSimple2, kPivotTable, ToUtf16Part, edited));
	ASSERT_TRUE(RoundTripThroughEditorBin(edited, result));

	std::string expected, actual;
	ASSERT_TRUE(ReadPackagePart(edited, kPivotTable, expected));
	ASSERT_TRUE(ReadPackagePart(result, kPivotTable, actual)) << "the UTF-16 pivot table was dropped on save";
	EXPECT_EQ(actual, expected) << "the UTF-16 pivot table changed in the round trip";
}

// A UTF-16 pivot cache definition. The cache's read() checks the root element before it loads the cache records; if
// that check fails on UTF-16, the records part is dropped on save.
TEST(XlsxEditorBinPivotsRootTag, Utf16PivotCacheDefinition)
{
	std::wstring edited, result;
	ASSERT_TRUE(TransformXlsxPart(kSimple2, kCacheDefinition, ToUtf16Part, edited));
	ASSERT_TRUE(RoundTripThroughEditorBin(edited, result));

	std::string expected, actual;
	ASSERT_TRUE(ReadPackagePart(edited, kCacheDefinition, expected));
	ASSERT_TRUE(ReadPackagePart(result, kCacheDefinition, actual));
	EXPECT_EQ(actual, expected) << "the UTF-16 cache definition changed in the round trip";
	ASSERT_TRUE(ReadPackagePart(kSimple2, kCacheRecords, expected));
	ASSERT_TRUE(ReadPackagePart(result, kCacheRecords, actual)) << "the cache records were dropped on save";
	EXPECT_EQ(actual, expected) << "the cache records changed in the round trip";
}
