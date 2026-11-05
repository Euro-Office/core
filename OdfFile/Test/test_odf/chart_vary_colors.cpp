// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODF chart -> DrawingML c:varyColors. ODF has no such switch, so the value
// follows the chart type: pie and doughnut charts colour each point, the
// other chart types colour each series, whatever the number of series.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
std::wstring convert_chart_part(const std::wstring& fixture)
{
	const std::wstring dir = convert_odf_to_ooxml_dir(fixture);
	if (dir.empty())
		return L"";
	return read_part(dir, L"xl/charts/chart1.xml");
}
} // namespace

TEST(OdfToOoxmlChartVaryColors, OneSeriesPieVariesColors)
{
	const std::wstring xml = convert_chart_part(L"ExampleFiles/chart_pie_one_series.ods");
	ASSERT_FALSE(xml.empty()) << "conversion failed or xl/charts/chart1.xml is missing";
	ASSERT_NE(xml.find(L"<c:pieChart>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<c:varyColors val=\"1\"/>"), std::wstring::npos);
}

TEST(OdfToOoxmlChartVaryColors, OneSeriesDoughnutVariesColors)
{
	const std::wstring xml = convert_chart_part(L"ExampleFiles/chart_ring_one_series.ods");
	ASSERT_FALSE(xml.empty()) << "conversion failed or xl/charts/chart1.xml is missing";
	ASSERT_NE(xml.find(L"<c:doughnutChart>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<c:varyColors val=\"1\"/>"), std::wstring::npos);
}

TEST(OdfToOoxmlChartVaryColors, TwoSeriesBarDoesNotVaryColors)
{
	const std::wstring xml = convert_chart_part(L"ExampleFiles/chart_bar_two_series.ods");
	ASSERT_FALSE(xml.empty()) << "conversion failed or xl/charts/chart1.xml is missing";
	ASSERT_NE(xml.find(L"<c:barChart>"), std::wstring::npos);
	EXPECT_NE(xml.find(L"<c:varyColors val=\"0\"/>"), std::wstring::npos);
	EXPECT_EQ(xml.find(L"<c:varyColors val=\"1\"/>"), std::wstring::npos);
}
