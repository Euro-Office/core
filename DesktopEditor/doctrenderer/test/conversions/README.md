<!--
  - SPDX-FileCopyrightText: 2026 Euro-Office contributors
  - SPDX-License-Identifier: AGPL-3.0-only
-->
# Conversion comparison

Checks a core build against the binaries of a DocumentServer image by converting
the same documents to PDF with both and comparing the PDFs: page count, text,
and rendered pages pixel by pixel. Use it whenever something under the JS engine
changes, above all when updating V8. `doctrenderer_engine_test` covers the
embedding itself; this covers sdkjs actually laying out documents.

It needs Docker and a core build (the directory that `EO_CORE_OUTPUT_DIR` points
into, with `bin/` and `tools/`):

```sh
DesktopEditor/doctrenderer/test/conversions/compare.sh /path/to/package
# optionally a directory with more documents:
DesktopEditor/doctrenderer/test/conversions/compare.sh /path/to/package ~/test-documents
```

`IMAGE` selects the DocumentServer image (default
`ghcr.io/euro-office/documentserver:latest-dev`), `OUT` the result directory
(default `./conversion-results`). It exits 0 when nothing differs.

In the image, it generates the fonts, then with each set of binaries runs
`allthemesgen`, `x2t -create-js-cache` and converts every document twice (the
second run gives the warm timings). For the build, it also converts once more
with the image's startup snapshots (`sdk-all.bin`) left in `sdkjs/`, as after an
update; they must be ignored.

The documents are listed in `corpus.txt`. Only add ones that convert the same
every time: a document using `RAND()` or `NOW()` differs between any two runs.
