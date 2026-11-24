#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Generate the xlsx_condfmt_test fixtures (see OOXML/XlsxFormat/Worksheets/test).

Builds ExampleFiles/condfmt_rules.xlsx from scratch (pure standard library):
a minimal single-sheet workbook whose sheet1.xml carries seven
<conditionalFormatting> blocks, one per rule family, plus three dxf entries in
styles.xml. This covers the areas most actively fixed on
OOXML/XlsxFormat/Worksheets/ConditionalFormatting.cpp (xlsx cfRule XML <->
XLSB Biff12 records): cellIs (two operators), expression, colorScale,
dataBar, iconSet, top10, and the dxf (differential styles) plumbing.

Derived variants next to the seed:

- condfmt_edge.xlsx — the "between" rule points at a dangling dxfId="99"
  that does not exist in styles.xml, exercising the guarded conversion paths
  (cf. upstream history "Fix undefined values in conversion").

- condfmt_x14.xlsx — the seed plus the Excel 2010 x14 extension of the
  dataBar rule: inline extLst/x14:id inside <dataBar> and a worksheet-level
  extLst carrying x14:conditionalFormattings/x14:cfRule type="databar" with
  negativeFillColor/axisColor. Covers the conditionalFormattings14 /
  worksheet ext conversion paths.

Usage:
    python3 make_fixtures.py <out-dir>     # write the three fixtures into <out-dir>
    python3 make_fixtures.py --check <dir>  # verify the three fixtures

