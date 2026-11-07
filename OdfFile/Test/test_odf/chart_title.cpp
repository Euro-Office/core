// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODF chart title/subtitle -> DrawingML c:title. The title and the subtitle
// are written as one paragraph each into the same c:rich, and a part that is
// empty must not produce a paragraph of its own.

#include "gtest/gtest.h"

#include "OdfFile/Reader/Converter/oox_title.h"

#include <sstream>
#include <string>

namespace
{
using cpdoccore::odf_reader::chart::title;

std::wstring serialize_title(title& main, title& sub)
{
	cpdoccore::oox::oox_title oTitle;
	oTitle.set_content(main);
	oTitle.set_sub_content(sub);

	std::wstringstream oStream;
	oTitle.oox_serialize(oStream);
	return oStream.str();
}

size_t count_of(const std::wstring& text, const std::wstring& what)
{
	size_t nCount = 0;
	for (size_t pos = text.find(what); pos != std::wstring::npos; pos = text.find(what, pos + what.size()))
		++nCount;
	return nCount;
}
} // namespace

TEST(OdfToOoxmlChartTitle, TitleWithoutSubtitleHasOneParagraph)
{
	title main, sub;
	main.bEnabled = true;
	main.content_ = L"Revenue 2024";
	sub.bEnabled = true;

	const std::wstring xml = serialize_title(main, sub);
	EXPECT_NE(xml.find(L"<a:t>Revenue 2024</a:t>"), std::wstring::npos) << xml;
	EXPECT_EQ(1u, count_of(xml, L"<a:p>")) << xml;
}

TEST(OdfToOoxmlChartTitle, SubtitleOnlyIsWritten)
{
	title main, sub;
	sub.bEnabled = true;
	sub.content_ = L"Sub";

	const std::wstring xml = serialize_title(main, sub);
	EXPECT_NE(xml.find(L"<a:t>Sub</a:t>"), std::wstring::npos) << xml;
	EXPECT_EQ(1u, count_of(xml, L"<a:p>")) << xml;
}

TEST(OdfToOoxmlChartTitle, TitleAndSubtitleHaveOneParagraphEach)
{
	title main, sub;
	main.bEnabled = true;
	main.content_ = L"Revenue 2024";
	sub.bEnabled = true;
	sub.content_ = L"Sub";

	const std::wstring xml = serialize_title(main, sub);
	EXPECT_EQ(2u, count_of(xml, L"<a:p>")) << xml;
	EXPECT_LT(xml.find(L"<a:t>Revenue 2024</a:t>"), xml.find(L"<a:t>Sub</a:t>")) << xml;
}
