#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Package the readable ODF fixture; optionally verify it through a real x2t."""

import argparse
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET
from zipfile import ZIP_DEFLATED, ZIP_STORED, ZipFile

NS = {
    "office": "urn:oasis:names:tc:opendocument:xmlns:office:1.0",
    "style": "urn:oasis:names:tc:opendocument:xmlns:style:1.0",
    "number": "urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0",
    "table": "urn:oasis:names:tc:opendocument:xmlns:table:1.0",
    "text": "urn:oasis:names:tc:opendocument:xmlns:text:1.0",
    "s": "http://schemas.openxmlformats.org/spreadsheetml/2006/main",
}


def make_ods(output):
    for prefix, uri in NS.items():
        ET.register_namespace(prefix, uri)
    document = ET.parse(Path(__file__).with_name("currency.fods")).getroot()
    document.tag = f"{{{NS['office']}}}document-content"
    del document.attrib[f"{{{NS['office']}}}mimetype"]
    document.remove(document.find("office:styles", NS))
    mime = "application/vnd.oasis.opendocument.spreadsheet"
    manifest = f'''<?xml version="1.0" encoding="UTF-8"?>
<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0" manifest:version="1.2">
 <manifest:file-entry manifest:full-path="/" manifest:media-type="{mime}"/>
 <manifest:file-entry manifest:full-path="content.xml" manifest:media-type="text/xml"/>
</manifest:manifest>'''
    output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
        archive.writestr("mimetype", mime, compress_type=ZIP_STORED)
        archive.writestr("content.xml", ET.tostring(document, encoding="utf-8", xml_declaration=True))
        archive.writestr("META-INF/manifest.xml", manifest)


CURRENCY_CELLS = [
    ("B2", "€", "1234.56"), ("B3", "€", "-1234.56"),
    ("B4", "$", "1234.56"), ("B5", None, "1234.56"), ("B6", "€", "1234.56"),
]

# B7/B8 are controls rather than currencies: a currency format that collided on
# reserved ID 7 used to walk 7 -> 8 -> 9 -> 10 and take the built-in percentage
# formats with it, so a currency-free cell could come back as a percentage.
CONTROL_CELLS = [("B7", "percentage"), ("B8", "date")]


def check_xlsx(path):
    with ZipFile(path) as archive:
        styles = ET.fromstring(archive.read("xl/styles.xml"))
        sheet = ET.fromstring(archive.read("xl/worksheets/sheet1.xml"))
    codes = {int(n.attrib["numFmtId"]): n.attrib["formatCode"]
             for n in styles.findall("s:numFmts/s:numFmt", NS)}
    xfs = styles.find("s:cellXfs", NS)
    cells = {cell.attrib["r"]: cell for cell in sheet.findall("s:sheetData/s:row/s:c", NS)}
    def number_format(address):
        cell = cells[address]
        fmt_id = int(xfs[int(cell.attrib["s"])].attrib["numFmtId"])
        return cell, fmt_id, codes.get(fmt_id, "")

    currency_ids = set()
    for address, symbol, value in CURRENCY_CELLS:
        cell, fmt_id, code = number_format(address)
        print(f"{address}: numFmtId={fmt_id}, formatCode={code}")
        if fmt_id < 164:
            raise AssertionError(f"{address}: currency uses reserved format ID {fmt_id}")
        if (symbol and f"[${symbol}]" not in code) or (symbol is None and "[$" in code):
            raise AssertionError(f"{address}: wrong currency symbol in {code!r}")
        if cell.findtext("s:v", namespaces=NS) != value:
            raise AssertionError(f"{address}: numeric value changed")
        currency_ids.add(fmt_id)

    for address, kind in CONTROL_CELLS:
        _, fmt_id, code = number_format(address)
        print(f"{address}: numFmtId={fmt_id}, formatCode={code} ({kind} control)")
        if fmt_id in currency_ids:
            raise AssertionError(f"{address}: {kind} shares format ID {fmt_id} with a currency")


def check_ods(path):
    with ZipFile(path) as archive:
        roots = [ET.fromstring(archive.read(name)) for name in ("content.xml", "styles.xml")
                 if name in archive.namelist()]
    styles = {node.attrib[f"{{{NS['style']}}}name"]: node
              for root in roots for node in root.iter() if f"{{{NS['style']}}}name" in node.attrib}
    rows = roots[0].findall("office:body/office:spreadsheet/table:table/table:table-row", NS)
    for index, symbol in enumerate(("€", "€", "$", "", "€"), start=1):
        cell = rows[index].findall("table:table-cell", NS)[1]
        style = styles[cell.attrib[f"{{{NS['table']}}}style-name"]]
        data_style = styles[style.attrib[f"{{{NS['style']}}}data-style-name"]]
        currency_symbols = data_style.findall("number:currency-symbol", NS)
        actual = "".join(node.text or "" for node in currency_symbols)
        print(f"ODS B{index + 1}: currency symbol={actual!r}")
        if actual != symbol:
            raise AssertionError(f"B{index + 1}: saved ODS lost its currency symbol")
        if not symbol and currency_symbols:
            raise AssertionError(f"B{index + 1}: hidden symbol became a locale-default currency")
        value = float(cell.attrib[f"{{{NS['office']}}}value"])
        if value != (-1234.56 if index == 2 else 1234.56):
            raise AssertionError(f"B{index + 1}: saved ODS changed its numeric value")
        if data_style.tag.endswith("percentage-style"):
            raise AssertionError(f"B{index + 1}: currency became a percentage")

    for index, expected_style in ((6, "percentage-style"), (7, "date-style")):
        cell = rows[index].findall("table:table-cell", NS)[1]
        style = styles[cell.attrib[f"{{{NS['table']}}}style-name"]]
        data_style = styles[style.attrib[f"{{{NS['style']}}}data-style-name"]]
        actual_style = data_style.tag.rsplit("}", 1)[-1]
        print(f"ODS B{index + 1}: data style={actual_style}")
        if actual_style != expected_style:
            raise AssertionError(
                f"B{index + 1}: expected a {expected_style} after the round trip, got {actual_style}")


def convert(x2t, source, destination, format_id):
    params = ET.Element("TaskQueueDataConvert")
    for name, value in [("m_sFileFrom", source), ("m_sFileTo", destination), ("m_nFormatTo", format_id)]:
        ET.SubElement(params, name).text = str(value)
    config = destination.with_suffix(".xml")
    ET.ElementTree(params).write(config, encoding="utf-8", xml_declaration=True)
    subprocess.run([str(x2t.resolve()), str(config)], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="Path for the reproduction .ods file")
    parser.add_argument("--x2t", type=Path, help="Also convert with this x2t executable and check styles")
    args = parser.parse_args()
    output = args.output.resolve()
    make_ods(output)
    print(f"Created {output}")
    if args.x2t:
        converted = output.with_suffix(".xlsx")
        convert(args.x2t, output, converted, 257)
        check_xlsx(converted)
        binary = output.with_suffix(".bin")
        roundtrip = output.with_name(output.stem + "-roundtrip.ods")
        convert(args.x2t, output, binary, 8194)
        convert(args.x2t, binary, roundtrip, 259)
        check_ods(roundtrip)


if __name__ == "__main__":
    main()
