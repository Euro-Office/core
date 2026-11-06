#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Dump an internal binary format file (DOCY/XLSY/PPTY, "Editor.bin") as a text tree.

The dump is meant for snapshot tests. It is deterministic and lossless: every
byte of the file shows up in it, with blobs over 32 bytes printed as length plus
SHA-256. So any change to what x2t writes changes the dump, and the diff says
which record changed.

Record framing (OOXML/Binary/Document/BinReader/BinaryReaderD.cpp):
  file        "DOCY;v<version>;<size>;" then base64 data, or raw data for v10
  main table  count:u8, count x (type:u8, offset:i32)
  table       length:i32 then Read1 records (the Signature table is bare Read2)
  Read1       type:u8 length:i32 content
  Read2       type:u8 lenType:u8 value   (lenType from c_oSerPropLenType)

Record contents are not self-describing. For DOCY and XLSY the walker follows
the record nesting that binformat_schema.py derives from the C++ readers, which
also gives the enum names used as labels. Where that schema has nothing, it
tries a Read1 sequence, then a Read2 sequence, then a UTF-16 string, then raw
bytes; a framing is only accepted if it consumes its span exactly.

PPTY uses another framing (OOXML/Binary/Presentation/BinaryFileReaderWriter.cpp):
  main table  count:u8, count x (type:u8, offset:i32), type from NSMainTables
  record      type:u8 length:i32 content
  content     optional attribute block 0xFA ... 0xFB, then child records
PPTY attributes are typed by the reader, not in the stream, so they are
printed raw and records are numbered only.

