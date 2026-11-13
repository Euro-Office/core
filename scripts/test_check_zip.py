# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
import struct
import unittest
import warnings
import zipfile
from io import BytesIO

import check_zip

ODT = b"application/vnd.oasis.opendocument.text"


class Unseekable(BytesIO):
    """zipfile writes data descriptors (flag bit 3) to a stream it cannot seek in, like minizip-ng's default."""

    def tell(self):
        raise OSError("not seekable")  # zipfile then falls back to data descriptors


def build(entries, seekable=True):
    """entries: (name, content, compression[, extra])."""
    out = BytesIO() if seekable else Unseekable()
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")  # duplicate names
        with zipfile.ZipFile(out, "w") as archive:
            for name, content, compression, *extra in entries:
                info = zipfile.ZipInfo(name)
                info.compress_type = compression
                if extra:
                    info.extra = extra[0]
                archive.writestr(info, content)
    return out.getvalue()


def odt(**changes):
    entries = [("mimetype", ODT, zipfile.ZIP_STORED), ("content.xml", b"<x/>", zipfile.ZIP_DEFLATED)]
    return build(changes.get("entries", entries), changes.get("seekable", True))


def docx(extra=()):
    return build(
        [
            ("[Content_Types].xml", b"<Types/>", zipfile.ZIP_STORED),
            ("_rels/.rels", b"<Relationships/>", zipfile.ZIP_STORED),
            ("word/document.xml", b"<d/>" * 100, zipfile.ZIP_DEFLATED),
            *extra,
        ]
    )


class CheckZipTest(unittest.TestCase):
    def test_valid_odt_and_docx_pass(self):
        self.assertEqual(check_zip.problems(odt(), "odt"), [])
        self.assertEqual(check_zip.problems(docx(), "docx"), [])

    def test_stored_entry_with_data_descriptor_is_rejected(self):
        # The saved-documents regression: minizip-ng wrote this for STORED entries.
        result = check_zip.problems(odt(seekable=False), "odt")
        self.assertTrue(any("STORED with a data descriptor" in p for p in result), result)

    def test_deflated_entry_with_valid_data_descriptor_passes(self):
        data = build([("[Content_Types].xml", b"<a/>" * 50, zipfile.ZIP_DEFLATED), ("_rels/.rels", b"<r/>", zipfile.ZIP_DEFLATED)], seekable=False)
        self.assertEqual(check_zip.problems(data, "docx"), [])

    def test_local_header_crc_mismatch_is_rejected(self):
        data = bytearray(docx())
        data[14:18] = struct.pack("<I", 1)  # local CRC of the first entry
        result = check_zip.problems(bytes(data), "docx")
        self.assertTrue(any("differ from central header" in p for p in result), result)

    def test_corrupt_data_is_rejected(self):
        data = bytearray(odt())
        data[30 + len("mimetype")] ^= 0xFF  # first byte of the mimetype content
        result = check_zip.problems(bytes(data), "odt")
        self.assertTrue(any("CRC" in p or "mimetype content" in p for p in result), result)

    def test_odf_mimetype_must_be_first_stored_without_extra_field(self):
        content = ("content.xml", b"<x/>", zipfile.ZIP_DEFLATED)
        mimetype = ("mimetype", ODT, zipfile.ZIP_STORED)
        self.assertTrue(any("first entry" in p for p in check_zip.problems(odt(entries=[content, mimetype]), "odt")))
        deflated = ("mimetype", ODT * 5, zipfile.ZIP_DEFLATED)
        self.assertTrue(any("must be STORED" in p for p in check_zip.problems(odt(entries=[deflated, content]), "odt")))
        extra = ("mimetype", ODT, zipfile.ZIP_STORED, b"\x01\x02\x00\x00")
        self.assertTrue(any("extra field" in p for p in check_zip.problems(odt(entries=[extra, content]), "odt")))

    def test_odf_mimetype_content_must_match_the_extension(self):
        self.assertTrue(any("does not match" in p for p in check_zip.problems(odt(), "ods")))

    def test_ooxml_needs_content_types_and_rels(self):
        data = build([("word/document.xml", b"<d/>", zipfile.ZIP_DEFLATED)])
        result = check_zip.problems(data, "docx")
        self.assertIn("missing [Content_Types].xml", result)
        self.assertIn("missing _rels/.rels", result)

    def test_duplicate_and_unsafe_names_are_rejected(self):
        result = check_zip.problems(docx([("Word/Document.xml", b"<d/>", zipfile.ZIP_DEFLATED), ("word/document.xml", b"<d/>", zipfile.ZIP_DEFLATED), ("..\\x", b"", zipfile.ZIP_DEFLATED)]), "docx")
        self.assertTrue(any("duplicate name" in p for p in result), result)
        self.assertTrue(any("unsafe name" in p for p in result), result)

    def test_zip64_looking_archive_still_gets_the_zipfile_checks(self):
        data = bytearray(docx())
        eocd = data.rfind(struct.pack("<I", check_zip.EOCD_SIG))
        data[eocd + 16 : eocd + 20] = b"\xff\xff\xff\xff"  # central directory offset: zip64 marker
        result = check_zip.problems(bytes(data), "docx")
        self.assertTrue(result and all("zipfile cannot read" in p for p in result), result)

    def test_garbage_is_reported_not_raised(self):
        self.assertEqual(check_zip.problems(b"not a zip", "docx")[0], "unreadable central directory: no end of central directory record")


if __name__ == "__main__":
    unittest.main()
