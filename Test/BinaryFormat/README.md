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

A case without a snapshot fails. In CI the `x2t` suite runs with
`--print-new`, which also prints missing snapshots into the job log as a
base64 tar.gz block, for when the artifact can't be downloaded. Save the log
and run `run_snapshots.py --import-log <log>` to write them.

## 3. Round trip through real sdkjs

The snapshots only show what x2t writes. `roundtrip/roundtrip.py` also has
sdkjs read and write the format, through docbuilder. For each case, one
docbuilder script runs three passes:

| Pass | Step | Exercises |
|------|------|-----------|
| 1 | `OpenFile(source)`: x2t writes `Editor.bin`, sdkjs reads it | C++ writer, JS reader |
| | `SaveFile(pass1)`: sdkjs writes `Editor.bin`, x2t turns it into OOXML | JS writer, C++ reader |
| 2 | reopen `pass1`, save `pass2` | the same again, starting from our own output |
| 3 | reopen `pass2` | |

After each open, `roundtrip/facts.js` records structural facts through the
builder API:

- documents: element types, paragraphs, table rows, drawings, comments,
  sections, and the text (length, hash, first 120 characters);
- spreadsheets: each sheet's name, used range, values and formulas (hashed),
  comments and drawings;
- presentations: each slide's drawings and text.

For each case the check requires that:

- docbuilder succeeds and every pass writes its facts;
- the first load matches `roundtrip/expected/<case>.json`;
- the facts of pass 2 and pass 3 equal those of pass 1, so nothing is lost or
  invented by either serializer;
- `pass1` and `pass2` have the same OOXML parts, ignoring `docProps`. This
  stability check only warns unless `--stability fail` is passed.

A failure names the case, the pass and the fact, for example
`tableRows.length: expected 2, got 1`.

The cases are the x2tTester inputs (`docx`/`odt`/`rtf`, `xlsx`/`ods`,
`pptx`/`odp`) and the `Common/empty` templates. docbuilder can't open the
internal format itself: `OpenFile` leaves the editor type unset for
`.doct`/`.xlst`/`.pptt`, and every later call fails with "file not opened". So
each template is wrapped as such a package, x2t converts it to OOXML (the C++
reader reads the committed `Editor.bin` unchanged), and that file is
round-tripped like the inputs.

The script needs docbuilder, x2t and an sdkjs build side by side. CI
(`build.yml`, server/amd64 leg) runs it inside the e2e overlay image: the
nightly DocumentServer with the freshly built core binaries.

```bash
docker run --rm --user root --entrypoint python3 -v "$PWD:/core:ro" -v /tmp/rt:/out \
  euro-office/documentserver:e2e-core /core/Test/BinaryFormat/roundtrip/roundtrip.py --out /out
```

Every case's folder in `--out` keeps the script, the docbuilder log, the facts
and the saved files. `--out/expected/` holds the first-load facts of the run;
copy these into `roundtrip/expected/` (or rerun with `--update`) when a change
is intended. CI uploads the folder as `sdkjs-roundtrip-<sha>`, and runs with
`--missing warn` until the expectations are committed. A case without an
expectation also prints what it saw as a `seen:` JSON line in the log.

## Self-tests

```bash
python3 -m unittest discover -s Test/BinaryFormat -p 'test_*.py'
```

`test_roundtrip.py` drives `roundtrip.py` with `roundtrip/fake_docbuilder.py`, a
stand-in that parses docbuilder scripts and runs `facts.js` in node against a
mock `Api`. Those tests are skipped where node is missing.
