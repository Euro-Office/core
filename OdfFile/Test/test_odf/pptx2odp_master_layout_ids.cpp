// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// PPTX -> ODP: a slide master and its layout become one ODF master page.

#include "gtest/gtest.h"

#include "common.h"

#include <map>
#include <regex>
#include <string>

namespace
{
	// Counts each value of the given attribute in the XML.
	std::map<std::wstring, int> count_values(const std::wstring& xml, const std::wstring& attribute)
	{
		std::map<std::wstring, int> counts;
		const std::wregex re(L" " + attribute + L"=\"([^\"]*)\"");
		for (std::wsregex_iterator it(xml.begin(), xml.end(), re), end; it != end; ++it)
			++counts[(*it)[1].str()];
		return counts;
	}
}

// The master has a plain shape with id 2 and its layout a title placeholder
// with id 2 (PPTX numbers the shapes of each part on its own). On the ODF
// master page every shape needs an id of its own.
TEST(PptxToOdpMasterLayoutIds, EveryShapeHasItsOwnId)
{
	const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/master_layout_shape_ids.pptx");
	ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

	const std::wstring xml = read_part(dir, L"styles.xml");
	ASSERT_FALSE(xml.empty()) << "styles.xml is missing";
	EXPECT_TRUE(is_well_formed(xml));
	for (const wchar_t* name : {L"draw:name=\"Bar\"", L"draw:name=\"Title 1\"", L"draw:name=\"Content 2\""})
		EXPECT_NE(xml.find(name), std::wstring::npos) << "missing shape " << std::wstring(name);

	for (const wchar_t* attribute : {L"xml:id", L"draw:id"})
	{
		const std::map<std::wstring, int> counts = count_values(xml, attribute);
		EXPECT_EQ(counts.size(), 3u) << attribute;
		for (const auto& count : counts)
			EXPECT_EQ(count.second, 1) << attribute << "=\"" << count.first << "\" is used more than once";
	}
}
