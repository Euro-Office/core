<!--
SPDX-FileCopyrightText: 2026 Euro-Office contributors
SPDX-License-Identifier: AGPL-3.0-only
-->
# x2t for WebAssembly

`x2t.wasm` is the document converter running in the browser. The editor's
client-only ("local session") mode uses it to turn a docx/xlsx/pptx/odf file
into the editor's binary format (`Editor.bin`) without a Document Server
session, so that mail attachments and end-to-end encrypted files can be viewed
without any server seeing the document.

The approach follows [CryptPad's onlyoffice-x2t-wasm](https://github.com/cryptpad/onlyoffice-x2t-wasm):
the full converter library graph linked statically with Emscripten, driven
through a `params.xml` in the in-memory file system. CryptPad builds an older
Euro-Office core snapshot with qmake; this directory does the same with the
CMake graph of the current tree, so the patches are guarded (`EMSCRIPTEN` in
CMake, `__EMSCRIPTEN__` in sources) and native builds are unaffected.

**Status: the CMake wiring is complete but has not been compiled yet.** It
needs the `ghcr.io/euro-office/emsdk` container; the `build-x2t-wasm` job in
`.github/workflows/build-wasm.yml` attempts it on pull requests without
blocking the sdkjs wasm modules. Expect the first runs to surface link and
third-party issues, see "Known gaps" below.

## Building

```sh
# in the emsdk container (ghcr.io/euro-office/emsdk)
mkdir build-wasm && cd build-wasm
emcmake cmake -S /path/to/core -B . -DEO_WASM_X2T=ON -DEO_CORE_OUTPUT_DIR=$PWD/package
cmake --build . --target x2t
ls package/x2t   # x2t.js x2t.wasm converter.js x2t-worker.js
```

`EO_WASM_X2T` is off by default. Switching it on extends the third-party set
built by `Common/3dParty/build_3rdparty.py` with `boost-wasm`, `html` and `md`.
`THIRD_PARTY_PREPARED` is cached, so in an existing build directory run
`build_3rdparty.py --only=boost-wasm,html,md <work> <install>` by hand first.

The directory can also be configured on its own:
`emcmake cmake -S X2tConverter/build/wasm -B build-x2t-wasm`.

Build inputs that differ from the native graph (all wired in `CMakeLists.txt`
here, directory-scoped):

| Native | WebAssembly |
|---|---|
| ICU shared libs (`icu`) | Emscripten port, `-sUSE_ICU=1` |
| zlib-ng/minizip (`zlib-ng`) | `zlib-ng-wasm` |
| OpenSSL (`openssl`) | `openssl-hash` |
| libxml2 (`libxml2`) | `libxml2-wasm` (already used by the sdkjs modules) |
| Boost b2 build (`boost`) | `boost-wasm`: Boost's CMake superproject under emcmake |
| V8 (`v8`) in doctrenderer | no JS engine, `DesktopEditor/doctrenderer/wasm/doctrenderer_stub.cpp` |
| socket.io in kernel_network | FileTransporter only (`USE_EXTERNAL_TRANSPORT`) |

## What is linked

Formats in the first link: docx, dotx, docm, doc, odt, rtf, txt, html, epub,
xlsx, xltx, xlsm, xls, ods, csv, pptx, potx, pptm, ppt, odp, and PDF *output*
from the editor's print data (see below). PDF, DjVu, XPS and OFD *input*
rendering, iWork, HWP and FB2 are left out (`X2T_WASM_SKIPPED_LIBS` in
`build/cmake/library/CMakeLists.txt`) to keep the binary small; the editor
renders PDF client-side on its own. A conversion that reaches one of the
skipped libraries fails at run time, because the executable links with
`-sERROR_ON_UNDEFINED_SYMBOLS=0`.

Everything that runs sdkjs inside x2t (bin to PDF/HTML rendering, JS caches,
document builder) is unavailable: in the browser the editor is the JavaScript
side. `to: 'pdf'` therefore takes the editor's print output (`pdf.bin`) and
only wraps it, the same way CryptPad implements print.

## JavaScript API

`converter.js` is an ES module; the conversion runs in a Web Worker
(`x2t-worker.js`) that loads `x2t.js` and `x2t.wasm` from the same directory.

```js
import { createConverter } from 'https://ds.example.org/x2t/converter.js';

const x2t = await createConverter();              // { baseUrl } overrides the directory
const { data, media } = await x2t.convert({
  data: bytes,                                    // ArrayBuffer | Uint8Array | Blob
  fileName: 'report.docx',                        // extension selects the input format
  to: 'bin',                                      // bin | docx | xlsx | pptx | odt | ods | odp | csv | txt | rtf | pdf ...
  fonts: { 'arial.ttf': arrayBufferOrUrl },       // optional, written to /working/fonts
  themes: { 'theme1/theme.xml': arrayBuffer },    // optional, written to /working/themes
  media: { 'image1.png': arrayBuffer },           // optional, written to /working/media (export)
  type: 'word',                                   // word | cell | slide, required when fileName ends in .bin
  pdfBin: printBytes,                             // required for to: 'pdf'
});
// data: Uint8Array of the output file
// media: { 'image1.png': Uint8Array, ... } extracted images when to === 'bin'
x2t.terminate();
```

Errors reject with an `Error` whose `code` is `load` (x2t.js/x2t.wasm could
not be loaded or the worker died), `convert` (x2t returned a non-zero exit
code, in `exitCode`), `unsupported` (format or missing `type`/`pdfBin`) or
`memory`. ODF inputs are retried through their OOXML sibling when the direct
conversion fails.

`params.xml` is the Document Server's `TaskQueueDataConvert`: `m_sFileFrom`,
`m_sFileTo`, `m_sFontDir`, `m_sThemeDir`, explicit `m_nFormatFrom`/`m_nFormatTo`
from `Common/OfficeFileFormats.h`, and the CSV encoding/delimiter defaults.
The worker exports `main1(xmlPath)` from the wasm module, which is
`x2t params.xml`.

## Testing

```sh
node test/run.js package/x2t                   # x2tTester sample documents -> bin -> ooxml/odf
X2T_NATIVE=/path/to/x2t node test/run.js package/x2t my.docx my.xlsx
```

With `X2T_NATIVE` the bin output size is compared with the native converter's.

## Known gaps

- Memory: the first successful compile needs about 280 MB of static data, so
  `INITIAL_MEMORY` is 320 MB. That is too much for phones; finding what takes
  the space (embedded tables, ICU data, format libraries x2t never calls here)
  is the next size task.
Things the first CI runs are expected to hit, in rough order of likelihood:

- Shared library targets under Emscripten: the graph declares every library
  `SHARED`; emcc emits static objects for `-shared` links, which is what the
  executable link relies on. If a library's link line carries options wasm-ld
  rejects, guard them like the version scripts in `graphics` and `PdfFile`.
- Third-party builds: `boost-wasm` is new (CryptPad's recipe), `html` and
  `md` are fetch-only and compile in-tree. The flex lexer in katana is built
  with `YY_NEVER_INTERACTIVE`; CryptPad patched the same line by hand.
- `kernel_network` under Emscripten drops the socket.io sources; check that no
  other library pulls `WebSocket/` headers.
- Binary size and memory: no numbers yet. CryptPad brotli-compresses the
  output; the Document Server should serve `/x2t/` with the same cache policy as
  `web-apps/`.
