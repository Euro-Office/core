// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODS -> XLSX conversion of cell font sizes and column widths. A cell font without a size is
// LibreOffice's 10 pt, and a column keeps its width when the editor draws it: the editor turns a
// width w (in characters) into (256 * w + 128 / mdw) / 256 * mdw pixels, mdw being the digit
// width of the default font in whole pixels.

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
	// One sheet, the Default cell style names a font but no size:
	// A1 Default, B1 bold (no size), C1 14 pt; B1 and C1 have automatic styles based on Default.
	// C1 has a conditional format with the style "Good", which only sets a colour.
	// Columns 1 cm, 2.5 cm, 5 cm.
	std::wstring part(const std::wstring& name)
	{
		static const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/sheet_font_size_column_width.ods");
		return dir.empty() ? std::wstring() : read_part(dir, name);
	}

	// The values of attribute name in the start tags of every element tag, in document order.
	std::vector<std::wstring> attributes(const std::wstring& xml, const std::wstring& tag, const std::wstring& name)
	{
		std::vector<std::wstring> values;
		for (size_t pos = xml.find(L"<" + tag + L" "); pos != std::wstring::npos; pos = xml.find(L"<" + tag + L" ", pos + 1))
		{
			const size_t end = xml.find(L">", pos);
			const std::wstring key = L" " + name + L"=\"";
			const size_t at = xml.find(key, pos);
			if (at == std::wstring::npos || at > end)
				continue;
			const size_t begin = at + key.size();
			values.push_back(xml.substr(begin, xml.find(L"\"", begin) - begin));
		}
		return values;
	}

	// The fonts element.
	std::wstring fonts()
	{
		const std::wstring styles = part(L"xl/styles.xml");
		const size_t begin = styles.find(L"<fonts");
		const size_t end = styles.find(L"</fonts>");
		return begin == std::wstring::npos || end == std::wstring::npos ? std::wstring() : styles.substr(begin, end - begin);
	}

	double editor_pixels(double width, int mdw)
	{
		return std::floor((256. * width + std::floor(128. / mdw)) / 256. * mdw);
	}
}

TEST(OdsToXlsxFontSize, FontsWithoutSizeGetTenPoints)
{
	const std::wstring f = fonts();
	ASSERT_FALSE(f.empty()) << "ODS -> XLSX conversion failed";

	const std::vector<std::wstring> sizes = attributes(f, L"sz", L"val");
	ASSERT_GE(sizes.size(), 3u) << f;
	// Font 0 (Default) and the bold font have no size in the ODS; the third is 14 pt.
	EXPECT_EQ(sizes[0], L"10") << f;
	EXPECT_EQ(std::count(sizes.begin(), sizes.end(), L"10"), 2) << f;
	EXPECT_EQ(std::count(sizes.begin(), sizes.end(), L"14"), 1) << f;
}

// C1 (14 pt) has a conditional format whose style only sets a colour: its font must not carry
// a size, or the 14 pt cell would shrink to 10 pt whenever the condition holds.
TEST(OdsToXlsxFontSize, ConditionalFormatKeepsTheCellSize)
{
	const std::wstring styles = part(L"xl/styles.xml");
	ASSERT_FALSE(styles.empty()) << "ODS -> XLSX conversion failed";

	const size_t begin = styles.find(L"<dxfs");
	const size_t end = styles.find(L"</dxfs>");
	ASSERT_NE(begin, std::wstring::npos) << styles;
	ASSERT_NE(end, std::wstring::npos) << styles;
	const std::wstring dxfs = styles.substr(begin, end - begin);
	EXPECT_NE(dxfs.find(L"<color"), std::wstring::npos) << dxfs;
	EXPECT_EQ(dxfs.find(L"<sz"), std::wstring::npos) << dxfs;
}

// Whatever the digit width of the test fonts, one whole-pixel digit width must give back every
// column's ODS width to the pixel.
TEST(OdsToXlsxColumnWidth, EditorDrawsTheOdsWidth)
{
	const std::wstring sheet = part(L"xl/worksheets/sheet1.xml");
	ASSERT_FALSE(sheet.empty()) << "ODS -> XLSX conversion failed";
	EXPECT_TRUE(is_well_formed(sheet));

	const std::vector<std::wstring> widths = attributes(sheet, L"col", L"width");
	ASSERT_GE(widths.size(), 3u) << sheet;
	const double pixels[3] = {1 / 2.54 * 96, 2.5 / 2.54 * 96, 5 / 2.54 * 96};

	bool found = false;
	for (int mdw = 4; mdw <= 16 && !found; ++mdw)
	{
		bool all = true;
		for (int i = 0; i < 3; ++i)
			all = all && std::fabs(editor_pixels(std::stod(widths[i]), mdw) - pixels[i]) <= 1.;
		found = all;
	}
	EXPECT_TRUE(found) << sheet;
}
