// SPDX-License-Identifier: AGPL-3.0-only

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

void date_time_formats()
{
    using cpdoccore::odf_types::office_value_type;
    struct format { const wchar_t* code; char type; };
    // The first two matched a reserved ID exactly; the rest reached one only
    // through a substring match that lost part of the format -- d-mmm-yyyy took
    // the two-digit-year ID 15, hh:mm:ss.00 the whole-second ID 21. A reserved ID
    // implies its own locale-dependent code, so none of them may take one.
    const std::vector<format> codes = {
        { L"mm-dd-yy",    office_value_type::Date },
        { L"d-mmm-yy",    office_value_type::Date },
        { L"d-mmm-yyyy",  office_value_type::Date },
        { L"yyyy-mm-dd",  office_value_type::Date },
        { L"dd.mm.yyyy",  office_value_type::Date },
        { L"h:mm:ss",     office_value_type::Time },
        { L"hh:mm:ss",    office_value_type::Time },
        { L"hh:mm:ss.00", office_value_type::Time },
        { L"[h]:mm:ss",   office_value_type::Time },
        { L"hh:mm AM/PM", office_value_type::Time },
    };

    cpdoccore::oox::xlsx_num_fmts formats;
    std::set<unsigned int> ids;
    for (const auto& entry : codes)
    {
        const auto id = formats.add_or_find(entry.code, entry.type);
        require(id >= 164, "ODS date and time formats must not use a reserved number format ID");
        require(ids.insert(id).second, "Distinct date and time formats must have distinct IDs");
        require(formats.add_or_find(entry.code, entry.type) == id,
                "Repeated date and time formats must reuse their ID");
    }

    require(formats.add_or_find(L"0%", office_value_type::Percentage) == 9,
            "Date and time allocation must leave the built-in percentage ID available");

    std::wostringstream xml;
    formats.serialize(xml);
    for (const auto& entry : codes)
        require(xml.str().find(std::wstring(L"formatCode=\"") + entry.code + L"\"") != std::wstring::npos,
                "Date and time format codes must survive serialization unchanged");
}

int main()
{
    try
    {
        currency_formats();
        date_time_formats();
        std::cout << "ODS number format tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
