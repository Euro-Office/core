<!--
SPDX-FileCopyrightText: 2026 Euro-Office contributors
SPDX-License-Identifier: AGPL-3.0-only
-->

# ODS currency round-trip regression

`currency.ods` is a ready-to-open reproduction. Regenerate it from `currency.fods`
with Python 3 (standard library only):

```sh
python3 OdfFile/Test/number_formats/reproduce.py OdfFile/Test/number_formats/currency.ods
```

Open `currency.ods` in Euro-Office, save/download it as ODS, and reopen that
download in LibreOffice. Cells B2, B3 and B6 should retain `€`, B4 should retain
`$`, and B5 should remain without a currency symbol. B7 must stay a percentage
and B8 a date: both are controls for the reserved IDs the currency allocator
used to walk into. The numeric values must remain unchanged. `currency.fods` is
the readable source of the fixture.

Before the fix, the converter assigns grouped currency formats reserved XLSX
IDs starting at 7, despite having explicit currency symbols. Those IDs select
locale-dependent built-in formats when converted back to ODS. The resulting
ODS contains empty `number:currency-symbol` elements instead of the original
symbols. LibreOffice 24.2.7 displays these as `DM` with a German locale and `$`
with an English (US) locale. The symbol-free cell also becomes a percentage
because its assigned ID collides with the built-in percentage format.
The ungrouped euro control also loses its symbol: the ODS writer recognizes
`[$€-407]` but does not recognize the importer's locale-free `[$€]` token.

The fix allocates custom IDs for currency formats and accepts currency tokens
without a locale suffix when writing ODS.

Two further consequences of accepting the locale-free token are worth knowing:

* The old token pattern was greedy past the closing bracket, so a format code
  holding both a symbol token and a locale token (`[$€]#,##0.00[$-407]`) matched
  the whole span and produced `€]#,##0.00[$` as the currency symbol, taking the
  number placeholders with it. The narrowed pattern stops at the first `]`.
* Classifying a format as a currency also changes the cell, not just the data
  style: `ods_conversion_context` reads the detected type back and writes
  `office:value-type="currency"` where it previously wrote `"float"`. The writer
  emits no `office:currency` attribute, which ODF leaves optional.

An empty currency-symbol element means the locale's default symbol, rather than
no symbol; see [ODF 1.3, section 16.29.9](https://docs.oasis-open.org/office/OpenDocument/v1.3/os/part3-schema/OpenDocument-v1.3-os-part3-schema.pdf).

The writer side of the fix (which format codes map onto which ODF data style)
is covered by `NumberFormatDetection` in `OdfFile/Test/test_odf`, alongside the
date, time, percentage and accounting codes that must keep their existing
classification:

```sh
ctest --test-dir <build-dir> -R test_odf --output-on-failure
```

Run the allocator regression test independently of the full converter build:

```sh
cmake -S OdfFile/Test/number_formats -B /tmp/odf-number-formats-test
cmake --build /tmp/odf-number-formats-test
ctest --test-dir /tmp/odf-number-formats-test --output-on-failure
```

With a built converter, also verify the actual ODS → XLSX styles and
ODS → editor binary → ODS round trip:

```sh
python3 OdfFile/Test/number_formats/reproduce.py /tmp/currency.ods \
  --x2t /path/to/FileConverter/bin/x2t
```

This checks custom format IDs, explicit symbols, the intentionally blank symbol,
numeric values, and that the percentage and date controls neither share a format
ID with a currency nor change data style across the round trip. The saved result
is `/tmp/currency-roundtrip.ods`.

Verified with the rebuilt converter, the live editor's Download As ODS flow,
and LibreOffice 24.2.7 using a German locale:
the euro rows retain `€`, the dollar row retains `$`, and the symbol-free row
stays numeric without a currency symbol or percentage sign. The numeric values
are unchanged.
