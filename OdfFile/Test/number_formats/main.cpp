// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "../../Reader/Converter/xlsx_numFmts.h"
#include "../../DataTypes/officevaluetype.h"

#include <iostream>
#include <set>
#include <sstream>
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
}

int main()
{
    try
    {
        currency_formats();
        std::cout << "ODS currency format tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
