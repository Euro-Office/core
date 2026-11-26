// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// PPTX -> ODP: the name of a master page.

#include "gtest/gtest.h"

#include "common.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

// The ODP -> PPTX converter gives the slide layout the name of the ODP master
// page and the master no name. The fixture is that output: ExampleFiles/
// master_name.pptx is the conversion of a one-slide ODP with the master page
// "Default". Converted back, the master page must keep the name "Default".
// Before, it became "Default_Default", and longer on each open and save.
TEST(PptxToOdp, MasterPageKeepsItsName)
{
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/master_name.pptx");
    ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

    const std::wstring styles = read_part(dir, L"styles.xml");
    ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
    EXPECT_TRUE(is_well_formed(styles));
    EXPECT_NE(styles.find(L"<style:master-page style:name=\"Default\""), std::wstring::npos);
    EXPECT_EQ(styles.find(L"Default_"), std::wstring::npos);

    const std::wstring content = read_part(dir, L"content.xml");
    ASSERT_FALSE(content.empty()) << "content.xml is missing";
    EXPECT_NE(content.find(L"draw:master-page-name=\"Default\""), std::wstring::npos);
}

// The names of all the master pages in styles.xml, in their order.
static std::vector<std::wstring> master_page_names(const std::wstring& styles)
{
    std::vector<std::wstring> names;
    const std::wstring tag = L"<style:master-page style:name=\"";
    for (size_t pos = styles.find(tag); pos != std::wstring::npos; pos = styles.find(tag, pos))
    {
        pos += tag.size();
        names.push_back(styles.substr(pos, styles.find(L'"', pos) - pos));
    }
    return names;
}

// The master page of each slide in content.xml, in the order of the slides.
static std::vector<std::wstring> slide_master_pages(const std::wstring& content)
{
    std::vector<std::wstring> names;
    const std::wstring attr = L"draw:master-page-name=\"";
    for (size_t pos = content.find(L"<draw:page "); pos != std::wstring::npos; pos = content.find(L"<draw:page ", pos))
    {
        const size_t end = content.find(L'>', pos);
        const size_t at = content.find(attr, pos);
        pos = end;
        if (at == std::wstring::npos || at > end)
            continue;
        const size_t begin = at + attr.size();
        names.push_back(content.substr(begin, content.find(L'"', begin) - begin));
    }
    return names;
}

// Two master pages of the same name get a number. The number must not give the
// name of another master page, and each master page keeps its own name.
// ExampleFiles/master_clash.odp (made with LibreOffice 25.2) has the master
// pages "Lights" and "Lights2". Slides 1-2 are on "Lights" with two layouts
// (title, title and content), slides 3-4 on "Lights2". The ODP -> PPTX
// converter gives both layouts of slides 1-2 the name "Lights". Before, the
// file after ODP -> PPTX -> ODP had two master pages "Lights2", and slides 3-4
// used the wrong one.
TEST(PptxToOdp, MasterPageNumberIsNotAnotherName)
{
    ASSERT_EQ(0, convert_single(L"ExampleFiles/master_clash.odp")) << "ODP -> PPTX conversion failed";
    const std::wstring dir = convert_ooxml_to_odf_dir(L"ExampleFiles/master_clash.odp-tmp.pptx");
    ASSERT_FALSE(dir.empty()) << "PPTX -> ODP conversion failed";

    const std::wstring styles = read_part(dir, L"styles.xml");
    ASSERT_FALSE(styles.empty()) << "styles.xml is missing";
    EXPECT_TRUE(is_well_formed(styles));
    const std::vector<std::wstring> masters = master_page_names(styles);
    EXPECT_EQ((std::vector<std::wstring>{L"Lights", L"Lights3", L"Lights2"}), masters);
    EXPECT_EQ(masters.size(), std::set<std::wstring>(masters.begin(), masters.end()).size())
        << "two master pages have the same name";

    const std::wstring content = read_part(dir, L"content.xml");
    ASSERT_FALSE(content.empty()) << "content.xml is missing";
    const std::vector<std::wstring> slides = slide_master_pages(content);
    ASSERT_EQ(4u, slides.size());
    EXPECT_EQ(L"Lights", slides[0]);
    // Slide 2 has the other layout of "Lights": the first free number, as
    // "Lights2" is the name of another master page.
    EXPECT_EQ(L"Lights3", slides[1]);
    // Slides 3-4 keep their master "Lights2".
    EXPECT_EQ(L"Lights2", slides[2]);
    EXPECT_EQ(L"Lights2", slides[3]);
    // Each slide refers to a master page of the file.
    for (const std::wstring& slide : slides)
        EXPECT_NE(std::find(masters.begin(), masters.end(), slide), masters.end()) << "no master page " << slide;
}
