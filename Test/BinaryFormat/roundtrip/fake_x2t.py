#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
"""Stand-in for x2t, used by test_roundtrip.py only.

Reads the TaskQueueDataConvert parameter file, checks that m_sFileFrom is a
.doct/.xlst/.pptt package whose Editor.bin carries the matching signature, and
writes an empty OOXML-like zip to m_sFileTo (fake_docbuilder.py then opens it
with its default model). FAKE_X2T_FAIL=1 makes it exit with an error.
"""

import os
import re
import sys
import zipfile
from pathlib import Path

SIGNATURES = {".doct": b"DOCY", ".xlst": b"XLSY", ".pptt": b"PPTY"}


def main():
    params = Path(sys.argv[1]).read_text(encoding="utf-8")
    get = lambda tag: re.search(f"<{tag}>(.*?)</{tag}>", params).group(1)  # noqa: E731
    source, target = Path(get("m_sFileFrom")), Path(get("m_sFileTo"))
    if os.environ.get("FAKE_X2T_FAIL"):
        print("fake x2t: conversion failed", file=sys.stderr)
        return 89
    with zipfile.ZipFile(source) as z:
        if not z.read("Editor.bin").startswith(SIGNATURES[source.suffix]):
            print("fake x2t: wrong signature", file=sys.stderr)
            return 1
    with zipfile.ZipFile(target, "w") as z:
        z.writestr("[Content_Types].xml", "<Types/>")
    return 0


if __name__ == "__main__":
    sys.exit(main())
