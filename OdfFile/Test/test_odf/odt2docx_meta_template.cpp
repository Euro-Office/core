// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// ODT -> DOCX document properties from meta.xml (docProps/app.xml).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

namespace
{
    // docProps/app.xml of the converted fixture; fails the test if it is missing or broken.
    std::wstring convert_app_xml(const std::wstring& fixture)
    {
        const std::wstring dir = convert_odf_to_ooxml_dir(fixture);
        EXPECT_FALSE(dir.empty()) << "ODT -> DOCX conversion failed";
        if (dir.empty())
            return L"";

        const std::wstring xml = read_part(dir, L"docProps/app.xml");
        EXPECT_FALSE(xml.empty()) << "docProps/app.xml is missing";
        EXPECT_TRUE(is_well_formed(xml));
        return xml;
    }
}

// meta:template keeps its data in attributes; meta:generator is optional.
TEST(OdtToDocxMeta, TemplateWithoutGenerator)
{
    const std::wstring xml = convert_app_xml(L"ExampleFiles/meta_template_no_generator.odt");
    EXPECT_NE(xml.find(L"<Template>Letter</Template>"), std::wstring::npos) << xml;
}

TEST(OdtToDocxMeta, TemplateIsNotTheGenerator)
{
    const std::wstring xml = convert_app_xml(L"ExampleFiles/meta_template_with_generator.odt");
    EXPECT_NE(xml.find(L"<Template>Letter</Template>"), std::wstring::npos) << xml;
}

TEST(OdtToDocxMeta, TemplateTitleIsEscaped)
{
    const std::wstring xml = convert_app_xml(L"ExampleFiles/meta_template_ampersand.odt");
    EXPECT_NE(xml.find(L"<Template>A &amp; B</Template>"), std::wstring::npos) << xml;
}

// The template name is xlink:title; without it no Template is written.
TEST(OdtToDocxMeta, TemplateWithoutTitle)
{
    const std::wstring xml = convert_app_xml(L"ExampleFiles/meta_template_href_only.odt");
    EXPECT_EQ(xml.find(L"<Template>"), std::wstring::npos) << xml;
}

TEST(OdtToDocxMeta, GeneratorIsEscaped)
{
    const std::wstring xml = convert_app_xml(L"ExampleFiles/meta_generator_ampersand.odt");
    EXPECT_NE(xml.find(L"<Application>A &amp; B Writer/1.0</Application>"), std::wstring::npos) << xml;
}
