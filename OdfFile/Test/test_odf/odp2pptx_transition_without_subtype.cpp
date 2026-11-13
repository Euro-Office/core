// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Slide transitions written by the ODF -> PPTX converter
// (anim_transitionFilter::convert_slide_transition_filter).

#include "gtest/gtest.h"

#include "common.h"

#include <string>

// smil:subtype is optional on anim:transitionFilter. A slide transition of
// type blindsWipe, checkerBoardWipe, pushWipe or barnDoorWipe without it
// converts with the PowerPoint default direction instead of aborting.
TEST(OdpToPptxTransition, TransitionWithoutSubtypeConverts)
{
    const std::wstring dir = convert_odf_to_ooxml_dir(L"ExampleFiles/transition_without_subtype.odp");
    ASSERT_FALSE(dir.empty()) << "ODP -> PPTX conversion failed";

    const struct
    {
        const wchar_t* part;
        const wchar_t* transition;
    } slides[] = {
        {L"ppt/slides/slide1.xml", L"<p:blinds/>"},
        {L"ppt/slides/slide2.xml", L"<p:checker/>"},
        {L"ppt/slides/slide3.xml", L"<p:push/>"},
        {L"ppt/slides/slide4.xml", L"<p:split/>"},
    };
    for (const auto& slide : slides)
    {
        const std::wstring xml = read_part(dir, slide.part);
        ASSERT_FALSE(xml.empty()) << "missing part";
        EXPECT_TRUE(is_well_formed(xml));
        EXPECT_NE(xml.find(slide.transition), std::wstring::npos) << "expected transition not written";
    }
}
