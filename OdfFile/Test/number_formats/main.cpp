// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

#include "../../Reader/Converter/xlsx_numFmts.h"
#include "../../DataTypes/officevaluetype.h"
#include "../../Common/CPColorUtils.h"

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void currency_formats()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    const std::vector<std::wstring> codes = {
        L"#,##0.00[$€]",
        L"[Red]#,##0.00[$€]",
        L"#,##0.00",
        L"[$$]#,##0.00",
        L"#,##0.00[$DM]",
        L"#,##0.00[$€];[Red]-#,##0.00[$€]",
        L"#,##0[$¥]",
        L"0.00[$€]"
    };
    std::set<unsigned int> ids;
    for (const auto& code : codes)
    {
        const auto id = formats.add_or_find(code, office_value_type::Currency);
        require(id >= 164, "ODS currency must not use a reserved number format ID");
        require(ids.insert(id).second, "Distinct currency formats must have distinct IDs");
        require(formats.add_or_find(code, office_value_type::Currency) == id,
                "Repeated currency formats must reuse their ID");
    }

    const auto numeric = formats.add_or_find(L"0.0000", office_value_type::Float);
    require(numeric >= 164 && ids.insert(numeric).second, "Custom numeric ID collides with currency");
    require(formats.add_or_find(L"0%", office_value_type::Percentage) == 9,
            "Currency allocation must leave the built-in percentage ID available");
    require(formats.add_or_find(L"0.00%", office_value_type::Percentage) == 10,
            "Currency allocation must leave the decimal percentage ID available");
    require(formats.add_or_find(L"", office_value_type::Currency) == 0, "Empty format must remain General");

    std::wostringstream xml;
    formats.serialize(xml);
    for (const auto& code : codes)
        require(xml.str().find(L"formatCode=\"" + code + L"\"") != std::wstring::npos,
                "Currency format code must survive serialization unchanged");
}

void scientific_builtin_format()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    require(formats.add_or_find(L"0.00E+00", office_value_type::Scientific) == 11,
            "Exact scientific format must use built-in ID 11");
    require(formats.add_or_find(L"0.00E+00", office_value_type::Scientific) == 11,
            "Repeated built-in scientific format must keep ID 11");

    std::wostringstream xml;
    formats.serialize(xml);
    require(xml.str().find(L"<numFmt ") == std::wstring::npos,
            "Built-in scientific format must not need a custom XML entry");
}

void scientific_additional_sections()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    require(formats.add_or_find(L"0.00E+00;-0.00E+00", office_value_type::Scientific) >= 164,
            "Scientific format with an additional section must use a custom ID");
}

void scientific_different_precision()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    const auto first = formats.add_or_find(L"0.000E+00", office_value_type::Scientific);
    const auto second = formats.add_or_find(L"0.0E+00", office_value_type::Scientific);
    require(first >= 164 && second >= 164,
            "Scientific formats with different precision must use custom IDs");
    require(first != second, "Different scientific formats must have distinct IDs");
    require(formats.add_or_find(L"0.00E+00", office_value_type::Scientific) == 11,
            "Custom scientific formats must leave built-in ID 11 available");
}

void scientific_literal()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    require(formats.add_or_find(L"0.00E+00\" units\"", office_value_type::Scientific) >= 164,
            "Scientific format with literal text must use a custom ID");
}

void scientific_serialization()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    // Quotes in the format code must be XML-escaped, while the format itself is preserved.
    struct FormatCase
    {
        const wchar_t* code;
        const wchar_t* serializedCode;
    };
    const FormatCase cases[] = {
        {L"0.00E+00;-0.00E+00", L"0.00E+00;-0.00E+00"},
        {L"0.000E+00", L"0.000E+00"},
        {L"0.00E+00\" units\"", L"0.00E+00&quot; units&quot;"}
    };
    for (const auto& test : cases)
        formats.add_or_find(test.code, office_value_type::Scientific);

    std::wostringstream xml;
    formats.serialize(xml);
    for (const auto& test : cases)
        require(xml.str().find(std::wstring(L"formatCode=\"") + test.serializedCode + L"\"") != std::wstring::npos,
                "Custom scientific format must survive XML serialization");
}