Only uses the Python standard library.
"""

import os
import re
import sys
import zipfile

NS_MAIN = "http://schemas.openxmlformats.org/spreadsheetml/2006/main"
NS_R = "http://schemas.openxmlformats.org/officeDocument/2006/relationships"
NS_PKG_REL = "http://schemas.openxmlformats.org/package/2006/relationships"
NS_CT = "http://schemas.openxmlformats.org/package/2006/content-types"
NS_X14 = "http://schemas.microsoft.com/office/spreadsheetml/2009/9/main"

REL_OFFICE_DOCUMENT = (
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
)
REL_WORKSHEET = (
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet"
)
REL_STYLES = (
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
)

CT_WORKBOOK = (
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"
)
CT_WORKSHEET = (
    "application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"
)
CT_STYLES = "application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"

# Marker values deliberately chosen to be distinctive in both the XML output
# and the UTF-16LE payloads of the XLSB intermediate.
X14_DATABAR_ID = "{D1111A3B-9692-4701-9B44-F5C37DC98B77}"

CONTENT_TYPES = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Types xmlns="%s">'
    '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
    '<Default Extension="xml" ContentType="application/xml"/>'
    '<Override PartName="/xl/workbook.xml" ContentType="%s"/>'
    '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="%s"/>'
    '<Override PartName="/xl/styles.xml" ContentType="%s"/>'
    "</Types>" % (NS_CT, CT_WORKBOOK, CT_WORKSHEET, CT_STYLES)
)

RELS_ROOT = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="%s">'
    '<Relationship Id="rId1" Type="%s" Target="xl/workbook.xml"/>'
    "</Relationships>" % (NS_PKG_REL, REL_OFFICE_DOCUMENT)
)

WORKBOOK = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<workbook xmlns="%s" xmlns:r="%s"><sheets>'
    '<sheet name="Sheet1" sheetId="1" r:id="rId1"/>'
    "</sheets></workbook>" % (NS_MAIN, NS_R)
)

WORKBOOK_RELS = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<Relationships xmlns="%s">'
    '<Relationship Id="rId1" Type="%s" Target="worksheets/sheet1.xml"/>'
    '<Relationship Id="rId2" Type="%s" Target="styles.xml"/>'
    "</Relationships>" % (NS_PKG_REL, REL_WORKSHEET, REL_STYLES)
)

# The border element lists all five side children explicitly (the
# shape Excel itself writes): a side-less <border/> leaves the
# nullable side objects of CBorder uninitialized and crashes
# xlsx->XLSB conversion (null this-pointer member call in
# CBorderProp::toBin — Excel never emits a side-less border, so
# production files do not hit it; filed as #180).

STYLES = (
    '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    '<styleSheet xmlns="%s">'
    '<fonts count="1"><font><sz val="11"/><name val="Calibri"/></font></fonts>'
    '<fills count="1"><fill><patternFill patternType="none"/></fill></fills>'
    '<borders count="1"><border><left/><right/><top/><bottom/><diagonal/></border></borders>'
    '<cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>'
    '<cellXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0" xfId="0"/></cellXfs>'
    # Three dxf (differential format) entries referenced by the cellIs and
    # expression rules. Distinctive colors so the test can assert them.
    '<dxfs count="3">'
    '<dxf><font><color rgb="FF9C0006"/></font>'
    '<fill><patternFill><bgColor rgb="FFFFC7CE"/></patternFill></fill></dxf>'
    '<dxf><font><color rgb="FF006100"/></font>'
    '<fill><patternFill><bgColor rgb="FFC6EFCE"/></patternFill></fill></dxf>'
    '<dxf><font><b/><color rgb="FF1F4E79"/></font></dxf>'
    "</dxfs>"
    "</styleSheet>" % NS_MAIN
)


def sheet_data_rows():
    cells = []
    for col in "ABCDEFG":
        for row in range(1, 11):
            cells.append('<c r="%s%d"><v>%d</v></c>' % (col, row, row * 7))
    return "<sheetData>%s</sheetData>" % "".join(cells)


def cf_block(sqref, rule):
    return '<conditionalFormatting sqref="%s">%s</conditionalFormatting>' % (
        sqref,
        rule,
    )


def classic_rules(dxf_between="1"):
    """The seven classic (ECMA-376) conditionalFormatting blocks."""
    blocks = [
        cf_block(
            "A1:A10",
            '<cfRule type="cellIs" dxfId="0" priority="1" operator="greaterThan">'
            "<formula>101</formula></cfRule>",
        ),
        cf_block(
            "B1:B10",
            '<cfRule type="cellIs" dxfId="%s" priority="2" operator="between">'
            "<formula>1234</formula><formula>5678</formula></cfRule>" % dxf_between,
        ),
        cf_block(
            "C1:C10",
            '<cfRule type="expression" dxfId="2" priority="3">'
            "<formula>$C$2&gt;2026</formula></cfRule>",
        ),
        cf_block(
            "D1:D10",
            '<cfRule type="colorScale" priority="4"><colorScale>'
            '<cfvo type="min"/><cfvo type="percentile" val="50"/><cfvo type="max"/>'
            '<color rgb="FFFF0000"/><color rgb="FF00B050"/><color rgb="FF0070C0"/>'
            "</colorScale></cfRule>",
        ),
        cf_block(
            "E1:E10",
            '<cfRule type="dataBar" priority="5"><dataBar>'
            '<cfvo type="min"/><cfvo type="max"/><color rgb="FF638EC6"/>'
            "</dataBar></cfRule>",
        ),
        # ST_IconSetType (OOXML/Common/SimpleTypes_Spreadsheet.cpp) only knows
        # 3TrafficLights1 / 3TrafficLights2 — any other value is silently turned
        # into 3Arrows, so the fixture must use a schema-valid name.
        cf_block(
            "F1:F10",
            '<cfRule type="iconSet" priority="6"><iconSet iconSet="3TrafficLights1">'
            '<cfvo type="percent" val="0"/><cfvo type="percent" val="33"/>'
            '<cfvo type="percent" val="67"/></iconSet></cfRule>',
        ),
        # In a conditional-formatting rule, rank is a <cfRule> ATTRIBUTE
        # (a <top10> child element belongs to autoFilter, not cfRule) — the
        # reader skips such a child and toBin falls back to rank 1.
        cf_block(
            "G1:G10",
            '<cfRule type="top10" priority="7" rank="10"/>',
        ),
    ]
    return "".join(blocks)


def worksheet_xml(dxf_between="1", x14_databar=False):
    parts = [
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<worksheet xmlns="%s" xmlns:x14="%s">' % (NS_MAIN, NS_X14),
        sheet_data_rows(),
        classic_rules(dxf_between=dxf_between),
    ]
    if x14_databar:
        # The classic dataBar block of the seed is replaced by the same block
        # with the x14 extension: x14:id inside the dataBar extLst, and the
        # matching x14:cfRule in the worksheet-level extLst.
        parts = [
            '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
            '<worksheet xmlns="%s" xmlns:x14="%s">' % (NS_MAIN, NS_X14),
            sheet_data_rows(),
            classic_rules(dxf_between=dxf_between).replace(
                '<cfRule type="dataBar" priority="5"><dataBar>'
                '<cfvo type="min"/><cfvo type="max"/><color rgb="FF638EC6"/>'
                "</dataBar></cfRule>",
                '<cfRule type="dataBar" priority="5"><dataBar>'
                '<cfvo type="min"/><cfvo type="max"/><color rgb="FF638EC6"/>'
                "<extLst><ext uri=\"{B025F937-C7B1-47D3-B67F-A62E662629BB}\">"
                "<x14:id>%s</x14:id></ext></extLst></dataBar></cfRule>" % X14_DATABAR_ID,
            ),
            (
                "<extLst><ext xmlns:x14=\"%s\" uri=\"{78C0D931-6437-407d-A8EE-F0AAD7539E65}\">"
                "<x14:conditionalFormattings>"
                '<x14:conditionalFormatting sqref="E1:E10">'
                '<x14:cfRule type="databar" id="%s" priority="5">'
                '<x14:dataBar minLength="0" maxLength="100" gradient="1"'
                ' negativeBarColorSameAsPositive="0" axisPosition="middle">'
                '<x14:negativeFillColor rgb="FFFF0000"/>'
                '<x14:axisColor rgb="FFD00000"/>'
                '<x14:cfvo type="min"/><x14:cfvo type="max"/>'
                "</x14:dataBar></x14:cfRule></x14:conditionalFormatting>"
                "</x14:conditionalFormattings></ext></extLst>"
                % (NS_X14, X14_DATABAR_ID)
            ),
        ]
    parts.append("</worksheet>")
    return "".join(parts)


def build_package(path, sheet_xml):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", CONTENT_TYPES)
        z.writestr("_rels/.rels", RELS_ROOT)
        z.writestr("xl/workbook.xml", WORKBOOK)
        z.writestr("xl/_rels/workbook.xml.rels", WORKBOOK_RELS)
        z.writestr("xl/styles.xml", STYLES)
        z.writestr("xl/worksheets/sheet1.xml", sheet_xml)
    print("wrote", path)


def build_all(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    build_package(
        os.path.join(out_dir, "condfmt_rules.xlsx"), worksheet_xml()
    )
    build_package(
        os.path.join(out_dir, "condfmt_edge.xlsx"),
        worksheet_xml(dxf_between="99"),  # dangling dxfId, no such dxf
    )
    build_package(
        os.path.join(out_dir, "condfmt_x14.xlsx"), worksheet_xml(x14_databar=True)
    )


def check(dir_path):
    def part(z, name):
        return z.read(name).decode("utf-8")

    ok = True

    seed = zipfile.ZipFile(os.path.join(dir_path, "condfmt_rules.xlsx"))
    s = part(seed, "xl/worksheets/sheet1.xml")
    st = part(seed, "xl/styles.xml")
    ok = ok and s.count("<cfRule ") == 7
    ok = ok and all(
        ('type="%s"' % t) in s for t in ("cellIs", "expression", "colorScale",
                                          "dataBar", "iconSet", "top10")
    )
    ok = ok and 'operator="greaterThan"' in s and 'operator="between"' in s
    ok = ok and "$C$2&gt;2026" in s and "5678" in s
    ok = ok and 'sqref="A1:A10"' in s and 'sqref="G1:G10"' in s
    # Valid-SpreadsheetML shape of the two rules the review corrected:
    # a schema-valid iconSet name, and rank as a cfRule attribute (the
    # autoFilter-style <top10> child must not be present).
    ok = ok and 'iconSet="3TrafficLights1"' in s and "FF638EC6" in s
    ok = ok and '<cfRule type="top10" priority="7" rank="10"/>' in s
    ok = ok and "<top10 " not in s
    ok = ok and st.count("<dxf>") == 3 and "FFFFC7CE" in st and "FF9C0006" in st

    edge = zipfile.ZipFile(os.path.join(dir_path, "condfmt_edge.xlsx"))
    e = part(edge, "xl/worksheets/sheet1.xml")
    ok = ok and 'dxfId="99"' in e and e.count("<cfRule ") == 7

    x14 = zipfile.ZipFile(os.path.join(dir_path, "condfmt_x14.xlsx"))
    x = part(x14, "xl/worksheets/sheet1.xml")
    ok = ok and X14_DATABAR_ID in x and "x14:conditionalFormattings" in x
    ok = ok and "negativeFillColor" in x and "axisPosition=\"middle\"" in x

    print("check:", "OK" if ok else "FAILED")
    return ok


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--check":
        sys.exit(0 if check(sys.argv[2]) else 1)
    if len(sys.argv) == 2:
        build_all(sys.argv[1])
        return
    raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
