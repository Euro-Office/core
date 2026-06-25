# core — Euro-Office

@../AGENTS.md

Guidance for Claude Code (and other AI agents) working in **core** — the C++ document format conversion and rendering engine.

## What this repo is
A monolithic C++ library implementing document format conversion (X2T CLI: DOC/DOCX ↔ ODF/PDF/HTML/EPUB/XPS/DjVu/…), font metrics generation (AllFontsGen), headless rendering (DocxRenderer), document scripting (doctrenderer), and WASM modules (fonts, spell, zlib, hash, drawingfile). Lowest layer of the stack — everything else depends on its output. Fork of `ONLYOFFICE/core`. Licensed **AGPL-3.0**.

## Build Architecture & Critical Dependencies
- **Dependency chain:** `core-fonts` → `AllFontsGen` → `AllFonts.js` (sdkjs source tree) → `sdkjs` → `web-apps`. A font change requires regenerating `AllFonts.js` and rebuilding sdkjs. Conversion errors from X2T surface in `converter/out.log`, not in any Node.js trace.
- **vcpkg binary cache:** First cold build compiles all dependencies from source — **~5 hours**. The `NUGET_CACHE` env variable points to a remote binary cache reducing this to minutes.
- **Incremental builds are container-scoped:** CMake/ninja incremental state exists only for the current container's lifetime. After `docker compose up -d --force-recreate eo`, the next `make core` is a full rebuild.
- **WASM modules are a separate build:** Require Emscripten, built independently from native targets. A WASM build failure does not block the native build — they can diverge silently.

## Build & verify

Run from inside the `eo` dev container (see parent `../AGENTS.md` for container setup):

```sh
make core               # full build — all components
make core/x2t           # X2T format converter only
make core/allfontsgen   # font metrics generator — produces AllFonts.js for sdkjs
```

Builds are incremental (CMake/ninja) for the lifetime of the container. A `docker compose up -d --force-recreate eo` resets all incremental state — the next `make core` is a full rebuild.

## Rules
- **Never** hand-edit vendored third-party sources in `Common/3dParty/` or `DesktopEditor/agg-2.4/`. ICU, OpenSSL, and HarfBuzz are vendored sources built via `build_3rdparty.py` — do not patch them directly. For vcpkg-managed dependencies (`hunspell` in the root `vcpkg.json`; `Boost` in `Common/3dParty/boost/vcpkg.json`), bump the manifest instead of patching.
- **Never** commit the `build/` output directory or compiled binaries.
- **Never** assume a passing native build means WASM is also clean — they are built separately.

## Findings & Long-tail
No centralized findings store exists in this repository yet. Document edge cases in code comments or GitHub issues until one is established.
