#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Derive the DOCY/XLSY record nesting from the C++ readers.

The binary format has no schema file; its structure lives in the reader
functions of BinaryReaderD.cpp / BinaryReaderS.cpp, which all look like

    int Binary_SettingsTableReader::ReadSettings(BYTE type, long length, void* poResult)
    {
        if (c_oSer_SettingsType::MathPr == type)
            READ1_DEF(length, res, this->ReadMathPr, ...);        // nested Read1 records
        else if (c_oSer_SettingsType::DefaultTabStop == type)
            ... m_oBufferedStream.GetDouble() ...                  // a scalar
    }

This module scans those functions (regex based, no C++ parser) and records,
per function, which enum it switches on and, per enum member, whether the
content is nested Read1/Read2 records (and which function reads them) or a
scalar of a known type. binformat_dump.py uses it to label records and to pick
the right framing; anything not recognised here falls back to its heuristics,
so the extraction only needs to be good, not complete.
"""

import re
from pathlib import Path

READER_SOURCES = {
    "DOCY": ["OOXML/Binary/Document/BinReader/BinaryReaderD.cpp"],
    "XLSY": ["OOXML/Binary/Sheets/Writer/BinaryReaderS.cpp"],
}
# Headers declaring the reader objects used as obj.ReadX(...) in the sources.
READER_HEADERS = {
    "DOCY": ["OOXML/Binary/Document/BinReader/BinaryReaderD.h"],
    "XLSY": ["OOXML/Binary/Sheets/Writer/BinaryReaderS.h", "OOXML/Binary/Sheets/Writer/BinaryCommonReader.h"],
}

# The function that reads each table's records: (format, table name) -> function.
TABLE_ENTRY = {
    "DOCY": {
        "HdrFtr": "Binary_HdrFtrTableReader::ReadHdrFtrContent",
        "Numbering": "Binary_NumberingTableReader::ReadNumberingContent",
        "Style": "BinaryStyleTableReader::ReadStyleTableContent",
        "Other": "Binary_OtherTableReader::ReadOtherContent",
        "Customs": "Binary_CustomsTableReader::ReadCustom",
        "Comments": "Binary_CommentsTableReader::ReadComments",
        "DocumentComments": "Binary_CommentsTableReader::ReadComments",
        "Settings": "Binary_SettingsTableReader::ReadSettings",
        "Document": "Binary_DocumentTableReader::ReadDocumentContent",
        "Footnotes": "Binary_NotesTableReader::ReadNotes",
        "Endnotes": "Binary_NotesTableReader::ReadNotes",
    },
    "XLSY": {
        "SharedStrings": "BinarySharedStringTableReader::ReadSharedStringTableContent",
        "Styles": "BinaryStyleTableReader::ReadStyleTableContent",
        "Workbook": "BinaryWorkbookTableReader::ReadWorkbookTableContent",
        "Worksheets": "BinaryWorksheetsTableReader::ReadWorksheetsTableContent",
        "Other": "BinaryOtherTableReader::ReadOtherTableContent",
        "PersonList": "BinaryPersonReader::ReadPersonList",
        "Customs": "BinaryCustomsReader::ReadCustom",
    },
}

# Scalar getters on the stream and how the dump should render their bytes.
GETTERS = {
    "GetDouble": "double100k",  # i32 holding value * 100000
    "GetDoubleReal": "f64",
    "GetLong": "i32",
    "GetULong": "u32",
    "GetUShort": "u16",
    "GetChar": "u8",
    "GetUChar": "u8",
    "GetBool": "u8",
    "GetString3": "utf16",
    "GetString4": "utf16",
    "GetString": "utf16",
    "GetString2": "string2",  # i32 char count + UTF-16
    "GetStringUtf8": "utf8",
}

_FUNC = re.compile(r"^[ \t]*(?:int|long)\s+(\w+)::(\w+)\s*\(([^)]*)\)\s*(?:const\s*)?\{", re.M)
_CMP = re.compile(r"\b(\w+)::(\w+)\s*==\s*type\b|\btype\s*==\s*(\w+)::(\w+)|\bcase\s+(\w+)::(\w+)\s*:")
# XLSY worksheets are read in two passes: record positions first (ReadWorksheetSeekPositions),
# then SEEK_TO_POS_START(type) -> mapPos.find(type) per record type in ReadWorksheet.
_SEEK = re.compile(r"\bmapPos\s*\.\s*find\s*\(\s*(\w+)::(\w+)\s*\)")
_READ = re.compile(r"\bREAD([12])_DEF(?:_SPREADSHEET)?\s*\(\s*[^,]+,\s*\w+\s*,\s*([^,]+?)\s*,")
_CALL = re.compile(r"\b(\w+)\s*(?:\.|->)\s*(Read\w*)\s*\(\s*(?:length|stLen|nLength)\b")
_THISCALL = re.compile(r"(?<![\.\w>])(?:this->)?(Read\w*)\s*\(\s*(?:length|stLen)\b")
_GETTER = re.compile(r"\bm_oBufferedStream\s*\.\s*(" + "|".join(GETTERS) + r")\s*\(")
_DECL = re.compile(r"\b(Binary\w*Reader\w*|\w+TableReader)\s*[&*]?\s+(\w+)\s*[;(=]")


def _strip(text):
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"', blank, text, flags=re.S)


_DEFINE = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)(\(([^)]*)\))?((?:[^\n]*\\\n)*[^\n]*)", re.M)
# Framing macros are matched as such and must not be expanded.
_KEEP = {"READ1_DEF", "READ2_DEF", "READ2_DEF_SPREADSHEET", "READ_TABLE_DEF"}


def _macros(src):
    """{name: (params or None, body)} for the #defines in a source file."""
    out = {}
    for m in _DEFINE.finditer(src):
        if m.group(1) in _KEEP:
            continue
        params = [x.strip() for x in m.group(3).split(",")] if m.group(2) else None
        out[m.group(1)] = (params, m.group(4).replace("\\\n", "\n"))
    return out


