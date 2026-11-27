/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// xlsx -> Editor.bin -> xlsx. x2t writes vertical runs of "the same formula shifted by one row" as shared formulas
// on the Editor.bin path. The round trip shows what was written: a master carries the text and the block range,
// the cells below it carry the id only. Cells that must not be shared have to come back as plain formulas, and
// every cell has to expand to the formula it had in the source.

#include "common.h"

#include "../../Common/OfficeFileFormats.h"
#include "../../DesktopEditor/common/Directory.h"
#include "../../DesktopEditor/common/File.h"
#include "../XlsxFormat/Worksheets/SharedFormula.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <string>
#include <vector>

namespace
{
	struct CellOut
	{
		bool shared = false;
		bool master = false;
		std::string si;
		std::string ref;
		std::string text;
	};

	std::string Escape(const std::string& s)
	{
		std::string out;
		for (char c : s)
		{
			if (c == '&') out += "&amp;";
			else if (c == '<') out += "&lt;";
			else if (c == '>') out += "&gt;";
			else out += c;
		}
		return out;
	}

	// Moves the relative rows of simple references by delta, the way a shared formula is expanded.
	std::string Shift(const std::string& formula, int delta)
	{
		static const std::regex ref("(\\$?[A-Z]{1,3})(\\$?)(\\d+)");
		std::string out;
		auto begin = std::sregex_iterator(formula.begin(), formula.end(), ref);
		size_t last = 0;
		for (auto it = begin; it != std::sregex_iterator(); ++it)
		{
			const std::smatch& m = *it;
			out += formula.substr(last, m.position() - last);
			out += m[1].str() + m[2].str();
			out += m[2].str().empty() ? std::to_string(std::stoi(m[3].str()) + delta) : m[3].str();
			last = m.position() + m.length();
		}
		return out + formula.substr(last);
	}

	class SharedFormulas : public ::testing::Test
	{
	public:
		// Source formulas: column letter -> row -> <f> attributes and text. Written as one sheet, converted to
		// Editor.bin and back, and read from the result.
		struct Source
		{
			std::string attrs;
			std::string text;
		};
		std::map<std::string, std::map<int, Source>> source;
		std::map<std::string, std::map<int, CellOut>> result;

