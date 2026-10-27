#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Stand-in for docbuilder, used by test_roundtrip.py only.

Parses a .docbuilder script the way RunTextA does (builder.* lines are
commands, '#' lines are dropped, everything else is JavaScript), and runs the
JavaScript in node against a mock Api with the method names facts.js uses.
The opened "document" is a small JSON model; SaveFile writes it into an
OOXML-like zip, so the next OpenFile reads it back.

Fault injection through the environment:
  FAKE_CRASH_ON_OPEN=N   exit 1 at the N-th OpenFile
  FAKE_DROP_ON_SAVE=1    every save loses one paragraph / sheet value / slide
  FAKE_UNSTABLE=1        every save writes a changing part outside docProps
"""

import json
import os
import re
import subprocess
import sys
import zipfile
from pathlib import Path

MOCK_API = r"""
var MODEL = __MODEL__;
function named(t) { return { GetClassType: function () { return t; } }; }
var Api = {
  GetDocument: function () {
    return {
      GetElementsCount: function () { return MODEL.paragraphs + MODEL.tables.length; },
      GetElement: function (i) { return named(i < MODEL.paragraphs ? "paragraph" : "table"); },
      GetAllParagraphs: function () { return new Array(MODEL.paragraphs); },
      GetAllTables: function () { return MODEL.tables.map(function (r) { return { GetRowsCount: function () { return r; } }; }); },
      GetAllDrawingObjects: function () { return [named("image")]; },
      GetAllComments: function () { return [{ GetText: function () { return "a comment"; } }]; },
      GetSections: function () { return [1]; },
      GetText: function () { return MODEL.text; }
    };
  },
  GetSheets: function () {
    return [{
      GetName: function () { return "Sheet1"; },
      GetUsedRange: function () {
        return {
          GetValue2: function () { return MODEL.values; },
          GetFormula: function () { return MODEL.values.map(function (r) { return r.map(function () { return ""; }); }); },
          GetAddress: function () { return "A1:B" + MODEL.values.length; }
        };
      },
      GetComments: function () { return []; },
      GetAllDrawings: function () { return [named("chart")]; }
    }];
  },
  GetPresentation: function () {
    return {
      GetAllSlides: function () {
        return MODEL.slides.map(function (t) {
          return {
            GetAllDrawings: function () { return [named("shape")]; },
            GetAllShapes: function () { return [{ GetDocContent: function () { return { GetText: function () { return t; } }; } }]; }
          };
        });
      }
    };
  }
};
"""


def default_model(path):
    return {"paragraphs": 3, "tables": [2], "text": "Hello\r\nworld from " + Path(path).name.split(".")[0],
            "values": [[1, "a"], [2, "b"]], "slides": ["Title", "Body"]}


def load(path):
    try:
        with zipfile.ZipFile(path) as z:
            if "model.json" in z.namelist():
                return json.loads(z.read("model.json"))
    except zipfile.BadZipFile:
        pass
    return default_model(path)


def save(model, path, n):
    if os.environ.get("FAKE_DROP_ON_SAVE"):
        model = dict(model, paragraphs=max(0, model["paragraphs"] - 1),
                     values=model["values"][:-1], slides=model["slides"][:-1])
    with zipfile.ZipFile(path, "w") as z:
        z.writestr("model.json", json.dumps(model))
        z.writestr("docProps/core.xml", f"<modified>{n}</modified>")
        body = "<doc><p>stable</p></doc>"
        if os.environ.get("FAKE_UNSTABLE"):
            body = f"<doc><p>stable</p><rsid>{n}</rsid></doc>"
        z.writestr("word/document.xml", body)


def js_value(js, model, expr):
    program = MOCK_API.replace("__MODEL__", json.dumps(model)) + js + f"\nprocess.stdout.write(String({expr}));\n"
    r = subprocess.run(["node", "-e", program], capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(r.stderr)
        sys.exit(1)
    return r.stdout


def params(line):
    return re.findall(r'"((?:[^"\\]|\\.)*)"', line)


def main():
    script = Path(sys.argv[-1]).read_text(encoding="utf-8")
    model, js, opens, saves = None, "", 0, 0
    for raw in script.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if not line.startswith("builder."):
            js += line + "\n"
            continue
        cmd = line[len("builder."):line.index("(")]
        args = params(line)
        if cmd == "OpenFile":
            opens += 1
            if str(opens) == os.environ.get("FAKE_CRASH_ON_OPEN"):
                print("error: cannot open file", file=sys.stderr)
                sys.exit(1)
            model, js = load(args[0]), ""
        elif cmd == "WriteData":
            value = args[1]
            m = re.fullmatch(r"jsValue\((.*)\)", value)
            if m:
                value = js_value(js, model, m.group(1))
            Path(args[0]).write_text(value, encoding="utf-8")
        elif cmd == "SaveFile":
            saves += 1
            save(model, args[1], saves)
        elif cmd == "CloseFile":
            model, js = None, ""
    return 0


if __name__ == "__main__":
    sys.exit(main())