Usage: binformat_dump.py FILE [--root <core>] [--no-labels]
"""

import argparse
import base64
import functools
import hashlib
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from binformat_defines import find_enum, load_defines  # noqa: E402
from binformat_schema import build_schema  # noqa: E402

HEX_INLINE_MAX = 32  # longer leaves are printed as length + SHA-256
MAX_DEPTH = 64

# Read2 lenType -> (name, byte size; None = i32 length prefix). XLSY stores Double as a real.
LEN_TYPES = {
    "DOCY": {0: ("Null", 0), 1: ("Byte", 1), 2: ("Short", 2), 3: ("Three", 3), 4: ("Long", 4),
             5: ("Double", 4), 6: ("Variable", None), 7: ("Double64", 8), 8: ("Long64", 8)},
    "XLSY": {0: ("Null", 0), 1: ("Byte", 1), 2: ("Short", 2), 3: ("Three", 3), 4: ("Long", 4),
             5: ("Double", 8), 6: ("Variable", None)},
}
LEN_TYPES["VSDY"] = LEN_TYPES["XLSY"]

# Enum naming the tables in the main table.
TABLE_ENUM = {"DOCY": "c_oSerTableTypes", "XLSY": "c_oSerTableTypes",
              "VSDY": "c_oSerTableTypes", "PPTY": "MainTables"}
# Tables without a reader function in the schema: enum for their top-level records.
TABLE_RECORD_ENUM = {
    "DOCY": {"Signature": "c_oSerSigTypes"},
}


def u8(b, p):
    return b[p]


def i32(b, p):
    return struct.unpack_from("<i", b, p)[0]


def _plausible_char(c):
    o = ord(c)
    # Context-free guesses stick to scripts below U+0800 and general punctuation, so
    # binary data is not mistaken for CJK text. Strings the reader types as UTF-16
    # are decoded in full by scalar_text().
    return (c.isprintable() or c in "\t\n\r") and (o < 0x800 or 0x2000 <= o <= 0x20CF)


def utf16(data):
    if len(data) < 2 or len(data) % 2:
        return None
    try:
        s = data.decode("utf-16-le")
    except UnicodeDecodeError:
        return None
    return s if all(_plausible_char(c) for c in s) else None


def leaf_text(data):
    """Render untyped bytes: small ones as numbers or hex, long ones by size and hash."""
    n = len(data)
    if n == 0:
        return "(empty)"
    if n == 1:
        return f"u8 {data[0]}"
    if n == 4:
        return f"i32 {struct.unpack('<i', data)[0]}"
    s = utf16(data)
    if s is not None:
        return f"utf16 {s!r}"
    if n <= HEX_INLINE_MAX:
        return "hex " + data.hex(" ")
    return f"bytes len={n} sha256={hashlib.sha256(data).hexdigest()[:16]}"


def scalar_text(kind, data):
    """Render bytes the reader reads with a known getter; None if the size does not fit."""
    n = len(data)
    if kind == "u8" and n == 1:
        return f"u8 {data[0]}"
    if kind == "u16" and n == 2:
        return f"u16 {struct.unpack('<H', data)[0]}"
    if kind == "i32" and n == 4:
        return f"i32 {struct.unpack('<i', data)[0]}"
    if kind == "u32" and n == 4:
        return f"u32 {struct.unpack('<I', data)[0]}"
    if kind == "double100k" and n == 4:
        v = struct.unpack("<i", data)[0]
        return f"double {v / 100000:g} (i32 {v})"
    if kind == "f64" and n == 8:
        return f"f64 {struct.unpack('<d', data)[0]!r}"
    if kind == "utf16" and n % 2 == 0:
        try:
            return f"utf16 {data.decode('utf-16-le')!r}"
        except UnicodeDecodeError:
            return None
    if kind == "utf8":
        try:
            return f"utf8 {data.decode('utf-8')!r}"
        except UnicodeDecodeError:
            return None
    if kind == "string2" and n >= 4 and (n - 4) == 2 * struct.unpack_from("<i", data)[0]:
        try:
            return f"utf16 {data[4:].decode('utf-16-le')!r}"
        except UnicodeDecodeError:
            return None
    return None


class Walker:
    def __init__(self, fmt, buf, groups, schema, out):
        self.fmt = fmt
        self.b = buf
        self.groups = groups
        self.schema = schema
        self.out = out
        self.len_types = LEN_TYPES.get(fmt, LEN_TYPES["DOCY"])
        self._names = {}

    def emit(self, depth, text):
        self.out.append("  " * depth + text)

    def names(self, enum):
        """value -> member name for an enum (first declared name wins for aliases)."""
        if not enum or self.groups is None:
            return None
        if enum not in self._names:
            g = find_enum(self.groups, enum)
            self._names[enum] = {v: k for k, v in reversed(list(g.members.items()))} if g else None
        return self._names[enum]

    def ctx_info(self, ctx):
        if not ctx or not self.schema:
            return None, {}
        info = self.schema.functions.get(ctx)
        if not info:
            return None, {}
        return info["enum"], info["members"]

    # -- trial parses: list of (type, kind, start, end, lenTypeName) or None --
    def try_read1(self, start, end):
        recs, p = [], start
        while p < end:
            if p + 5 > end:
                return None
            t, n = u8(self.b, p), i32(self.b, p + 1)
            if n < 0 or p + 5 + n > end:
                return None
            recs.append((t, "r1", p + 5, p + 5 + n, None))
            p += 5 + n
        return recs or None

    def try_read2(self, start, end, guessing=True):
        recs, p = [], start
        while p < end:
            if p + 2 > end:
                return None
            t, lt = u8(self.b, p), u8(self.b, p + 1)
            if lt not in self.len_types:
                return None
            name, size = self.len_types[lt]
            q = p + 2
            if size is None:
                if q + 4 > end:
                    return None
                size = i32(self.b, q)
                q += 4
                if size < 0:
                    return None
            if q + size > end:
                return None
            recs.append((t, "r2", q, q + size, name))
            p = q + size
        # When guessing, a run of empty properties is far more likely a number or string.
        if not recs or (guessing and all(r[4] == "Null" for r in recs)):
            return None
        return recs

    def content(self, start, end, depth, ctx=None, mode=None, enum=None):
        """Dump [start, end). ctx: reader function for these records; mode: "r1"/"r2"."""
        if start >= end:
            return
        data = self.b[start:end]
        if depth > MAX_DEPTH:
            self.emit(depth, leaf_text(data))
            return
        recs = None
        if mode == "r1":
            recs = self.try_read1(start, end)
        elif mode == "r2":
            recs = self.try_read2(start, end, guessing=False)
        if recs is None:
            if mode:
                self.emit(depth, f"(does not parse as {mode} records)")
                ctx = None
            if utf16(data) is None:
                recs = self.try_read1(start, end) or self.try_read2(start, end)
        if not recs:
            if data[0] == 0xFA and ppty_fits(self, start, end):
                # Drawings and themes inside DOCY/XLSY are written with the PPTY writer.
                self.emit(depth, "(PPTY records)")
                ppty_records(self, start, end, depth)
            else:
                self.emit(depth, leaf_text(data))
            return
        ctx_enum, members = self.ctx_info(ctx)
        names = self.names(ctx_enum or enum)
        for t, kind, a, z, lt in recs:
            member = names.get(t) if names else None
            label = f"{member}({t})" if member else f"#{t}"
            child_kind, child_ctx = members.get(member, (None, None)) if member else (None, None)
            if kind == "r2":
                head = f"{label} {lt}"
                if lt != "Variable":
                    self.emit(depth, f"{head}: {self.r2_value(lt, a, z)}")
                    continue
                self.emit(depth, f"{head} len={z - a}")
            else:
                self.emit(depth, f"{label} len={z - a}")
            if child_kind in ("r1", "r2"):
                self.content(a, z, depth + 1, child_ctx, child_kind)
            elif child_kind and a < z and scalar_text(child_kind, self.b[a:z]) is not None:
                self.emit(depth + 1, scalar_text(child_kind, self.b[a:z]))
            else:
                self.content(a, z, depth + 1)

    def r2_value(self, lt, a, z):
        b = self.b
        if lt == "Null":
            return "-"
        if lt == "Byte":
            return str(b[a])
        if lt == "Short":
            return str(struct.unpack_from("<h", b, a)[0])
        if lt == "Three":
            return "hex " + b[a:z].hex(" ")
        if lt == "Long":
            return str(i32(b, a))
        if lt == "Double" and z - a == 4:
            v = i32(b, a)
            return f"{v / 100000:g} (i32 {v})"
        if lt in ("Double", "Double64"):
            return repr(struct.unpack_from("<d", b, a)[0])
        if lt == "Long64":
            return str(struct.unpack_from("<q", b, a)[0])
        return f"len={z - a}"


@functools.lru_cache(maxsize=None)
def _knowledge(core_root, sig):
    """Enum groups and reader schema for one format, parsed once per process."""
    return load_defines(core_root).get(sig), build_schema(core_root, sig)


def split_header(raw):
    """Return (signature, version, buffer, start offset, header bytes)."""
    parts = raw[:64].split(b";", 3)
    if len(parts) < 4 or not parts[1].startswith(b"v"):
        raise ValueError("not a DOCY/XLSY/PPTY/VSDY file (no 'SIG;vN;len;' header)")
    sig = parts[0].decode("ascii", "replace")
    version = int(parts[1][1:])
    header_len = len(parts[0]) + len(parts[1]) + len(parts[2]) + 3
    if version == 10:
        # g_nFormatVersionNoBase64: raw data, table offsets count from the start of the file.
        return sig, version, raw, header_len, raw[:header_len]
    return sig, version, base64.b64decode(raw[header_len:]), 0, raw[:header_len]


def dump(raw, core_root=None):
    sig, version, buf, start, header = split_header(raw)
    out = [f"header {header.decode('ascii', 'replace')}"]
    if version != 10:
        out.append(f"base64 payload: {len(buf)} bytes")
    groups = schema = None
    if core_root is not None:
        groups, schema = _knowledge(str(core_root), sig)
    w = Walker(sig, buf, groups, schema, out)
    table_names = w.names(TABLE_ENUM.get(sig))

    def tlabel(t):
        n = table_names.get(t) if table_names else None
        return f"{n}({t})" if n else f"#{t}"

    p = start
    tables = []
    if sig == "PPTY":
        # Up to 30 (type, offset) items, ended by type 0 (PPTXWriter.cpp OpenPPTY).
        while p < len(buf) and buf[p] != 0 and len(tables) < 30:
            tables.append((u8(buf, p), i32(buf, p + 1)))
            p += 5
    else:
        count = u8(buf, p)
        p += 1
        for _ in range(count):
            tables.append((u8(buf, p), i32(buf, p + 1)))
            p += 5
    out.append(f"main table: {len(tables)} tables")
    for t, off in tables:
        out.append(f"  {tlabel(t)} @{off}")

    # A table runs until the next table (by offset) begins.
    bounds = sorted({off for _, off in tables})
    first = bounds[0] if bounds else len(buf)
    if p < first:
        gap = buf[p:first]
        out.append(f"main table padding: {len(gap)} zero bytes" if not gap.strip(b"\0")
                   else f"main table padding: {leaf_text(gap)}")

    for t, off in sorted(tables, key=lambda x: x[1]):
        end = next((o for o in bounds if o > off), len(buf))
        tname = table_names.get(t) if table_names else None
        out.append(f"table {tlabel(t)} @{off} span={end - off}")
        if sig == "PPTY":
            ppty_table(w, off, end)
            continue
        ctx = schema.entry(sig, tname) if schema and tname else None
        enum = TABLE_RECORD_ENUM.get(sig, {}).get(tname)
        n = i32(buf, off) if off + 4 <= end else -1
        if 0 <= n <= end - off - 4:
            if n:
                w.content(off + 4, off + 4 + n, 1, ctx, "r1", enum)
            else:
                out.append("  (empty)")
            if off + 4 + n < end:
                out.append(f"  trailing {leaf_text(buf[off + 4 + n:end])}")
        else:
            # No length prefix: the Signature table is a bare Read2 sequence.
            out.append("  (no length prefix)")
            w.content(off, end, 1, ctx, None, enum)
    return "\n".join(out) + "\n"


def ppty_table(w, start, end):
    """A PPTY table is one record, or an i32 count followed by records."""
    ppty_records(w, start, end, 1, allow_attrs=False)


def ppty_attr_end(w, start, end):
    """Index of the 0xFB closing the attribute block at start, or -1."""
    b = w.b
    q = b.find(b"\xfb", start + 1, end)
    while q >= 0 and not (q + 1 == end or ppty_parse(w, q + 1, end)):
        q = b.find(b"\xfb", q + 1, end)
    return q


def ppty_fits(w, start, end):
    return w.b[start] == 0xFA and ppty_attr_end(w, start, end) >= 0


def ppty_records(w, start, end, depth, allow_attrs=True):
    """[attribute block] then records, each list optionally preceded by an i32 count."""
    b = w.b
    if depth > MAX_DEPTH:
        w.emit(depth, leaf_text(b[start:end]))
        return
    p = start
    if allow_attrs and p < end and b[p] == 0xFA:
        # Attribute block: the first 0xFB after which the rest still parses ends it.
        q = ppty_attr_end(w, p, end)
        if q >= 0:
            w.emit(depth, f"attrs {attr_text(b[p + 1:q])}")
            p = q + 1
    if p >= end:
        return
    parsed = ppty_parse(w, p, end)
    if not parsed:
        w.emit(depth, leaf_text(b[p:end]))
        return
    count, recs = parsed
    if count is not None:
        w.emit(depth, f"count {count}")
    for t, _, a, z, _ in recs:
        w.emit(depth, f"#{t} len={z - a}")
        ppty_records(w, a, z, depth + 1)


def ppty_parse(w, p, end):
    """(count or None, records) if [p, end) is a record list, optionally counted."""
    recs = w.try_read1(p, end)
    if recs:
        return None, recs
    if p + 4 <= end:
        n = i32(w.b, p)
        if n == 0 and p + 4 == end:
            return 0, []
        recs = w.try_read1(p + 4, end) if n > 0 else None
        if recs and len(recs) == n:
            return n, recs
    return None


def attr_text(data):
    """PPTY attributes are (index:u8, value) with the value type known only to the reader.

    String values (index, i32 char count, UTF-16) are recognised and printed as
    [index]'text'; everything else stays hex, so the rendering remains lossless.
    """
    if not data:
        return "(empty)"
    parts, raw, i, n = [], bytearray(), 0, len(data)

    def flush():
        if raw:
            parts.append("hex " + raw.hex(" ") if len(raw) <= 2 * HEX_INLINE_MAX else
                         f"bytes len={len(raw)} sha256={hashlib.sha256(bytes(raw)).hexdigest()[:16]}")
            raw.clear()

    while i < n:
        if i + 5 <= n:
            count = struct.unpack_from("<i", data, i + 1)[0]
            if 1 < count and i + 5 + 2 * count <= n:
                text = utf16(data[i + 5:i + 5 + 2 * count])
                if text is not None:
                    flush()
                    parts.append(f"[{data[i]}]{text!r}")
                    i += 5 + 2 * count
                    continue
        raw.append(data[i])
        i += 1
    flush()
    return " ".join(parts)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file", type=Path)
    ap.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2],
                    help="core checkout to read enum names and reader code from (default: this repository)")
    ap.add_argument("--no-labels", action="store_true", help="numeric record types and heuristics only")
    args = ap.parse_args()
    sys.stdout.write(dump(args.file.read_bytes(), None if args.no_labels else args.root))


if __name__ == "__main__":
    main()
