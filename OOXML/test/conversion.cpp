/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// Converts the fixtures in ExampleFiles/ between XLSB and XLSX with x2t and
// compares the content with the same workbook saved by Microsoft Excel.
//
// The comparison is semantic (see workbook_facts.h): x2t and Excel write the
// same content with different part names, ids, string tables and records, so
// the files themselves never match. XLSB has no XML to compare, so for
// XLSX -> XLSB both XLSB files (x2t's and Excel's) are converted back to XLSX
// first; a difference then comes from x2t's XLSB writer, since both went
// through the same reader.

#include "common.h"
#include "workbook_facts.h"

#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <memory>

namespace
{
	struct Fixture
	{
		const char* direction; // "xlsb2xlsx" or "xlsx2xlsb"
		const char* name;      // file name without extension
	};

	void PrintTo(const Fixture& fixture, std::ostream* os)
	{
		*os << fixture.direction << "/" << fixture.name;
	}

	std::wstring ExampleFile(const Fixture& fixture, const char* extension)
	{
		return L"ExampleFiles" FILE_SEPARATOR_STR + UTF8_TO_U(std::string(fixture.direction)) +
			   FILE_SEPARATOR_STR + UTF8_TO_U(std::string(fixture.name)) + L"." + UTF8_TO_U(std::string(extension));
	}

	struct Conversion
	{
		std::wstring workDir;
		std::string error;
		WorkbookFacts result;    // what x2t wrote
		WorkbookFacts reference; // what Excel wrote
	};

	// Converts `from` to `to` (both by extension) and unpacks the XLSX that
	// comes out, or `from` itself if it is an XLSX already.
	bool ToUnpackedXlsx(const std::wstring& from, const std::wstring& workDir, const std::wstring& name,
						std::wstring& unpacked, std::string& error)
	{
		std::wstring xlsx = from;
		if (from.size() > 5 && from.substr(from.size() - 5) == L".xlsb")
		{
			xlsx = workDir + FILE_SEPARATOR_STR + name + L".xlsx";
			if (!ConvertTo(from, xlsx, workDir, error))
				return false;
		}
		unpacked = workDir + FILE_SEPARATOR_STR + name;
		return Unpack(xlsx, unpacked, error);
	}

	Conversion* Convert(const Fixture& fixture)
	{
		static std::map<std::string, std::unique_ptr<Conversion>> cache;
		std::string key = std::string(fixture.direction) + "/" + fixture.name;
		auto found = cache.find(key);
		if (found != cache.end())
			return found->second.get();

		Conversion* c = new Conversion();
		cache[key].reset(c);
		c->workDir = GetWorkDir();

		bool toXlsb = std::string(fixture.direction) == "xlsx2xlsb";
		std::wstring source = ExampleFile(fixture, toXlsb ? "xlsx" : "xlsb");
		std::wstring reference = ExampleFile(fixture, toXlsb ? "xlsb" : "xlsx");
		std::wstring converted = c->workDir + FILE_SEPARATOR_STR + (toXlsb ? L"result.xlsb" : L"result.xlsx");

		std::wstring resultDir, referenceDir;
		if (!ConvertTo(source, converted, c->workDir, c->error) ||
			!ToUnpackedXlsx(converted, c->workDir, L"result", resultDir, c->error) ||
			!ToUnpackedXlsx(reference, c->workDir, L"reference", referenceDir, c->error))
			return c;

		c->result = LoadWorkbookFacts(resultDir);
		c->reference = LoadWorkbookFacts(referenceDir);
		return c;
	}

	// Lists every fact of the category that differs; skips the test if
	// Excel's file has nothing in that category.
	void ExpectSameContent(const Fixture& fixture, const std::string& category)
	{
		Conversion* c = Convert(fixture);
		ASSERT_TRUE(c->error.empty()) << c->error;

		const std::string prefix = category + "/";
		WorkbookFacts expected, actual;
		for (const auto& fact : c->reference)
			if (fact.first.compare(0, prefix.size(), prefix) == 0)
				expected.insert(fact);
		for (const auto& fact : c->result)
			if (fact.first.compare(0, prefix.size(), prefix) == 0)
				actual.insert(fact);
		if (expected.empty() && actual.empty())
			GTEST_SKIP() << "no " << category << " in this workbook";

		std::string differences;
		size_t count = 0;
		for (const auto& fact : expected)
		{
			auto other = actual.find(fact.first);
			if (other == actual.end())
				differences += "\n  " + fact.first + "\n    missing, Excel: " + fact.second;
			else if (other->second != fact.second)
				differences += "\n  " + fact.first + "\n    x2t:   " + other->second + "\n    Excel: " + fact.second;
			else
				continue;
			if (++count == 20)
				break;
		}
		for (const auto& fact : actual)
			if (count < 20 && expected.find(fact.first) == expected.end())
			{
				differences += "\n  " + fact.first + "\n    extra, x2t: " + fact.second;
				++count;
			}
		EXPECT_TRUE(differences.empty()) << "differences in " << category << " (first 20):" << differences;
	}

	class XlsxConversion : public ::testing::TestWithParam<Fixture>
	{
	};

	std::string FixtureName(const ::testing::TestParamInfo<Fixture>& info)
	{
		return std::string(info.param.direction) + "_" + info.param.name;
	}
}

TEST_P(XlsxConversion, Workbook)    { ExpectSameContent(GetParam(), "workbook"); }
TEST_P(XlsxConversion, DefinedNames){ ExpectSameContent(GetParam(), "names"); }
TEST_P(XlsxConversion, Cells)       { ExpectSameContent(GetParam(), "cells"); }
TEST_P(XlsxConversion, CellStyles)  { ExpectSameContent(GetParam(), "styles"); }
TEST_P(XlsxConversion, MergedCells) { ExpectSameContent(GetParam(), "merges"); }
TEST_P(XlsxConversion, Comments)    { ExpectSameContent(GetParam(), "comments"); }
TEST_P(XlsxConversion, Tables)      { ExpectSameContent(GetParam(), "tables"); }
TEST_P(XlsxConversion, Pivots)      { ExpectSameContent(GetParam(), "pivots"); }
TEST_P(XlsxConversion, Slicers)     { ExpectSameContent(GetParam(), "slicers"); }
TEST_P(XlsxConversion, ChartSheets) { ExpectSameContent(GetParam(), "chartsheets"); }

INSTANTIATE_TEST_SUITE_P(ExcelReference, XlsxConversion,
	::testing::Values(
		Fixture{"xlsb2xlsx", "simple1"},
		Fixture{"xlsb2xlsx", "simple2"},
		Fixture{"xlsb2xlsx", "fmla"},
		Fixture{"xlsx2xlsb", "simple1"},
		Fixture{"xlsx2xlsb", "simple2"},
		Fixture{"xlsx2xlsb", "fmla"}),
	FixtureName);

// The work directories are removed after all tests ran.
class CleanUp : public ::testing::Environment
{
public:
	void TearDown() override
	{
		RemoveWorkDirs();
	}
};
static ::testing::Environment* const g_cleanUp = ::testing::AddGlobalTestEnvironment(new CleanUp);
