<!--
SPDX-FileCopyrightText: 2026 Euro-Office contributors
SPDX-License-Identifier: AGPL-3.0-only
-->

# Binary format guards

sdkjs never reads OOXML or ODF directly. x2t converts every document to an
internal binary format (DOCY for text, XLSY for spreadsheets, PPTY for
presentations; the `Editor.bin` inside `.doct`/`.xlst`/`.pptt`), and converts
it back when saving. The format has two implementations: the C++ readers and
writers under `OOXML/Binary/`, and the JavaScript serializers in sdkjs. Stored
`Editor.bin` files also outlive upgrades. A change on one side that the other
side doesn't expect breaks documents without any error.

The tools here guard that contract. They use only the Python standard library.

## 1. Enum values are append-only

`check_binformat_enums.py` compares every enumerator, integer `#define` and
integer `const` in the four `BinReaderWriterDefines.h` headers with a base
revision:

- an existing name must keep its value, and must not be removed;
- a new name must not reuse a value already taken in the same enum;
- `g_nFormatVersion*` may only go up, and a bump is reported as a notice.

```bash
python3 Test/BinaryFormat/check_binformat_enums.py --base origin/main
```

CI runs it on every PR (`.github/workflows/binary-format.yml`). For a
deliberate break, bump the format version, keep the reader branch for the old
version, land the matching sdkjs change, and add the **`binary-format-break`**
label to the PR. The report is still printed.

## 2. Snapshots of the binary output

`binformat_dump.py` prints a binary file as a text tree, with record names
taken from the enums:

```text
table Style(5) @801 span=42314
  DefpPr(0) len=220
    Ind(1) Variable len=18
      Ind_Left(2) Double: 0 (i32 0)
```

The dump is lossless: every byte of the input shows up in it, with blobs over
32 bytes printed as length plus SHA-256. So any change to the bytes changes the
dump, and the diff names the record that changed. The self-tests check this by
flipping random bits.

The format is not self-describing. For DOCY and XLSY, `binformat_schema.py`
reads the record nesting and scalar types out of the C++ readers
(`BinaryReaderD.cpp`, `BinaryReaderS.cpp`), so the labels follow the code with
no hand-kept schema. Where it finds nothing, the dumper falls back to
heuristics (Read1, Read2, UTF-16, raw). PPTY records are numbered but not
named.

`run_snapshots.py` compares the dumps with `snapshots/`:

| Suite | Input | Needs a build | Runs in |
|-------|-------|---------------|---------|
| `templates` | `Common/empty/*.bin`, the new-document templates | no | `binary-format.yml` |
| `x2t` | the x2tTester inputs (`docx`/`odt`/`rtf`, `xlsx`/`ods`, `pptx`/`odp`) converted by x2ttester | yes | `build.yml`, server/amd64 leg |

```bash
python3 Test/BinaryFormat/run_snapshots.py templates
python3 Test/BinaryFormat/run_snapshots.py x2t --package build/package
python3 Test/BinaryFormat/binformat_dump.py path/to/Editor.bin    # inspect one file
```

A failing snapshot prints a unified diff. If the change is intended, and sdkjs
reads the new output, rerun with `--update` and commit the snapshot together
with the change. CI also uploads every dump as the `binformat-dumps-<sha>`
artifact, so snapshots can be taken from there when no local build is at hand.

The `x2t` suite runs with `--missing warn --print-new` in CI until its
snapshots are committed. `--print-new` also prints the missing snapshots into
the job log as a base64 tar.gz block, for when the artifact can't be
downloaded. Save the log and run `run_snapshots.py --import-log <log>` to write
them. Once they are committed, drop both flags so a missing snapshot fails.

## Self-tests

```bash
python3 -m unittest discover -s Test/BinaryFormat -p 'test_*.py'
```
