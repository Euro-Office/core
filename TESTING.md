# Testing (C++ core)

The core ships ~50 test projects historically authored as **qmake `.pro`** files; 13 are
real [GoogleTest](https://github.com/google/googletest) suites. CI builds core with
**CMake/Ninja**, so tests are being migrated to **CMake targets registered with CTest** —
no separate qmake build is needed. This file documents how to run them and tracks the
per-suite migration.

## Running the tests

GoogleTest is pulled in through vcpkg via the `tests` manifest feature, so configure with
that feature and `-DEO_BUILD_TESTS=ON`:

```bash
cmake -GNinja -S . -B build \
  -DVCPKG_TARGET_TRIPLET=x64-linux-dynamic \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_MANIFEST_MODE=ON -DVCPKG_MANIFEST_DIR=. \
  -DVCPKG_MANIFEST_FEATURES=tests -DEO_BUILD_TESTS=ON

cmake --build build
ctest --test-dir build --output-on-failure
```

Run a single suite:

```bash
ctest --test-dir build -R cfcpp_test --output-on-failure
```

CI runs `ctest --test-dir build --output-on-failure` after the build step
(`.github/workflows/build.yml`); a failing assertion fails the job.

## Binary format (Editor.bin) guards

The internal DOCY/XLSY/PPTY format that sdkjs reads is covered by the Python
tools in [`Test/BinaryFormat/`](Test/BinaryFormat/README.md). One checks that
the enum values in the `BinReaderWriterDefines.h` headers are only ever
appended to. The other turns `Editor.bin` into a text dump; the conversion test
compares the dumps of what x2t writes with `out_*_snap`. Both run in CI, and the
tools' self-tests and the template snapshots also run in CTest.

## How a migrated suite is wired

CMake already builds every core library these tests link against. To migrate a suite:

1. Add a `CMakeLists.txt` next to the suite mirroring the `.pro`'s `ADD_DEPENDENCY` /
   `SOURCES` lines. Pull in each dependency with a guarded
   `if(NOT TARGET <lib>) add_subdirectory(...) endif()`.
2. Build and register it with the `add_core_gtest()` helper in `common.cmake`
   (`GTEST_MAIN` if the suite has no `main()`, `USE_GMOCK` if it uses gmock, optional
   `WORKING_DIRECTORY`).
3. Reproduce any extra `INCLUDEPATH` from the `.pro` with `target_include_directories`.
4. If the suite reads fixtures, stage them next to the test (or at the path the suite
   expects) with a `POST_BUILD` `copy_directory` and point `WORKING_DIRECTORY` at it.
5. `add_subdirectory(...)` the new `CMakeLists.txt` from the top-level `CMakeLists.txt`
   inside the `if(EO_BUILD_TESTS)` block.

`Common/cfcpp/test/CMakeLists.txt` is the reference example (own `main`, gmock, and
working-dir-relative fixtures).

## Migration status

Done:

- [x] CMake/CTest scaffolding — `add_core_gtest()` in `common.cmake`, `EO_BUILD_TESTS`
      option (default **OFF**; tests are opt-in, enabled with `-DEO_BUILD_TESTS=ON` plus the
      vcpkg `tests` feature) + `enable_testing()` in `CMakeLists.txt`, CTest step in CI.
- [x] `Common/cfcpp/test`
- [x] `DesktopEditor/graphics/tests/BooleanOperations_Unit-tests` — deps: kernel, graphics,
      UnicodeConverter. No fixtures. 17/20 tests run in CI; `OneIntersOutside`,
      `OneIntersInside`, `CurveIntersCurve` are quarantined via `GTEST_FILTER` (their
      computed boolean-path output differs from the expected paths — engine bug vs stale
      expectations not yet determined). See the `TODO` in that suite's `CMakeLists.txt`.
- [x] `OdfFile/Reader/Converter/StarMath2OOXML/TestSMConverter` — dep: StarMathConverter.
      No fixtures.
- [x] `OdfFile/Reader/Converter/StarMath2OOXML/TestEQNtoOOXML` — dep: StarMathConverter.
      No fixtures.
- [x] `OOXML/test` — most complex. Own `main()` (no `GTEST_MAIN`). Links the existing
      `x2tlib` shared target (X2tConverter/build/cmake) for the full converter dependency
      chain; adds only the dylib entry point `X2tConverter/src/dylib/x2t.cpp` to the test
      (x2tlib already compiles `cextracttools.cpp`, `ASCConverters.cpp`,
      `OfficeFileFormatChecker2.cpp`, so they are not duplicated). Compiled with
      `BUILD_X2T_AS_LIBRARY_DYLIB`. Converts the fixtures under `test/ExampleFiles/`
      (`xlsb2xlsx/`, `xlsx2xlsb/`, staged in the working directory) and compares the
      result with the same workbook saved by Microsoft Excel. The comparison is semantic
      (`workbook_facts.cpp`): sheets, defined names, cell values and formulas, cell
      formatting, merges, comments, tables, pivots, slicers and chart sheets. Byte
      comparison cannot work: x2t and Excel write the same content differently. For
      XLSX → XLSB both XLSB files are converted back to XLSX first. A category the
      workbook doesn't have is skipped. `Pivots` and `Slicers` of `xlsx2xlsb_simple2` are
      filtered out until #266 (flags never set in the XLSB writer) is fixed.
- [x] `X2tConverter/test/TestOOOXml2Odf` (`ooxml2odf_math_test`) — 45 cases for
      `StarMath::COOXml2Odf`, which converts OOXML math (`m:oMath`) to StarMath/MathML when
      OdfFormatLib writes ODF. It has a `.pro` but was missed by the first migration. Links
      the static format libraries in a group, like `test_odf`. `GTEST_MAIN` (`main.cpp` has
      no `main()`). No fixtures: the OOXML math is parsed from strings.
- [x] `OdfFile/Test/Test` (`odf_regression_test`) — 54 regression cases named after bug
      numbers (44363, 57197, 61364: ODT → DOCX; 59708: DOCX → ODT; 69238: PPTX → ODP),
      inspecting the converted document through the converters' models. Only a Visual
      Studio project existed and the suite had never built as committed (a duplicate global
      in `Test57197.cpp`). Links the static format libraries in a group, like `test_odf`;
      own `main()`. Fixtures under `ExampleFiles/` are staged in the working directory.
- [x] `OfficeUtils/tests` — deps: kernel, UnicodeConverter. The committed `tests/zip/`
      fixtures are staged next to the binary: the suite resolves fixtures relative to the
      process directory (`<binary dir>/../zip`), and writes its `unzip`/`temp` output dirs
      alongside (under the build tree, so writable).
- [x] `OfficeCryptReader/tests` — new suite (`officecrypt_test`), not a `.pro` migration.
      Covers the Crypto++ based encryption in `CryptTransform`: decrypts ECMA-376 agile
      data encrypted by msoffcrypto-tool and ODF 1.2 data encrypted with Python's
      `cryptography`, verifies ECMA-376/ODF write protection hashes computed with
      `hashlib`, and round-trips agile, standard (AES, RC4) and ODF (AES, Blowfish)
      encryption. Deps: CryptoPPLib, kernel, UnicodeConverter, plus
      `OOXML/Base/unicode_util.cpp` compiled in. The reference data is generated into
      `reference_vectors.h` by `generate_vectors.py`; no fixture files.
- [x] `OdfFile/Reader/Converter/SMCustomShape2OOXML/TestSMCustomShape` — dep:
      SMCustomShape2OOXML (new CMake library target created for this suite; itself depends on
      UnicodeConverter, kernel). No fixtures.
- [x] `OOXML/DocxFormat/test` — new suite (`docx_numbering_test`), not a `.pro` migration.
      Added to cover a DOCX bullet/numbering-list schema-ordering fix (Microsoft 365
      Online was misclassifying list types because `<w:lvl>` children were emitted out of
      ECMA-376 `CT_Lvl` sequence). Deps: links the existing `x2tlib` CMake target (the same
      one `OOXML/test` uses) — `DocxFormatLib` alone is not link-complete for
      `Numbering.cpp` (it pulls in `Logic/SectionProperty.h`, which reaches `PPTXFormat`
      and `XmlUtils` symbols that only `x2tlib`'s full link supplies), and
      `PPTXFormat/Namespaces.cpp` is added directly as a source (a small pre-existing gap
      in the CMake port, unrelated to this suite). `GTEST_MAIN` (no own `main()`). No
      fixtures — constructs `OOX::Numbering::CLvl` objects directly and asserts the
      ECMA-376 `CT_Lvl` element order of `toXML()`'s output (including a fully-populated
      level covering all twelve possible child elements), plus `fromXML()`'s tolerance of
      non-schema-ordered input.
- [x] `DesktopEditor/raster/test` — new suite (`raster_formats_test`), not a `.pro`
      migration. Covers the PNM (`raster/Pnm`) and Sun Raster (`raster/Ras`) decoders that
      replaced CxImage's JasPer based decoding, through `CBgraFrame::Decode` /
      `OpenFile` and `CImageFileFormatChecker`: all Netpbm variants (ASCII/binary, 16 bit,
      comments, row padding), RAS at 1/8/24/32 bit, BGR and RGB types, color maps and RLE,
      plus truncated and oversized input. Also covers CxImage's JPEG codec on libjpeg-turbo:
      baseline, progressive, arithmetic, grayscale and Adobe CMYK decoding in both channel
      orders, partial decoding of truncated files, and `CBgraFrame::Encode` read back with
      libjpeg. And its PNG codec on libpng: RGBA, 16 bit, palette and gray transparency,
      interlaced 2 bit gray, `CBgraFrame::Encode` and `SaveFile` of a palette image read
      back with libpng. And its TIFF codec on libtiff: uncompressed, LZW, Deflate, PackBits
      and JPEG in strips and tiles, CCITT G4, palette, 16 bit, multipage, and an IFD loop
      that libtiff 3.9.4 never returned from. And the WebP decoder (`raster/WebP`) on
      libwebp: lossy and lossless in both channel orders, alpha (lossless and next to
      lossy color), the first frame of an animation, `OpenFile`, and truncated input.
      And the HEIF decoder (`raster/heif`) on libheif: HEIC and AVIF detection (also by
      file name, as before inserting a file), HEIC in both channel orders, AVIF with
      alpha and at 10 bits, rotation from `irot`, `OpenFile`, and truncated input.
      And ICO and PSD: detection of bitmap, PNG and cursor icons and of PSD (not PSB),
      icons of both kinds in both channel orders, PSD raw and RLE at 8 and 16 bits, with
      transparency, with a layer section that is skipped, and truncated or corrupt RLE
      data.
      Deps: kernel, graphics, UnicodeConverter, jpeg, png, tiff, webp and webpmux (to
      build and read the test images). `GTEST_MAIN`. Images are built in memory, except
      the HEIF ones: core cannot encode HEIF, so `heif/` holds small fixtures from
      `heif/generate.py`, read in place.
- [x] `DesktopEditor/fontengine/test` — new suite (`fontengine_test`), not a `.pro`
      migration. Covers the FreeType based font engine in libgraphics: `CApplicationFonts`
      (font list, localized family names read from FreeType's internal sfnt name table) and
      `CFontManager` (font type, metrics incl. `head` and `OS/2` values, cmap lookup,
      advances, glyph boxes) for TrueType, WOFF2 (brotli), OpenType/CFF and Type 1.
      Deps: kernel, graphics, UnicodeConverter. `GTEST_MAIN`. Fixtures are read in place:
      `fonts/` holds tiny fonts with exactly known metrics generated by `fonts/generate.py`
      (fontTools), the Type 1 font is `PdfFile/Resources/Fonts/n019003l.pfb`.
- [x] `DesktopEditor/raster/Jp2/openjpeg/test` — new suite (`openjpeg_test`), not a `.pro`
      migration. Covers `openjpeg::GetData` (`opj_bgraframe.cpp`), the JPEG 2000 decoder behind
      PdfFile's JPXDecode streams. Compiles the same openjp2 sources (from
      `Common/3dParty/openjpeg`) and wrapper as PdfFile; dep: kernel. `GTEST_MAIN`. No
      fixtures — images are encoded losslessly in memory with OpenJPEG's encoder and must
      decode back exactly: J2K/JP2 with 1/3/4 components, tiled multi-resolution, the
      unflipped (bottom-up) row order, and an image above the 5 MP strip-decoding limit.
      Plus non-JPEG 2000, truncated and corrupted input (must return, not crash).
- [x] `PdfFile/test/jbig2` — new suite (`pdf_jbig2_test`), not a `.pro` migration. Covers
      JBIG2 encoding on jbig2enc and Leptonica: `CPdfFile::DrawImage1bpp` (DjVu pages, symbol
      mode with shared `JBIG2Globals`, also across the writer's five-image dictionary
      limit), `DrawImageWith1bppMask` (DjVu color pages) and `CBgraFrame::SaveFile` with
      type 21 (a generic region, wrapped into a hand-built PDF). Every result is decoded
      again by xpdf's JBIG2 decoder through `ConvertToPixels` at 1:1 and compared with the
      source pixels. Deps: UnicodeConverter, kernel, graphics, PdfFile. `GTEST_MAIN`. No
      fixtures — pages are built in memory.
- [x] `DjVuFile/test` — new suite (`djvu_test`), not a `.pro` migration. Covers `CDjVuFile` on
      DjVuLibre's libdjvu (`Common/3dParty/djvulibre`): page count, sizes and resolution of a
      bundled document, an exact 1:1 rendering of a JB2 page, an IW44 color page (with a
      tolerance), bookmarks (`GetStructure`), links (`GetLinks`), the text layer as written to
      a PDF page, and truncated or overwritten input (must not crash). Deps: UnicodeConverter,
      kernel, graphics, PdfFile, DjVuFile. `GTEST_MAIN`. Fixtures are read in place: `data/`
      holds three tiny documents generated by `data/generate.py` with DjVuLibre's tools.
- [x] `RtfFile/Format/test` — new suite (`rtf_lex_test`), not a `.pro` migration. First
      coverage for the RTF tokenizer that fronts every RTF import (`RtfLex`, rtf2docx).
      Deps: links the existing `x2tlib` target; `RtfLex.cpp`, `RtfToken.cpp` and
      `Utils.cpp` are compiled directly into the test binary (their symbols are hidden
      inside the shared `x2tlib`, same pattern as `docx_numbering_test` compiling
      `PPTXFormat/Namespaces.cpp`). `GTEST_MAIN`. No fixtures — each case writes its RTF
      snippet to a temp file (the lexer only accepts file paths); every snippet rides a
      minimal `{\rtf1 …}` envelope because `SetSource` refuses files under 5 bytes.
      16 tests: token dispatch (group/keyword/control/text/EOF incl. EOF persistence),
      keyword parameters (positive, negative, trailing-space consumption), escaped
      delimiters, control symbols, `\'hh` hex control tokens (the decimal-key quirk and
      the +1 shift for the 0x00–0x1E range), backslash-tab/CR/LF → `tab`/`par`, newline
      suppression inside text runs, control-byte skipping, the 5-byte error contract,
      and read-buffer growth past the initial 5 MB (a 6 MB text run lexes whole). The
      binary links the system `libstdc++` ahead of the converter chain (ICU
      interposition, #173).
- [x] `OOXML/Binary/Sheets/Reader/test` — new suite (`csv_reader_test`), not a `.pro` migration.
      First coverage for the bin-format CSV reader used by every CSV/TXT import: encodings
      (ANSI/UTF-8/UTF-16LE/UTF-32LE, BOM), `sep=` directive, RFC4180 quoting, tab handling,
      empty-cell positions, the 500k-char chunked-erase path, and the `readToxmlCache_` branch.
      Carries the regression case (`Utf8TrailingLineBreakAddsNoEmptyRow`) for the #160
      ghost-row bug fixed in #170. The binary links the system `libstdc++` ahead of the
      converter chain on Linux (ICU interposition, #173).
- [x] `OdfFile/Test/test_odf` — deps: OdfFormatLib (for `ConvertODF2OOXml`), kernel,
      graphics, UnicodeConverter; `OfficeFileFormatChecker2.cpp`, `pole.cpp` and
      `unicode_util.cpp` are compiled into the target (as `x2tTester` does). Owns its
      `main()` (so no `GTEST_MAIN`). The `.pro` built only `test.cpp` + `common.cpp`;
      the CMake target also builds the entrance/motion/audio/interactions modules (144
      cases), which convert their `.odp`/`.pptx` fixtures in both directions and inspect
      the converters' animation trees. Their environments in `common.cpp` read the
      fixtures working-dir-relative. `columns.cpp` stays out: `columns.pptx` was never
      committed. The
      global `TestEnv` exercises an `.odp -> .pptx` conversion in its `SetUp()`, and
      `ODP2PPTX.EmptyCustomShapeFormula` converts `empty_custom_shape_formula.odp`, a
      regression fixture for a custom-shape equation with an empty `draw:formula`. Fixtures under `ExampleFiles/` are
      staged next to the binary and the test runs from there (the fixture path
      `ExampleFiles/motion.odp` is working-dir-relative). The committed `common.cpp` had
      absolute Windows include paths and a `#pragma comment(lib, ...)` block; these were
      replaced with repo-relative includes (CMake links the libraries).
      Tests that check one XML part of a converted document use the helpers in
      `common.h`: `convert_odf_to_ooxml_dir()` (ODT/ODS/ODP → unpacked OOXML, via
      `ConvertODF2OOXml`), `convert_ooxml_to_odf_dir()` (DOCX/XLSX/PPTX → unpacked ODF,
      via `Oox2Odf::Converter` as x2t calls it), `read_part()` and `is_well_formed()`.
      Both converters run in-process, not through `X2T_Convert`, so they work headless.
      The output directories are removed by the global `TearDown()`.
      `conversion_helpers.cpp` holds the controls: `plain_paragraph.odt` and
      `plain_paragraph.docx` convert and the expected part is well-formed and holds the
      text, and `is_well_formed()` rejects an unescaped `&` and broken markup.
- [x] `DesktopEditor/xmlsec/src/test` (`xmlsec_test`) — deps: kernel, ooxmlsignature. Not a
      GoogleTest suite (unlike every other entry here) — a plain `main()` returning a real
      exit code, since it exercises `COOXMLSigner`/`COOXMLVerifier` directly rather than
      asserting individual cases. Signs `file.docx` and verifies the result in the same
      process (`USE_SIGN`/`USE_VERIFY`), and separately verifies a committed fixture,
      `legacy-signed.docx`, signed by upstream ONLYOFFICE Desktop Editors 9.4.0 under
      OpenSSL 1.1.1f (`USE_VERIFY_LEGACY`) — proving documents signed under OpenSSL 1.1.1
      still verify under this fork's 4.0.1. Because it copies itself into
      `EO_CORE_OUTPUT_DIR` (matching `x2t`/`UnicodeConverter`'s convention, not the
      `WORKING_DIRECTORY $<TARGET_FILE_DIR:...>` pattern the other suites use), its `add_test`
      runs that copy directly rather than the raw build-tree binary. **Not** the same as the
      `DesktopEditor/xmlsec/src/osign/test` suite below despite the similar path —
      `osign` is an unrelated, separate signing library.
- [x] `DesktopEditor/doctrenderer/test/json` — dep: doctrenderer (V8). Define
      `JSON_GOOGLE_TEST`; own `main()` is compiled out under that define, so `GTEST_MAIN`.
      No fixtures (JS built inline via `runScript`).
- [x] `DesktopEditor/doctrenderer/test/js_internal` — dep: doctrenderer (V8). Define
      `JS_INTERNAL_GOOGLE_TEST`; own `main()` is compiled out under that define, so
      `GTEST_MAIN`. No fixtures.
- [x] `DesktopEditor/doctrenderer/test/embed/internal/hash` — dep: doctrenderer (V8). No
      google-test define; no own `main()`, so `GTEST_MAIN`. No fixtures.

  **V8 runtime requirement (satisfied in CI):** these three suites instantiate
  `CJSContext`, which initializes a V8 isolate. The V8 monolith is built in CI by
  `common.cmake` (`build_3rdparty.py`, populating `V8_INSTALL_DIR`) and is self-contained
  at run time: the x64-linux V8 build sets `v8_use_external_startup_data=false` (startup
  snapshot is linked into the monolith) and `v8_enable_i18n_support=false` (no external
  `icudtl.dat`), so no external V8 runtime data files need to be staged next to the test
  binaries. The `doctrenderer` shared library these suites link is already built in CI
  (the `docbuilder` app links it), so the suites run headless without extra provisioning.
- [x] `PdfFile/test` (`pdffile_test`) — deps: UnicodeConverter, kernel, graphics, PdfFile,
      DjVuFile, ooxmlsignature. Covers `CPdfFile`: writing and reading (page sizes, colours
      read back by rendering), loading from memory, document info, metadata
      (`ONLYOFFICEFORM`), passwords and `ChangePassword`, editing (rotate, delete, move,
      add, draw), `SplitPages`, `MergePages`, copying pages through the renderer, and
      signing and verifying with a generated RSA certificate. `GTEST_MAIN`. No fixtures:
      every test writes the PDFs it needs with `CPdfFile` itself, in its own directory.
      `DrawTextInsideRedaction` drives the redaction early-return in `CPdfWriter::DrawText`
      (#224); it checks the path succeeds, not the heap, so it would not catch the leak alone.
      It replaces ONLYOFFICE's scratchpad, whose cases mostly asserted nothing, depended on
      each other's output files and needed fixtures that were never published, among them
      editor binaries (`pdf.bin`, `changes.bin`, `base64.txt`) only sdkjs produces. Not
      covered yet: DjVu to PDF, the editor binary formats, applying redactions and printing.
- [x] `MsBinaryFile/DocFile/test` (`doc_fkp_test`) — new suite, not a `.pro` migration.
      Regression tests for the bounds checks in the `.doc` Formatted Disk Page parsers
      (`FormattedDiskPageCHPX`/`PAPX`). Each case writes a hand-built 512 byte FKP into a
      temporary compound file through POLE and runs the real constructors on it. Deps:
      DocFormatLib + XlsFormatLib (for `Log::warning`) in a linker group, kernel,
      UnicodeConverter; `pole.cpp`/`unicode_util.cpp` are compiled into the target as
      `x2tTester` does. No fixtures. Building it with `-fsanitize=address` also turns any
      remaining out-of-bounds read into a failure.
- [x] `MsBinaryFile/PptFile/test` (`pptfile_test`) — new suite, not a `.pro` migration.
      Regression cases for `CRecordExObjStg`, the reader of embedded OLE objects and
      `vbaProject.bin` in `.ppt` files: a compressed record too short to hold its size field,
      a claimed size the stored data cannot inflate to or above the 128 MiB ceiling, data
      that does not inflate, and stored, compressed and named objects that must still come
      out intact. Links `PptFormatLib` with the rest of the format-library group the same
      way `OdfFile/Test/test_odf` does. `GTEST_MAIN`. No fixtures: each case writes its own
      compound file into a fresh temp directory with POLE.
- [x] `DesktopEditor/xmlsec/src/osign/test` — deps: kernel and a new `osign` CMake library
      target (`DesktopEditor/xmlsec/src/osign/lib/CMakeLists.txt`) ported from `osign.pro`.
      No fixtures — the suite generates its own passwords and certificates at run time.
      `common_openssl.cpp` is compiled into the library (`osign.pro` pulls it in through
      `CONFIG += open_ssl_common`, and `certificate.cpp` needs `NSOpenSSL::PBKDF2` /
      `AES_*_desktop_GCM`); `Base64.cpp` and `File.cpp` are **not** — the `.pro` compiled
      them directly because qmake did not link kernel, but the CMake target links `kernel`,
      which already provides `NSBase64`/`NSFile`. Unlike `ooxmlsignature` — which `x2tlib`
      pulls into the default build — `osign` has no non-test consumer, so it is reached
      only through the test's guarded `add_subdirectory` and nothing changes when
      `EO_BUILD_TESTS` is OFF.
      `certificate.cpp` was written for OpenSSL 1.1 and was ported to the OpenSSL 4 API
      (opaque `ASN1_STRING`, const `X509_get_subject_name`/`X509_get_ext`) for this.

### gtest suites to migrate

Runnable headless once migrated (no missing fixtures, no JS engine):

_(none — all migrated; see Done above.)_

Blocked / need extra work (build targets intentionally not created yet):

_(none)_

### Deferred: non-gtest qmake `.pro` tools

The remaining ~37 `.pro` "tests" are interactive/GUI tools (SVG dumpers, viewers, manual
testers) — they don't assert and can't run headless, so they are **not** CTest candidates.
They can optionally be given build-only CMake targets later; until then build them with
qmake (`qmake <tool>.pro && make`) against a qmake build of core.
