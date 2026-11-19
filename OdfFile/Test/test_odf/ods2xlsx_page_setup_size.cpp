// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of the page size of style:page-layout-properties. LibreOffice prints a
// sheet on a page of fo:page-width x fo:page-height; style:print-orientation does not turn it.
// XLSX gives the paper upright and turns it with orientation="landscape".

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
	// The fixture has one sheet per case, each with its own page layout:
	// 1 style:print-orientation="landscape", no page size
	// 2 A3 landscape: 42cm x 29.7cm, landscape
	// 3 Letter landscape: 11in x 8.5in, landscape
	// 4 A4 portrait size with style:print-orientation="landscape"
	// 5 A4 landscape size with style:print-orientation="portrait"
	// 6 300mm x 200mm (no paper size id), landscape
	// 7 A4 portrait, portrait
	// 8 style:print-orientation="landscape" and style:scale-to="71%", no page size
	std::wstring sheet_xml(int sheet)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_page_setup_size.ods");
		if (dir.empty())
			return std::wstring();
		return read_part(dir, L"xl/worksheets/sheet" + std::to_wstring(sheet) + L".xml");
	}

	// The start tag of the first pageSetup element, empty if none.
	std::wstring page_setup(const std::wstring& xml)
	{
		size_t pos = xml.find(L"<pageSetup ");
		if (pos == std::wstring::npos)
			pos = xml.find(L"<pageSetup/");
		if (pos == std::wstring::npos)
			return std::wstring();
		const size_t end = xml.find(L">", pos);
		return end == std::wstring::npos ? std::wstring() : xml.substr(pos, end - pos + 1);
	}

	// Value of an attribute in a start tag, or "(none)" if it is absent.
	std::wstring attribute(const std::wstring& tag, const std::wstring& name)
	{
		const std::wstring key = L" " + name + L"=\"";
		const size_t pos = tag.find(key);
		if (pos == std::wstring::npos)
			return L"(none)";
		const size_t begin = pos + key.size();
		const size_t end = tag.find(L"\"", begin);
		return end == std::wstring::npos ? L"(none)" : tag.substr(begin, end - begin);
	}
}

// Without a page size LibreOffice prints on its default upright page: the flag alone turns nothing.
TEST(OdsToXlsxPageSetupSize, OrientationFlagWithoutSizeTurnsNothing)
{
	const std::wstring sheet = sheet_xml(1);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	// Only the landscape flag, no size and nothing else for a pageSetup: none is written.
	const std::wstring setup = page_setup(sheet);
	EXPECT_TRUE(setup.empty()) << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"(none)") << setup;
	EXPECT_EQ(attribute(setup, L"paperSize"), L"(none)");
}

TEST(OdsToXlsxPageSetupSize, A3LandscapeIsA3TurnedLandscape)
{
	const std::wstring sheet = sheet_xml(2);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"paperSize"), L"8") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"landscape");
	EXPECT_EQ(attribute(setup, L"paperWidth"), L"(none)");
}

TEST(OdsToXlsxPageSetupSize, LetterLandscapeIsLetterTurnedLandscape)
{
	const std::wstring sheet = sheet_xml(3);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"paperSize"), L"1") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"landscape");
}

// An upright page with the landscape flag is how older versions of this converter wrote a
// landscape sheet: it stays landscape.
TEST(OdsToXlsxPageSetupSize, UprightSizeWithLandscapeFlagStaysLandscape)
{
	const std::wstring sheet = sheet_xml(4);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"paperSize"), L"9") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"landscape");
}

// The page is wider than high; the portrait flag does not turn it back.
TEST(OdsToXlsxPageSetupSize, WideSizeWinsOverPortraitFlag)
{
	const std::wstring sheet = sheet_xml(5);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"paperSize"), L"9") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"landscape");
}

// A size without a paper size id gets no paper size id, and its paper is written upright and turned.
// The unit form of the width and height is not pinned here.
TEST(OdsToXlsxPageSetupSize, CustomWideSizeIsTurned)
{
	const std::wstring sheet = sheet_xml(6);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"paperSize"), L"(none)") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"landscape");
	const std::wstring paper_width = attribute(setup, L"paperWidth");
	const std::wstring paper_height = attribute(setup, L"paperHeight");
	ASSERT_NE(paper_width, L"(none)") << setup;
	ASSERT_NE(paper_height, L"(none)") << setup;
	EXPECT_LT(std::stod(paper_width), std::stod(paper_height)) << setup;
}

TEST(OdsToXlsxPageSetupSize, A4PortraitUnchanged)
{
	const std::wstring sheet = sheet_xml(7);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"paperSize"), L"9") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"portrait");
}

// Without a page size the sheet is printed upright, and its scale is kept: the orientation is written
// so that the pageSetup is not dropped on the way through the editor's binary format.
TEST(OdsToXlsxPageSetupSize, ScaleWithoutSizeKeepsItsPageSetup)
{
	const std::wstring sheet = sheet_xml(8);
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::wstring setup = page_setup(sheet);
	EXPECT_EQ(attribute(setup, L"scale"), L"71") << setup;
	EXPECT_EQ(attribute(setup, L"orientation"), L"portrait");
	EXPECT_EQ(attribute(setup, L"paperSize"), L"(none)");
}
