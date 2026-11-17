// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// PPTX -> ODP: the name of a master page.

#include "gtest/gtest.h"

#include "common.h"

#include <string>

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
