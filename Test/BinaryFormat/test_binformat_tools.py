# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Self-tests for the binary format tools. Run: python3 -m unittest discover -s Test/BinaryFormat"""

import base64
import contextlib
import io
import random
import struct
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))

import check_binformat_enums as check  # noqa: E402
from binformat_defines import find_enum, parse_defines  # noqa: E402
from binformat_dump import dump  # noqa: E402

HEADER = """
namespace BinDocxRW
{
const int g_nFormatVersion = 5;
const int align_Left = 0x01; // comment = 9
#define COLOR_TYPE_SRGB 1
#define COLOR_ALIAS COLOR_TYPE_SRGB
	namespace c_oSerTableTypes{enum c_oSerTableTypes
	{
		Signature = 0,
		Info,            /* implicit 1 */
		Media = 0x10,
		Neg = -2,
		Shifted = (1 << 3) + Info,
	};}
	namespace c_oOuter {enum c_oInner
	{
		A = 7
	};}
}
"""


class ParseDefines(unittest.TestCase):
    def setUp(self):
        self.groups = parse_defines(HEADER)

    def test_enum_values(self):
        g = self.groups["BinDocxRW::c_oSerTableTypes"]
        self.assertEqual(g.members, {"Signature": 0, "Info": 1, "Media": 16, "Neg": -2, "Shifted": 9})

    def test_consts_and_defines(self):
        self.assertEqual(self.groups["BinDocxRW::const"].members, {"g_nFormatVersion": 5, "align_Left": 1})
        self.assertEqual(self.groups["#define"].members, {"COLOR_TYPE_SRGB": 1, "COLOR_ALIAS": 1})

    def test_lookup_by_namespace_qualifier(self):
        # Code writes c_oOuter::A, so the namespace names the enum.
        self.assertEqual(find_enum(self.groups, "c_oOuter").members, {"A": 7})
        self.assertEqual(find_enum(self.groups, "c_oSerTableTypes").members["Media"], 16)

    def test_real_headers_parse(self):
        from binformat_defines import load_defines
        for fmt, groups in load_defines(ROOT).items():
            self.assertTrue(groups, fmt)
        docy = load_defines(ROOT)["DOCY"]
        self.assertEqual(find_enum(docy, "c_oSerTableTypes").members["Document"], 6)


class CompareRules(unittest.TestCase):
    def run_compare(self, head_text):
        report = check.Report()
        with contextlib.redirect_stdout(io.StringIO()) as out:
            check.compare("DOCY", "h", parse_defines(HEADER), parse_defines(head_text), report)
        return report.errors, out.getvalue()

    def test_unchanged_and_appended_is_ok(self):
        errors, _ = self.run_compare(HEADER.replace("Shifted = (1 << 3) + Info,", "Shifted = 9,\n\t\tNewOne = 17,"))
        self.assertEqual(errors, 0)

    def test_changed_value(self):
        errors, out = self.run_compare(HEADER.replace("Media = 0x10", "Media = 0x11"))
        self.assertEqual(errors, 1)
        self.assertIn("Media changed value 16 -> 17", out)

    def test_removed_member(self):
        errors, out = self.run_compare(HEADER.replace("Neg = -2,", ""))
        self.assertEqual(errors, 1)
        self.assertIn("Neg (= -2) was removed", out)

    def test_implicit_renumbering_is_caught(self):
        # Inserting a member before an implicit one shifts it.
        errors, out = self.run_compare(HEADER.replace("Signature = 0,", "Signature = 0,\n\t\tInserted,"))
        self.assertGreaterEqual(errors, 1)
        self.assertIn("Info changed value 1 -> 2", out)

    def test_new_member_reusing_a_value(self):
        errors, out = self.run_compare(HEADER.replace("Neg = -2,", "Neg = -2,\n\t\tClash = 16,"))
        self.assertEqual(errors, 1)
        self.assertIn("Clash (new) reuses value 16 of Media", out)

    def test_version_bump_is_a_notice(self):
        errors, out = self.run_compare(HEADER.replace("g_nFormatVersion = 5", "g_nFormatVersion = 6"))
        self.assertEqual(errors, 0)
        self.assertIn("bumped 5 -> 6", out)
        errors, _ = self.run_compare(HEADER.replace("g_nFormatVersion = 5", "g_nFormatVersion = 4"))
        self.assertEqual(errors, 1)


def docy(version, tables):
    """Build a DOCY file: tables is [(type, payload bytes)] (payload without length prefix)."""
    body = bytearray()
    entries = []
    for t, payload in tables:
        entries.append((t, len(body)))
        body += payload
    head_len = 1 + 5 * len(tables)
    payload = bytearray([len(tables)])
    if version == 10:
        prefix = b"DOCY;v10;0;"
        base = len(prefix) + head_len
    else:
        base = head_len
    for t, off in entries:
        payload += bytes([t]) + struct.pack("<i", base + off)
    payload += body
    if version == 10:
        return prefix + bytes(payload)
    return f"DOCY;v{version};{len(payload)};".encode() + base64.b64encode(bytes(payload))


def read1(t, content):
    return bytes([t]) + struct.pack("<i", len(content)) + content


def table(content):
    return struct.pack("<i", len(content)) + content


class Dump(unittest.TestCase):
    def test_base64_and_raw_containers_dump_the_same_tree(self):
        sig = bytes([0, 4]) + struct.pack("<i", 5)  # Signature table: bare Read2 Version=5
        doc = table(read1(0, read1(1, "Hi".encode("utf-16-le"))))
        dumps = []
        for version in (5, 10):
            text = dump(docy(version, [(0, sig), (6, doc)]), ROOT)
            # Drop the header lines and table offsets, which differ between the containers.
            dumps.append([line.split(" @")[0] for line in text.splitlines()[1:] if not line.startswith("base64")])
        self.assertEqual(dumps[0], dumps[1])
        self.assertIn("  Version(0) Long: 5", dumps[1])
        self.assertTrue(any("Document(6)" in line for line in dumps[1]))

    def test_every_bit_flip_changes_the_dump(self):
        rnd = random.Random(7)
        for name in ("docx", "xlsx", "pptx"):
            raw = (ROOT / "Common/empty" / f"{name}.bin").read_bytes()
            base = dump(raw, ROOT)
            for _ in range(40):
                i = rnd.randrange(11, len(raw))
                mutated = bytearray(raw)
                mutated[i] ^= 1 << rnd.randrange(8)
                self.assertNotEqual(dump(bytes(mutated), ROOT), base, f"{name}: flip at byte {i} not visible")


class SnapshotBundle(unittest.TestCase):
    def test_printed_snapshots_import_from_a_timestamped_log(self):
        import tempfile
        from unittest import mock
        import run_snapshots
        texts = {"x2t/word_proc/a.docx.txt": "header DOCY;v10;0;\n  ä\n", "x2t/sheets/b.xlsx.txt": "x\n"}
        with tempfile.TemporaryDirectory() as d:
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                run_snapshots.print_bundle(texts)
            log = Path(d) / "job.log"
            log.write_text("".join(f"2026-10-27T23:46:43.1832561Z {l}\n" for l in out.getvalue().splitlines()))
            with mock.patch.object(run_snapshots, "SNAPSHOTS", Path(d) / "snapshots"), \
                    contextlib.redirect_stdout(io.StringIO()):
                run_snapshots.import_bundle(log)
            for rel, text in texts.items():
                self.assertEqual((Path(d) / "snapshots" / rel).read_text(encoding="utf-8"), text)


if __name__ == "__main__":
    unittest.main()