void scientific_repeated_lookup()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    const std::vector<std::wstring> codes = {
        L"0.000E+00", L"0.00E+00;-0.00E+00", L"0.00E+00\" units\""
    };
    std::vector<unsigned int> ids;
    std::set<unsigned int> uniqueIds;
    for (const auto& code : codes)
    {
        const auto id = formats.add_or_find(code, office_value_type::Scientific);
        require(uniqueIds.insert(id).second, "Distinct scientific formats must not share an ID");
        ids.push_back(id);
    }

    std::wostringstream before;
    formats.serialize(before);
    for (size_t i = 0; i < codes.size(); ++i)
        require(formats.add_or_find(codes[i], office_value_type::Scientific) == ids[i],
                "Repeated scientific lookup must reuse the original ID");
    std::wostringstream after;
    formats.serialize(after);
    require(before.str() == after.str(), "Repeated lookup must not add duplicate XML entries");
}

void percentage_builtin_formats()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    require(formats.add_or_find(L"0%", office_value_type::Percentage) == 9,
            "Exact percentage format must use built-in ID 9");
    require(formats.add_or_find(L"0.00%", office_value_type::Percentage) == 10,
            "Exact decimal percentage format must use built-in ID 10");
    require(formats.add_or_find(L"0%", office_value_type::Percentage) == 9 &&
                formats.add_or_find(L"0.00%", office_value_type::Percentage) == 10,
            "Repeated built-in percentage formats must keep their built-in IDs");

    std::wostringstream xml;
    formats.serialize(xml);
    require(xml.str().find(L"<numFmt ") == std::wstring::npos,
            "Built-in percentage formats must not need a custom XML entry");
}

void percentage_custom_formats()
{
    using cpdoccore::odf_types::office_value_type;
    cpdoccore::oox::xlsx_num_fmts formats;
    // Each of these contains "0%" or "0.00%" but shows numbers differently from the built-in formats.
    const std::vector<std::wstring> codes = {
        L"#,##0.00%",
        L"#,##0%",
        L"0.0%",
        L"0.000%",
        L".00%",
        L"0.00%;[Red]-0.00%",
        L"[Red]0.00%",
        L"\"Rendite \"0.00%",
        L"0.00%\" p.a.\""
    };
    std::set<unsigned int> ids;
    for (const auto& code : codes)
    {
        const auto id = formats.add_or_find(code, office_value_type::Percentage);
        require(id >= 164, "Percentage format that is not a built-in one must use a custom ID");
        require(ids.insert(id).second, "Distinct percentage formats must have distinct IDs");
        require(formats.add_or_find(code, office_value_type::Percentage) == id,
                "Repeated percentage formats must reuse their ID");
    }

    std::wostringstream xml;
    formats.serialize(xml);
    for (const auto& code : codes)
    {
        std::wstring escaped;
        for (const wchar_t c : code)
            escaped += c == L'"' ? std::wstring(L"&quot;") : std::wstring(1, c);
        require(xml.str().find(L"formatCode=\"" + escaped + L"\"") != std::wstring::npos,
                "Custom percentage format code must survive serialization unchanged");
    }
}

void number_format_colours()
{
    struct Colour
    {
        const char* label;
        int r, g, b;
        const wchar_t* expected;
    };
    // the eight colours of an Excel number format, and two colours close to red and cyan
    const Colour colours[] = {
        {"black", 0, 0, 0, L"Black"},
        {"white", 255, 255, 255, L"White"},
        {"red", 255, 0, 0, L"Red"},
        {"green", 0, 255, 0, L"Green"},
        {"blue", 0, 0, 255, L"Blue"},
        {"yellow", 255, 255, 0, L"Yellow"},
        {"magenta", 255, 0, 255, L"Magenta"},
        {"cyan", 0, 255, 255, L"Cyan"},
        {"near red", 240, 20, 10, L"Red"},
        {"near cyan", 10, 230, 250, L"Cyan"}
    };
    for (const auto& colour : colours)
    {
        if (cpdoccore::RGBToString(colour.r, colour.g, colour.b) != colour.expected)
            throw std::runtime_error(std::string("Wrong number format colour for ") + colour.label);
    }
}
}

int main()
{
    struct TestCase
    {
        const char* name;
        void (*run)();
    };
    const TestCase tests[] = {
        {"currency formats", currency_formats},
        {"scientific built-in format", scientific_builtin_format},
        {"scientific additional sections", scientific_additional_sections},
        {"scientific different precision", scientific_different_precision},
        {"scientific literal", scientific_literal},
        {"scientific serialization", scientific_serialization},
        {"scientific repeated lookup", scientific_repeated_lookup},
        {"percentage built-in formats", percentage_builtin_formats},
        {"percentage custom formats", percentage_custom_formats},
        {"number format colours", number_format_colours}
    };
    int failures = 0;
    for (const auto& test : tests)
    {
        try
        {
            test.run();
            std::cout << "[PASS] " << test.name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }
    return failures == 0 ? 0 : 1;
}