def _args(text, open_paren):
    """Split the call arguments starting at text[open_paren] == '('; return (args, end)."""
    depth, args, cur = 0, [], open_paren + 1
    for i in range(open_paren, len(text)):
        c = text[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                args.append(text[cur:i])
                return args, i + 1
        elif c == "," and depth == 1:
            args.append(text[cur:i])
            cur = i + 1
    return None, open_paren


def _expand(body, macros, rounds=4):
    if not macros:
        return body
    names = re.compile(r"\b(" + "|".join(map(re.escape, macros)) + r")\b")
    for _ in range(rounds):
        out, pos, changed = [], 0, False
        for m in names.finditer(body):
            if m.start() < pos:
                continue
            params, text = macros[m.group(1)]
            end = m.end()
            if params is not None:
                k = end
                while k < len(body) and body[k] in " \t":
                    k += 1
                if k >= len(body) or body[k] != "(":
                    continue
                args, end = _args(body, k)
                if args is None:
                    continue
                for prm, arg in zip(params, args):
                    if prm:
                        text = re.sub(r"\b%s\b" % re.escape(prm), lambda _m, a=arg: a.strip(), text)
            out.append(body[pos:m.start()])
            out.append(" " + text + " ")
            pos = end
            changed = True
        out.append(body[pos:])
        body = "".join(out)
        if not changed:
            break
    return body


def _body(src, open_brace):
    depth = 0
    for i in range(open_brace, len(src)):
        c = src[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return src[open_brace + 1:i]
    return src[open_brace + 1:]


class Schema:
    """functions: {"Class::Func": {"enum": str|None, "members": {member: (kind, target)}}}

    kind is "r1"/"r2" (target = function reading the nested records) or a
    scalar kind from GETTERS (target None).
    """

    def __init__(self):
        self.functions = {}
        self.wrappers = {}  # "Class::Read" -> ("r1"/"r2", "Class::Func") for non-type readers

    def entry(self, fmt, table):
        return TABLE_ENTRY.get(fmt, {}).get(table)


def build_schema(core_root, fmt):
    schema = Schema()
    root = Path(core_root)
    # Member/local reader objects: variable name -> reader class (format-wide approximation).
    var_class = {}
    seek_members = {}
    for rel in READER_HEADERS.get(fmt, []) + READER_SOURCES.get(fmt, []):
        path = root / rel
        if path.exists():
            for m in _DECL.finditer(_strip(path.read_text(encoding="utf-8", errors="replace"))):
                var_class.setdefault(m.group(2), m.group(1))
    for rel in READER_SOURCES.get(fmt, []):
        path = root / rel
        if not path.exists():
            continue
        src = _strip(path.read_text(encoding="utf-8", errors="replace"))
        macros = _macros(src)
        for fm in _FUNC.finditer(src):
            cls, fn, params = fm.group(1), fm.group(2), fm.group(3)
            body = _expand(_body(src, fm.end() - 1), macros)
            qname = f"{cls}::{fn}"
            if re.match(r"\s*BYTE\s+type\s*,", params):
                schema.functions[qname] = _scan_type_function(cls, body, var_class)
            elif "mapPos" in params and _SEEK.search(body):
                info = _scan_type_function(cls, body, var_class, seek=True)
                seek = seek_members.setdefault(cls, {"enum": info["enum"], "members": {}})
                for member, val in info["members"].items():
                    seek["members"].setdefault(member, val)
            else:
                reads = list(_READ.finditer(body))
                if len(reads) == 1:
                    schema.wrappers[qname] = (f"r{reads[0].group(1)}", _resolve(cls, reads[0].group(2), var_class))
    for qname, info in schema.functions.items():
        cls = qname.split("::")[0]
        if qname.endswith("SeekPositions") and cls in seek_members:
            schema.functions[qname] = seek_members[cls]
    # Resolve calls through wrapper functions (obj.Read(length, ...) -> its READn_DEF target).
    for info in schema.functions.values():
        for member, (kind, target) in list(info["members"].items()):
            if kind == "call":
                if target in schema.wrappers:
                    info["members"][member] = schema.wrappers[target]
                else:
                    del info["members"][member]
    return schema


def _resolve(cls, ref, var_class):
    ref = ref.strip().lstrip("&")
    m = re.match(r"(?:this\s*->\s*)?(\w+)$", ref)
    if m:
        return f"{cls}::{m.group(1)}"
    m = re.match(r"(\w+)\s*(?:\.|->)\s*(\w+)$", ref)
    if m:
        return f"{var_class.get(m.group(1), m.group(1))}::{m.group(2)}"
    m = re.match(r"(\w+)::(\w+)$", ref)
    if m:
        return f"{m.group(1)}::{m.group(2)}"
    return ref


def _scan_type_function(cls, body, var_class, seek=False):
    """Attribute READn_DEF calls and getters to the enum members compared before them."""
    events = []
    switches = []
    for sm in re.finditer(r"\bswitch\s*\(\s*type\s*\)\s*\{", body):
        switches.append((sm.end(), sm.end() + len(_body(body, sm.end() - 1))))
    for m in (_SEEK if seek else _CMP).finditer(body):
        if not seek and m.group(5) and not any(a <= m.start() < z for a, z in switches):
            continue  # a case label of some other switch
        g = [x for x in m.groups() if x]
        events.append((m.start(), m.end(), "cmp", (g[0], g[1])))
    for m in _READ.finditer(body):
        events.append((m.start(), m.end(), "read", (f"r{m.group(1)}", _resolve(cls, m.group(2), var_class))))
    for m in _CALL.finditer(body):
        target = f"{var_class.get(m.group(1), m.group(1))}::{m.group(2)}"
        events.append((m.start(), m.end(), "call", ("call", target)))
    for m in _THISCALL.finditer(body):
        events.append((m.start(), m.end(), "call", ("call", f"{cls}::{m.group(1)}")))
    for m in _GETTER.finditer(body):
        events.append((m.start(), m.end(), "get", (GETTERS[m.group(1)], None)))
    events.sort()

    enum_votes = {}
    members = {}
    current = []
    last_cmp_end = None
    for start, end, kind, val in events:
        if kind == "cmp":
            enum, member = val
            enum_votes[enum] = enum_votes.get(enum, 0) + 1
            gap = body[last_cmp_end:start] if last_cmp_end is not None else None
            if gap is not None and re.fullmatch(r"[\s|():]*(case\s*)?", gap):
                current.append(member)  # A == type || B == type, or stacked case labels
            else:
                current = [member]
            last_cmp_end = end
            continue
        last_cmp_end = None
        for member in current:
            # The first structural action in a branch decides; a nested READ wins over getters.
            prev = members.get(member)
            if prev is None or (prev[0] not in ("r1", "r2", "call") and kind in ("read", "call")):
                members[member] = val
    enum = max(enum_votes, key=enum_votes.get) if enum_votes else None
    return {"enum": enum, "members": members}


if __name__ == "__main__":
    import sys

    root = Path(__file__).resolve().parents[2]
    for fmt in READER_SOURCES:
        s = build_schema(root, fmt)
        n = sum(len(f["members"]) for f in s.functions.values())
        nested = sum(1 for f in s.functions.values() for k, _ in f["members"].values() if k in ("r1", "r2"))
        print(f"{fmt}: {len(s.functions)} reader functions, {n} members typed, {nested} nested, "
              f"{len(s.wrappers)} wrappers", file=sys.stderr)
        if len(sys.argv) > 1 and sys.argv[1] == fmt:
            for fn, info in sorted(s.functions.items()):
                print(fn, info["enum"])
                for member, (k, t) in info["members"].items():
                    print(f"    {member}: {k} {t or ''}")
