#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Parse the numeric constants of the internal binary format (DOCY/XLSY/PPTY/VSDY).

The serializer constants in the BinReaderWriterDefines.h headers are a wire
contract shared with the sdkjs serializers: a value written by one side is read
back by the other, and by every Editor.bin already stored on a server. This
module extracts every enumerator, integer #define and integer const from those
headers so they can be compared between revisions (check_binformat_enums.py)
and used to label records (binformat_dump.py).

Only the standard library is used.
"""

import ast
import operator
import re
from dataclasses import dataclass, field
from pathlib import Path

# Headers that define the binary format. Paths are relative to the core root.
DEFINES_HEADERS = {
    "DOCY": "OOXML/Binary/Document/BinWriter/BinReaderWriterDefines.h",
    "XLSY": "OOXML/Binary/Sheets/Common/BinReaderWriterDefines.h",
    "PPTY": "OOXML/Binary/Presentation/BinReaderWriterDefines.h",
    "VSDY": "OOXML/Binary/Draw/BinReaderWriterDefines.h",
}

# Constants that are expected to change when the format is versioned on purpose.
VERSION_CONSTANTS = {"g_nFormatVersion", "g_nFormatVersionNoBase64"}


@dataclass
class Group:
    """One enum, or the pseudo-groups for #define and const integers."""

    name: str
    kind: str  # "enum", "define" or "const"
    qualifier: str = ""  # how code names the members: innermost namespace, else the enum name
    members: dict = field(default_factory=dict)  # name -> int, in declaration order
    lines: dict = field(default_factory=dict)  # name -> line number


class ParseError(Exception):
    pass


_BINOPS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: operator.mul,
    ast.LShift: operator.lshift,
    ast.RShift: operator.rshift,
    ast.BitOr: operator.or_,
    ast.BitAnd: operator.and_,
}


def _strip_comments(text):
    """Blank out comments and string literals, keeping offsets and newlines."""

    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))

    return re.sub(r'//[^\n]*|/\*.*?\*/|L?"(?:\\.|[^"\\])*"', blank, text, flags=re.S)


def _eval_int(expr, scope):
    """Evaluate a C integer constant expression made of literals and known names."""
    expr = expr.strip()
    # C integer suffixes and casts that Python does not understand.
    expr = re.sub(r"\b(0[xX][0-9A-Fa-f]+|\d+)[uUlL]+\b", r"\1", expr)
    expr = re.sub(r"\(\s*(?:BYTE|int|long|unsigned\s+\w+|_INT32|_UINT32)\s*\)", "", expr)
    try:
        tree = ast.parse(expr, mode="eval")
    except SyntaxError:
        return None

    def ev(node):
        if isinstance(node, ast.Expression):
            return ev(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return node.value
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.USub, ast.UAdd, ast.Invert)):
            v = ev(node.operand)
            return {ast.USub: operator.neg, ast.UAdd: operator.pos, ast.Invert: operator.invert}[type(node.op)](v)
        if isinstance(node, ast.BinOp) and type(node.op) in _BINOPS:
            return _BINOPS[type(node.op)](ev(node.left), ev(node.right))
        if isinstance(node, ast.Name) and node.id in scope:
            return scope[node.id]
        if isinstance(node, ast.Attribute):
            # Qualified reference such as c_oSerConstants::ReadOk (rewritten to dots).
            parts = []
            n = node
            while isinstance(n, ast.Attribute):
                parts.append(n.attr)
                n = n.value
            if isinstance(n, ast.Name):
                parts.append(n.id)
                key = ".".join(reversed(parts))
                if key in scope:
                    return scope[key]
        raise ValueError(expr)

    try:
        return ev(tree)
    except (ValueError, TypeError):
        return None


_TOKEN = re.compile(
    r"(?P<ns>\bnamespace\s+(?P<nsname>\w+)\s*\{)"
    r"|(?P<enum>\benum\s+(?:class\s+|struct\s+)?(?P<enumname>\w+)?\s*(?::\s*[\w\s]+?)?\{)"
    r"|(?P<open>\{)"
    r"|(?P<close>\})"
    r"|(?P<define>^[ \t]*#[ \t]*define[ \t]+(?P<defname>\w+)[ \t]+(?P<defval>[^\n]+))"
    r"|(?P<const>\b(?:static\s+)?const\s+(?:static\s+)?(?:unsigned\s+)?(?:int|BYTE|char|short|long|_INT32|_UINT32)\s+(?P<constname>\w+)\s*=\s*(?P<constval>[^;]+);)",
    re.M,
)


