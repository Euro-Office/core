// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

// Function names that OpenFormula and OOXML spell differently. The ODF side is what
// LibreOffice writes for an Excel workbook saved as ODS, the OOXML side is what Excel stores.

#include "gtest/gtest.h"

#include "OdfFile/Formulas/formulasconvert.h"

#include <string>
#include <utility>
#include <vector>

namespace
{
using cpdoccore::formulasconvert::odf2oox_converter;
using cpdoccore::formulasconvert::oox2odf_converter;

// ODF formula, OOXML formula
const std::vector<std::pair<std::wstring, std::wstring>> renamed = {
    {L"of:=COM.MICROSOFT.CONCAT(\"a\";[.A1])", L"_xlfn.CONCAT(\"a\",A1)"},
    {L"of:=COM.MICROSOFT.TEXTJOIN(\",\";1;COM.MICROSOFT.SORT([.A2:.A9];1;-1))",
     L"_xlfn.TEXTJOIN(\",\",1,_xlfn._xlws.SORT(A2:A9,1,-1))"},
    {L"of:=COM.MICROSOFT.CEILING(2.5;1)", L"CEILING(2.5,1)"},
    {L"of:=LEGACY.FDIST(15.2;6;4)+LEGACY.NORMSDIST(1)", L"FDIST(15.2,6,4)+NORMSDIST(1)"},
    {L"of:=FDIST(15.2;6;4;1)", L"_xlfn.F.DIST(15.2,6,4,1)"},
    {L"of:=FINV(0.01;6;4)", L"_xlfn.F.INV(0.01,6,4)"},
    {L"of:=SKEWP([.A1:.A9])", L"_xlfn.SKEW.P(A1:A9)"},
    {L"of:=FORMULA([.A1])", L"_xlfn.FORMULATEXT(A1)"},
    {L"of:=ISFORMULA([.A1])", L"_xlfn.ISFORMULA(A1)"},
    {L"of:=ACOT(1)", L"_xlfn.ACOT(1)"},
    {L"of:=LOG10(1000)+BIN2DEC(1100100)", L"LOG10(1000)+BIN2DEC(1100100)"},
};
}

TEST(OdsFormulaFunctionNames, Import)
{
    odf2oox_converter converter;
    for (const auto& formula : renamed)
        EXPECT_EQ(formula.second, converter.convert(formula.first));
}

TEST(OdsFormulaFunctionNames, Export)
{
    oox2odf_converter converter;
    for (const auto& formula : renamed)
        EXPECT_EQ(formula.first, converter.convert_formula(formula.second));
}

TEST(OdsFormulaFunctionNames, LegacyExcelNamesExportAsLegacy)
{
    oox2odf_converter converter;
    EXPECT_EQ(L"of:=LEGACY.FINV(0.01;6;4)", converter.convert_formula(L"FINV(0.01,6,4)"));
}

TEST(OdsFormulaFunctionNames, UnknownComMicrosoftNameGetsXlfnPrefix)
{
    odf2oox_converter converter;
    EXPECT_EQ(L"_xlfn.NEWFUNCTION(1)", converter.convert(L"of:=COM.MICROSOFT.NEWFUNCTION(1)"));
}

TEST(OdsFormulaFunctionNames, StringsKeepTheirText)
{
    odf2oox_converter converter;
    EXPECT_EQ(L"CONCATENATE(\"FORMULA(\",\"COM.MICROSOFT.CONCAT(\")",
              converter.convert(L"of:=CONCATENATE(\"FORMULA(\";\"COM.MICROSOFT.CONCAT(\")"));
}

// Conditional formatting and data validation pass the condition without "of:="
TEST(OdsFormulaFunctionNames, ImportWithoutFormulaPrefix)
{
    odf2oox_converter converter;
    EXPECT_EQ(L"A1>_xlfn.MAXIFS(B1:B9,C1:C9,1)", converter.convert(L"[.A1]>COM.MICROSOFT.MAXIFS([.B1:.B9];[.C1:.C9];1)"));
}

TEST(OdsFormulaFunctionNames, ValidationConditionKeepsItsName)
{
    odf2oox_converter converter;
    EXPECT_EQ(L"is-true-formula(A1>0)", converter.convert(L"of:is-true-formula([.A1]>0)"));
}
