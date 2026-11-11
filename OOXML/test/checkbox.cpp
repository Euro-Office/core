/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */
// Round-trips an XLSX with Excel cell checkboxes through Editor.bin
// (XLSX -> bin -> XLSX), as the editor does on open and save.
//
// A checkbox is an <xf> whose extLst references, through
// xl/featurePropertyBag/featurePropertyBag.xml, a bag of type "Checkbox".
// The bin carries only a boolean per xf (c_oSerXfsTypes::CellControl), not the
// xfComplement index or the part, so the checkbox can only reach the saved
// XLSX through that flag.

#include "common.h"

#include "../../DesktopEditor/common/File.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{
	const std::string kXfComplement = "<xfpb:xfComplement i=\"0\"/>";

	std::string ReadText(const std::wstring& path)
	{
		std::string text;
		NSFile::CFileBinary::ReadAllTextUtf8A(path, text);
		return text;
	}

	// The element starting at the first `open` at or after `from`, up to `close`.
	std::string Element(const std::string& xml, const std::string& open, const std::string& close, size_t from = 0)
	{
		size_t begin = xml.find(open, from);
		if (begin == std::string::npos)
			return "";
		size_t end = xml.find(close, begin);
		return end == std::string::npos ? "" : xml.substr(begin, end + close.size() - begin);
	}

	std::string Attribute(const std::string& tag, const std::string& name)
	{
		std::string open = " " + name + "=\"";
		size_t begin = tag.find(open);
		if (begin == std::string::npos)
			return "";
		begin += open.size();
		return tag.substr(begin, tag.find('"', begin) - begin);
	}

	std::vector<std::string> CellXfs(const std::string& styles)
	{
		std::string cellXfs = Element(styles, "<cellXfs", "</cellXfs>");
		// "<xf" followed by " ", "/" or ">", not "<xfpb:..."
		auto nextXf = [&cellXfs](size_t from)
		{
			for (size_t at = cellXfs.find("<xf", from); at != std::string::npos; at = cellXfs.find("<xf", at + 1))
				if (at + 3 < cellXfs.size() && std::string(" />").find(cellXfs[at + 3]) != std::string::npos)
					return at;
			return std::string::npos;
		};
		std::vector<std::string> xfs;
		for (size_t begin = nextXf(0); begin != std::string::npos;)
		{
			size_t next = nextXf(begin + 1);
			xfs.push_back(cellXfs.substr(begin, next == std::string::npos ? std::string::npos : next - begin));
			begin = next;
		}
		return xfs;
	}

	// The xf index of a cell, 0 if it has no s attribute.
	size_t CellStyle(const std::string& sheet, const std::string& ref)
	{
		std::string c = Element(sheet, "<c r=\"" + ref + "\"", ">");
		std::string s = Attribute(c, "s");
		return s.empty() ? 0 : std::stoul(s);
	}

	class CheckboxRoundTrip : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			std::wstring workDir = GetWorkDir();
			std::wstring source = L"ExampleFiles" FILE_SEPARATOR_STR L"xlsx2bin" FILE_SEPARATOR_STR L"checkbox.xlsx";
			bin = workDir + FILE_SEPARATOR_STR L"Editor.bin";
			std::wstring xlsx = workDir + FILE_SEPARATOR_STR L"result.xlsx";
			unpacked = workDir + FILE_SEPARATOR_STR L"result";

			// Each step gets its own temp dir: x2t unpacks into <temp>/xlsx_unpacked
			// in both directions, so a shared one would leak the source parts into the result.
			error.clear();
			if (ConvertTo(source, bin, GetWorkDir(), error) &&
				ConvertTo(bin, xlsx, GetWorkDir(), error))
				Unpack(xlsx, unpacked, error);
		}

		std::string Part(const wchar_t* path)
		{
			return ReadText(unpacked + FILE_SEPARATOR_STR + path);
		}

		static std::wstring bin;
		static std::wstring unpacked;
		static std::string error;
	};

	std::wstring CheckboxRoundTrip::bin;
	std::wstring CheckboxRoundTrip::unpacked;
	std::string CheckboxRoundTrip::error;
}

TEST_F(CheckboxRoundTrip, BinIsSpreadsheet)
{
	ASSERT_TRUE(error.empty()) << error;
	EXPECT_EQ(ReadText(bin).compare(0, 4, "XLSY"), 0);
}

TEST_F(CheckboxRoundTrip, FeaturePropertyBagPart)
{
	ASSERT_TRUE(error.empty()) << error;
	std::string bag = Part(L"xl" FILE_SEPARATOR_STR L"featurePropertyBag" FILE_SEPARATOR_STR L"featurePropertyBag.xml");
	ASSERT_FALSE(bag.empty()) << "xl/featurePropertyBag/featurePropertyBag.xml is missing";
	EXPECT_NE(bag.find("<bag type=\"Checkbox\"/>"), std::string::npos) << bag;
	EXPECT_NE(bag.find("<bag type=\"XFComplements\""), std::string::npos) << bag;
}

TEST_F(CheckboxRoundTrip, FeaturePropertyBagRelationship)
{
	ASSERT_TRUE(error.empty()) << error;
	std::string rels = Part(L"xl" FILE_SEPARATOR_STR L"_rels" FILE_SEPARATOR_STR L"workbook.xml.rels");
	size_t type = rels.find("Type=\"http://schemas.microsoft.com/office/2022/11/relationships/FeaturePropertyBag\"");
	ASSERT_NE(type, std::string::npos) << rels;
	std::string relationship = Element(rels, "<Relationship ", ">", rels.rfind("<Relationship ", type));
	EXPECT_EQ(Attribute(relationship, "Target"), "featurePropertyBag/featurePropertyBag.xml") << relationship;
}

TEST_F(CheckboxRoundTrip, FeaturePropertyBagContentType)
{
	ASSERT_TRUE(error.empty()) << error;
	std::string types = Part(L"[Content_Types].xml");
	size_t part = types.find("PartName=\"/xl/featurePropertyBag/featurePropertyBag.xml\"");
	ASSERT_NE(part, std::string::npos) << types;
	std::string contentType = Element(types, "<Override ", ">", types.rfind("<Override ", part));
	EXPECT_EQ(Attribute(contentType, "ContentType"), "application/vnd.ms-excel.featurepropertybag+xml") << contentType;
}

TEST_F(CheckboxRoundTrip, CheckboxOnlyOnCheckboxCells)
{
	ASSERT_TRUE(error.empty()) << error;
	std::vector<std::string> xfs = CellXfs(Part(L"xl" FILE_SEPARATOR_STR L"styles.xml"));
	std::string sheet = Part(L"xl" FILE_SEPARATOR_STR L"worksheets" FILE_SEPARATOR_STR L"sheet1.xml");

	// A1 is a plain boolean, B1 (checked) and B2 (unchecked) are checkboxes.
	size_t plain = CellStyle(sheet, "A1");
	size_t checked = CellStyle(sheet, "B1");
	ASSERT_LT(plain, xfs.size());
	ASSERT_LT(checked, xfs.size());
	EXPECT_EQ(CellStyle(sheet, "B2"), checked);
	EXPECT_EQ(xfs[plain].find(kXfComplement), std::string::npos) << xfs[plain];
	EXPECT_NE(xfs[checked].find(kXfComplement), std::string::npos) << xfs[checked];
	EXPECT_NE(xfs[checked].find("<ext uri=\"{C7286773-470A-42A8-94C5-96B5CB345126}\""), std::string::npos) << xfs[checked];
}
