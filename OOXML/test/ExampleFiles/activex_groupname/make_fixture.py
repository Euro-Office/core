#!/usr/bin/env python3
"""Builds groupname.docx: two option buttons that share an ActiveX group name.

Word is not available where this fixture was written, so the ActiveX parts are
packed by hand. Each control is an MS-OFORMS 2.2.5 MorphData preceded by the
two header bytes that OOX::ActiveX_xml::read_bin skips before handing the
buffer to OOX::ActiveXObjectMorphData::Parse.

MorphDataPropMask is a 64-bit mask; fSize is bit 8, fCaption bit 23 and
fGroupName bit 32. Both controls write the group name compressed
(0x80000006 followed by the ASCII of "Group1"), which is what Word does and
what the reader must consume.

Layout of one MorphData (offsets for the first control):

   0  MinorVersion                      0
   1  MajorVersion                      0
   2  cbMorphData                       64
   4  PropMask                          0x0000000100800100
  12  fCaption CountOfChars+Compression 0x80000008 (8)
  16  fGroupName CountOfChars+Compress. 0x80000006 (6)
  20  fSize.cx                         120
  24  fSize.cy                          45
  28  fCaption characters               "Option A", padded to 32
  32  fGroupName characters             "Group1", padded to 40
  44  TextProps.MinorVersionText        0
  45  TextProps.MajorVersionText        0
  46  TextProps.cbTextProps             20
  48  TextProps.PropMask                1 (fFontName)
  52  fFontName CountOfChars+Compress.  0x80000005 (5)
  56  fFontName characters              "Arial", padded to 64

Run it from anywhere: python3 make_fixture.py (writes groupname.docx next to it).
"""

import struct
import zipfile
from pathlib import Path

# MS-OFORMS 2.2.5.1 MorphDataPropMask
F_SIZE = 1 << 8
F_CAPTION = 1 << 23
F_GROUP_NAME = 1 << 32

GROUP_NAME = b"Group1"
CLASS_ID = "{8BD21D50-EC42-11CE-9E0D-00AA006002F3}"  # OptionButton


def count_and_compression(text: bytes) -> int:
    """CountOfCharsWithCompressionFlag: bit 31 set, one byte per character."""
    return 0x80000000 | len(text)


def pad4(out: bytearray) -> None:
    while len(out) % 4:
        out.append(0)


def morph_data(caption: bytes, font: bytes, size: tuple[int, int]) -> bytes:
    out = bytearray()
    out += struct.pack("<BBH", 0, 0, 0)  # MinorVersion, MajorVersion, cbMorphData
    out += struct.pack("<Q", F_SIZE | F_CAPTION | F_GROUP_NAME)
    # DataBlock: the string sizes, in mask order (fCaption, then fGroupName).
    out += struct.pack("<I", count_and_compression(caption))
    out += struct.pack("<I", count_and_compression(GROUP_NAME))
    # ExtraDataBlock: fSize, then the characters in DataBlock order.
    out += struct.pack("<II", *size)
    out += caption
    pad4(out)
    out += GROUP_NAME
    pad4(out)
    # StreamData: TextProps (MS-OFORMS 2.2.5.4) with only fFontName.
    out += struct.pack("<BBHI", 0, 0, 20, 1)
    out += struct.pack("<I", count_and_compression(font))
    out += font
    pad4(out)
    # cbMorphData covers the whole structure.
    struct.pack_into("<H", out, 2, len(out))
    return bytes(out)


def activex_bin(*args) -> bytes:
    # Two header bytes, skipped by ActiveX_xml::read_bin, then the MorphData.
    return b"\x00\x00" + morph_data(*args)


REL_NS = "http://schemas.openxmlformats.org/package/2006/relationships"
AX_REL = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/control"
BIN_REL = "http://schemas.microsoft.com/office/2006/relationships/activeXControlBinary"
AX_NS = "http://schemas.microsoft.com/office/2006/activeX"
W_NS = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
V_NS = "urn:schemas-microsoft-com:vml"
R_NS = "http://schemas.openxmlformats.org/officeDocument/2006/relationships"


def pict(rel_id: str, name: str, shape_id: str, style: str) -> str:
    return (
        "<w:r><w:pict>"
        f'<v:shape id="{shape_id}" style="{style}">'
        f'<w:control r:id="{rel_id}" w:name="{name}" w:shapeid="{shape_id}"/>'
        "</v:shape></w:pict></w:r>"
    )


def build() -> dict[str, bytes]:
    controls = [
        dict(rel="rId7", name="OptionButton1", shape="_x0000_s1025",
             style="width:120pt;height:45pt", caption=b"Option A",
             font=b"Arial", size=(120, 45)),
        dict(rel="rId8", name="OptionButton2", shape="_x0000_s1026",
             style="width:130pt;height:50pt", caption=b"Option B",
             font=b"Courier", size=(130, 50)),
    ]

    content_types = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="xml" ContentType="application/xml"/>'
        '<Default Extension="bin" ContentType="application/vnd.ms-office.activeX"/>'
        '<Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>'
        '<Override PartName="/word/activeX/activeX1.xml" ContentType="application/vnd.ms-office.activeX+xml"/>'
        '<Override PartName="/word/activeX/activeX2.xml" ContentType="application/vnd.ms-office.activeX+xml"/>'
        "</Types>"
    )
    root_rels = (
        f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="{REL_NS}">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>'
        "</Relationships>"
    )
    document = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        f'<w:document xmlns:w="{W_NS}" xmlns:v="{V_NS}" xmlns:r="{R_NS}"><w:body>'
        "<w:p>"
        + pict("rId7", "OptionButton1", "_x0000_s1025", "width:120pt;height:45pt")
        + pict("rId8", "OptionButton2", "_x0000_s1026", "width:130pt;height:50pt")
        + "</w:p><w:sectPr/></w:body></w:document>"
    )
    document_rels = (
        f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?><Relationships xmlns="{REL_NS}">'
        f'<Relationship Id="rId7" Type="{AX_REL}" Target="activeX/activeX1.xml"/>'
        f'<Relationship Id="rId8" Type="{AX_REL}" Target="activeX/activeX2.xml"/>'
        "</Relationships>"
    )

    parts: dict[str, bytes] = {
        "[Content_Types].xml": content_types.encode(),
        "_rels/.rels": root_rels.encode(),
        "word/document.xml": document.encode(),
        "word/_rels/document.xml.rels": document_rels.encode(),
    }
    for index, control in enumerate(controls, start=1):
        for path, data in (
            (f"word/activeX/activeX{index}.xml", (
                '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                f'<ax:ocx xmlns:ax="{AX_NS}" xmlns:r="{R_NS}" '
                f'ax:classid="{CLASS_ID}" ax:persistence="persistStorage" r:id="rId1"/>'
            ).encode()),
            (f"word/activeX/_rels/activeX{index}.xml.rels", (
                f'<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
                f'<Relationships xmlns="{REL_NS}">'
                f'<Relationship Id="rId1" Type="{BIN_REL}" Target="activeX{index}.bin"/>'
                "</Relationships>"
            ).encode()),
            (f"word/activeX/activeX{index}.bin", activex_bin(
                control["caption"], control["font"], control["size"])),
        ):
            parts[path] = data
    return parts


def main() -> None:
    target = Path(__file__).with_name("groupname.docx")
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as package:
        for name, data in build().items():
            package.writestr(name, data)
    print(f"wrote {target}")


if __name__ == "__main__":
    main()