		void Run()
		{
			std::string sheet = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
								"<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData>";
			std::map<int, std::map<std::string, Source>> byRow;
			for (auto& col : source)
				for (auto& row : col.second)
					byRow[row.first][col.first] = row.second;
			for (auto& row : byRow)
			{
				sheet += "<row r=\"" + std::to_string(row.first) + "\">";
				for (auto& cell : row.second)
					sheet += "<c r=\"" + cell.first + std::to_string(row.first) + "\"><f" + cell.second.attrs + ">" +
							 Escape(cell.second.text) + "</f><v>1</v></c>";
				sheet += "</row>";
			}
			sheet += "</sheetData></worksheet>";

			std::wstring input = L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2xlsb" FILE_SEPARATOR_STR L"simple2.xlsx";
			std::wstring edited;
			ASSERT_TRUE(TransformXlsxPart(input, L"xl/worksheets/sheet1.xml", [&](const std::string&) { return sheet; }, edited));

			std::wstring dir = GetWorkDir();
			std::wstring bin = dir + FILE_SEPARATOR_STR + L"Editor.bin";
			std::wstring out = dir + FILE_SEPARATOR_STR + L"result.xlsx";
			std::wstring t1 = dir + FILE_SEPARATOR_STR + L"open";
			std::wstring t2 = dir + FILE_SEPARATOR_STR + L"save";
			NSDirectory::CreateDirectory(t1);
			NSDirectory::CreateDirectory(t2);
			ASSERT_EQ(0, ConvertFile(CreateParamsFile(edited, bin, t1, 0, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET)));
			ASSERT_EQ(0, ConvertFile(CreateParamsFile(bin, out, t2, AVS_OFFICESTUDIO_FILE_CANVAS_SPREADSHEET,
													  AVS_OFFICESTUDIO_FILE_SPREADSHEET_XLSX)));
			std::wstring unpacked = dir + FILE_SEPARATOR_STR + L"result";
			std::string error;
			ASSERT_TRUE(Unpack(out, unpacked, error)) << error;
			std::ifstream file(std::filesystem::path(unpacked + FILE_SEPARATOR_STR + L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet1.xml"), std::ios::binary);
			std::string xml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			ASSERT_FALSE(xml.empty());

			static const std::regex cell("<c r=\"([A-Z]+)(\\d+)\"[^>]*>\\s*<f([^>]*?)(?:/>|>(.*?)</f>)");
			for (auto it = std::sregex_iterator(xml.begin(), xml.end(), cell); it != std::sregex_iterator(); ++it)
			{
				const std::smatch& m = *it;
				CellOut c;
				std::string attrs = m[3].str();
				c.shared = attrs.find("t=\"shared\"") != std::string::npos;
				c.si = XmlAttribute(attrs, "si");
				c.ref = XmlAttribute(attrs, "ref");
				c.master = c.shared && !c.ref.empty();
				c.text = m[4].str();
				c.text = std::regex_replace(c.text, std::regex("&quot;"), "\"");
				c.text = std::regex_replace(c.text, std::regex("&apos;"), "'");
				c.text = std::regex_replace(c.text, std::regex("&lt;"), "<");
				c.text = std::regex_replace(c.text, std::regex("&gt;"), ">");
				c.text = std::regex_replace(c.text, std::regex("&amp;"), "&");
				result[m[1].str()][std::stoi(m[2].str())] = c;
			}
		}

		// Every cell must expand to its source formula: a follower is its master shifted by the row distance.
		void ExpectExpandsToSource()
		{
			for (auto& col : source)
			{
				std::map<std::string, std::pair<int, std::string>> masters;
				for (auto& row : col.second)
				{
					const CellOut& c = result[col.first][row.first];
					std::string text;
					if (!c.shared)
						text = c.text;
					else if (c.master)
					{
						masters[c.si] = {row.first, c.text};
						text = c.text;
					}
					else
					{
						ASSERT_TRUE(masters.count(c.si)) << col.first << row.first << " has no master";
						text = Shift(masters[c.si].second, row.first - masters[c.si].first);
					}
					EXPECT_EQ(Shift(text, 0), Shift(row.second.text, 0)) << col.first << row.first;
					EXPECT_EQ(text, row.second.text) << col.first << row.first;
				}
			}
		}

		int Masters(const std::string& col)
		{
			int n = 0;
			for (auto& c : result[col])
				n += c.second.master ? 1 : 0;
			return n;
		}
		int Followers(const std::string& col)
		{
			int n = 0;
			for (auto& c : result[col])
				n += (c.second.shared && !c.second.master) ? 1 : 0;
			return n;
		}
	};
}

TEST_F(SharedFormulas, RunsAreSharedInBlocksOf64)
{
	for (int r = 1; r <= 200; ++r)
		source["B"][r] = {"", "A" + std::to_string(r) + "*2+$A$1"};
	Run();
	ASSERT_FALSE(result["B"].empty());
	EXPECT_EQ(4, Masters("B"));
	EXPECT_EQ(196, Followers("B"));
	EXPECT_EQ("B1:B64", result["B"][1].ref);
	EXPECT_EQ("B65:B128", result["B"][65].ref);
	// the last block states the whole block, even though the run ends at row 200
	EXPECT_EQ("B193:B256", result["B"][193].ref);
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, MixedAbsoluteAndRelativeReferences)
{
	for (int r = 1; r <= 10; ++r)
	{
		std::string n = std::to_string(r);
		source["B"][r] = {"", "$A" + n + "+A$1+$A$2+Other!C" + n + ":D" + n};
		source["C"][r] = {"", "SUM(A" + n + ":A" + std::to_string(r + 3) + ")"};
		source["D"][r] = {"", "Rate*2"};
	}
	Run();
	for (const char* col : {"B", "C", "D"})
	{
		EXPECT_EQ(1, Masters(col)) << col;
		EXPECT_EQ(9, Followers(col)) << col;
	}
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, ThreeDimensionalReferencesNamesAndWholeColumns)
{
	for (int r = 1; r <= 8; ++r)
	{
		std::string n = std::to_string(r);
		source["B"][r] = {"", "SUM(Sheet1:Sheet3!A" + n + ")"};            // 3D reference, unquoted
		source["C"][r] = {"", "SUM('Sheet 1:Sheet 3'!A" + n + ":B" + n + ")"}; // 3D reference, quoted
		source["D"][r] = {"", "TaxRate*A" + n + "+Sales2024"};              // defined names stay as they are
		source["E"][r] = {"", "SUM($A:$B)+COUNT(C:C)"};                     // whole columns have no row
		source["F"][r] = {"", "[1]Ext!$A$1+[1]Ext!B" + n};                  // external workbook index
	}
	Run();
	for (const char* col : {"B", "C", "D", "E", "F"})
	{
		EXPECT_EQ(1, Masters(col)) << col;
		EXPECT_EQ(7, Followers(col)) << col;
	}
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, FormulasThatMustStayPlain)
{
	for (int r = 1; r <= 6; ++r)
	{
		std::string n = std::to_string(r);
		source["B"][r] = {"", n + ":" + n};                          // whole rows
		source["C"][r] = {"", "\"A" + n + "\"&A1"};                    // text that looks like a reference
		source["D"][r] = {" t=\"array\" ref=\"D" + n + "\"", "A" + n}; // array formula
		source["E"][r] = {"", "A" + n + "+ZZZ" + n};                   // ZZZ is past the last column
		source["F"][r] = {"", "R" + n + "C1"};                         // starts like a reference
		source["G"][r] = {"", "Table1[[#This Row],[X" + n + "]]+A" + n};   // structured reference text differs per row
		source["H"][r] = {"", "'Sheet " + n + "'!A1"};                 // sheet differs per row
		source["I"][r] = {"", "_xludf.SUM(A" + n + ")"};
	}
	Run();
	for (const char* col : {"B", "C", "D", "E", "F", "G", "H", "I"})
	{
		EXPECT_EQ(0, Masters(col)) << col;
		EXPECT_EQ(0, Followers(col)) << col;
	}
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, NotAdjacentRowsAreNotOneRun)
{
	for (int r : {1, 2, 3, 5, 6, 7})
		source["B"][r] = {"", "A" + std::to_string(r) + "+1"};
	Run();
	EXPECT_EQ(2, Masters("B"));
	EXPECT_EQ(4, Followers("B"));
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, DifferentFormulaStartsANewRun)
{
	for (int r = 1; r <= 6; ++r)
		source["B"][r] = {"", r <= 3 ? "A" + std::to_string(r) + "+1" : "A" + std::to_string(r) + "*2"};
	Run();
	EXPECT_EQ(2, Masters("B"));
	EXPECT_EQ(4, Followers("B"));
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, GoogleSheetsWrapperIsComparedWithoutTheCachedValue)
{
	for (int r = 1; r <= 5; ++r)
	{
		std::string n = std::to_string(r);
		source["B"][r] = {"", "IFERROR(__xludf.DUMMYFUNCTION(\"A" + n + "+1\")," + n + ")"};
	}
	Run();
	EXPECT_EQ(1, Masters("B"));
	EXPECT_EQ(4, Followers("B"));
	// The master keeps the wrapped text. The cached value of the cells below it is not part of the formula
	// (sdkjs drops it when it unwraps), so each cell expands to the master's formula shifted by its distance.
	const CellOut& master = result["B"][1];
	ASSERT_TRUE(master.master);
	for (int r = 2; r <= 5; ++r)
	{
		EXPECT_TRUE(result["B"][r].shared && !result["B"][r].master) << r;
		EXPECT_EQ(master.si, result["B"][r].si) << r;
		EXPECT_NE(Shift(master.text, r - 1).find("A" + std::to_string(r) + "+1"), std::string::npos) << r;
	}
}

// sdkjs cuts the wrapper at the last '")'. With a text as the cached value that is the end of the text, not of
// the formula, so sdkjs cannot parse the result and cannot shift it: such cells must not be shared.
TEST_F(SharedFormulas, WrapperWithTextCachedValueStaysPlain)
{
	for (int r = 1; r <= 5; ++r)
	{
		std::string n = std::to_string(r);
		source["B"][r] = {"", "IFERROR(__xludf.DUMMYFUNCTION(\"A" + n + "+1\"),\"#REF!\")"};
		source["C"][r] = {"", "IFERROR(__xludf.DUMMYFUNCTION(\"IF(A" + n + "<>\"\"\"\",1,2)\"),\"\")"};
	}
	Run();
	for (const char* col : {"B", "C"})
	{
		EXPECT_EQ(0, Masters(col)) << col;
		EXPECT_EQ(0, Followers(col)) << col;
	}
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, BrokenWrapperStaysPlain)
{
	for (int r = 1; r <= 4; ++r)
		source["B"][r] = {"", "IFERROR(__xludf.DUMMYFUNCTION(\"A" + std::to_string(r) + "+1),5)"}; // string never closed
	Run();
	EXPECT_EQ(0, Masters("B"));
	EXPECT_EQ(0, Followers("B"));
	ExpectExpandsToSource();
}

// Running totals: the range starts at an absolute row and ends at the cell's own row. At the row of the anchor it is a
// single cell; above it, it is reversed, and that cell must not start or join a shared run.
TEST_F(SharedFormulas, RunningTotalIsSharedFromTheAnchorRowDown)
{
	for (int r = 1; r <= 8; ++r)
		source["B"][r] = {"", "SUM(A$3:A" + std::to_string(r) + ")"};
	Run();
	EXPECT_FALSE(result["B"][1].shared);
	EXPECT_FALSE(result["B"][2].shared);
	EXPECT_EQ(1, Masters("B"));
	EXPECT_EQ(5, Followers("B"));
	EXPECT_EQ("B3:B66", result["B"][3].ref);
	ExpectExpandsToSource();
}

TEST_F(SharedFormulas, NativeSharedFormulasAreLeftAlone)
{
	source["B"][1] = {" t=\"shared\" ref=\"B1:B3\" si=\"0\"", "A1+1"};
	source["B"][2] = {" t=\"shared\" si=\"0\"", ""};
	source["B"][3] = {" t=\"shared\" si=\"0\"", ""};
	Run();
	EXPECT_EQ(1, Masters("B"));
	EXPECT_EQ(2, Followers("B"));
	EXPECT_EQ("B1:B3", result["B"][1].ref);
	EXPECT_EQ("0", result["B"][1].si);
}

// The key itself: equal for rows that differ by their own row, different for anything else.
TEST(SharedFormulaKey, RowRelativeKey)
{
	using namespace OOX::Spreadsheet::SharedFormula;
	auto key = [](const std::wstring& f, unsigned row, std::basic_string<WCHAR>& out) {
		CKeyBuilder b;
		return b.Build(f.c_str(), (LONG)f.size(), row, out);
	};
	std::basic_string<WCHAR> a, b;
	ASSERT_TRUE(key(L"A5+B$1+$C7", 5, a));
	ASSERT_TRUE(key(L"A6+B$1+$C8", 6, b));
	EXPECT_EQ(a, b);
	ASSERT_TRUE(key(L"A6+B$2+$C8", 6, b));
	EXPECT_NE(a, b);
	ASSERT_TRUE(key(L"IF(A5=\"x\",SUM(Table1[[#This Row],[A5]]),LOG10(3))", 5, a));
	ASSERT_TRUE(key(L"IF(A6=\"x\",SUM(Table1[[#This Row],[A5]]),LOG10(3))", 6, b));
	EXPECT_EQ(a, b);
	for (const wchar_t* refused : {L"1:3", L"$1:$3", L"A1_x", L"A$", L"=A1", L"_xludf.A1", L"XFE1", L"A0", L"\"open", L"'x'", L"1A", L"SUM(B$2:B1)", L"SUM(B3:B1)", L"Sheet1!C5:C4", L"IFERROR(__xludf.DUMMYFUNCTION(\"x\"", L"ab`c"})
		EXPECT_FALSE(key(refused, 5, a)) << refused;
	for (const wchar_t* accepted : {L"A:A", L"$A:$C", L"A1#", L"#REF!+1", L"{1,2;3,\"a\"}", L"1.5E+3*A1", L"Sheet1!A1:B2", L"SUM(B$2:B2)", L"SUM(B$2:B9)", L"[1]Sheet1!$A$1", L"-A1%", L"Sales2024+Rate"})
		EXPECT_TRUE(key(accepted, 5, a)) << accepted;
}