def parse_defines(text):
    """Return {qualified group name: Group} for one header's text."""
    src = _strip_comments(text)
    line_of = lambda pos: src.count("\n", 0, pos) + 1  # noqa: E731

    groups = {}
    scope = {}  # names usable in later expressions (plain and qualified)
    stack = []  # ("ns", name) / ("block", None)
    pos = 0
    anon = 0

    def ns_prefix():
        return "::".join(n for k, n in stack if k == "ns")

    def group(name, kind):
        if name not in groups:
            groups[name] = Group(name=name, kind=kind)
        return groups[name]

    while True:
        m = _TOKEN.search(src, pos)
        if not m:
            break
        pos = m.end()
        if m.group("ns"):
            stack.append(("ns", m.group("nsname")))
        elif m.group("enum"):
            ename = m.group("enumname")
            if not ename:
                anon += 1
                ename = f"<anonymous{anon}>"
            body_start = m.end()
            body_end = src.find("}", body_start)
            if body_end < 0:
                raise ParseError(f"unterminated enum {ename} at line {line_of(m.start())}")
            prefix = ns_prefix()
            if prefix.split("::")[-1] == ename:
                # The usual spelling is namespace X { enum X {...} }; name it X once.
                qname = prefix
            else:
                qname = f"{prefix}::{ename}" if prefix else ename
            g = group(qname, "enum")
            g.qualifier = prefix.split("::")[-1] if prefix else ename
            nxt = 0
            cursor = body_start
            for item in src[body_start:body_end].split(","):
                item_pos = cursor
                cursor += len(item) + 1
                if not item.strip():
                    continue
                im = re.match(r"\s*(\w+)\s*(?:=\s*(.+?))?\s*$", item, re.S)
                if not im:
                    raise ParseError(f"cannot parse enumerator {item.strip()!r} in {qname}")
                name, expr = im.group(1), im.group(2)
                if expr is not None:
                    local = dict(scope)
                    local.update(g.members)
                    val = _eval_int(expr.replace("::", "."), local)
                    if val is None:
                        raise ParseError(f"cannot evaluate {name} = {expr.strip()} in {qname}")
                else:
                    val = nxt
                g.members[name] = val
                g.lines[name] = line_of(item_pos + len(item) - len(item.lstrip()))
                nxt = val + 1
                scope[name] = val
                # Qualified forms: enum-scoped and namespace-scoped.
                for q in (qname, prefix):
                    if q:
                        scope[f"{q}::{name}".replace("::", ".")] = val
                if prefix:
                    last_ns = prefix.split("::")[-1]
                    scope[f"{last_ns}.{name}"] = val
            pos = body_end + 1
        elif m.group("open"):
            stack.append(("block", None))
        elif m.group("close"):
            if stack:
                stack.pop()
        elif m.group("define"):
            val = _eval_int(m.group("defval"), scope)
            if val is not None:
                g = group("#define", "define")
                g.members[m.group("defname")] = val
                g.lines[m.group("defname")] = line_of(m.start("defname"))
                scope[m.group("defname")] = val
        elif m.group("const"):
            val = _eval_int(m.group("constval"), scope)
            if val is not None:
                prefix = ns_prefix()
                qname = f"{prefix}::const" if prefix else "const"
                g = group(qname, "const")
                g.members[m.group("constname")] = val
                g.lines[m.group("constname")] = line_of(m.start("constname"))
                scope[m.group("constname")] = val
    return groups


def load_defines(core_root):
    """Parse all format headers under core_root: {format: {group: Group}}."""
    root = Path(core_root)
    return {fmt: parse_defines((root / rel).read_text(encoding="utf-8", errors="replace"))
            for fmt, rel in DEFINES_HEADERS.items()}


def find_enum(groups, qualifier):
    """Look up an enum the way the C++ code qualifies its members, e.g. 'c_oSerTableTypes'.

    Members are written namespace::Member, and a few headers reuse an enum name
    inside a differently named namespace, so the namespace decides.
    """
    for g in groups.values():
        if g.kind == "enum" and g.qualifier == qualifier:
            return g
    for qname, g in groups.items():
        if g.kind == "enum" and qname.split("::")[-1] == qualifier:
            return g
    return None


if __name__ == "__main__":
    import json
    import sys

    root = sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[2]
    out = {}
    for fmt, groups in load_defines(root).items():
        out[fmt] = {name: g.members for name, g in groups.items()}
    json.dump(out, sys.stdout, indent=1)
    print()
