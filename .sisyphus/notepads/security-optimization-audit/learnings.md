## 2026-03-30 Session Start: security-optimization-audit
- Session: ses_2c046a7efffepSlnlTh0U0r3vQ
- Plan: 39 implementation tasks + 4 verification tasks across 6 waves
- Key: This is a C++17 codebase (ONLYOFFICE Core), NOT a web app
- clang-13 required for V8 builds
- WASM build must be preserved

## Task 1: GoogleTest Integration (2026-03-30)

### Patterns & Conventions
- Root CMakeLists.txt uses `add_subdirectory()` in the `else()` block (line 18+) for non-Emscripten builds
- `enable_testing()` must be called at the ROOT CMakeLists.txt level for CTest to find tests from the build root
- `gtest_discover_tests()` from `include(GoogleTest)` creates test discovery at build time
- `set_default_options()` from common.cmake applies project-wide compiler flags to targets
- `gtest_force_shared_crt ON` prevents GoogleTest from overriding CRT settings

### Gotchas
- GoogleTest source must be placed at `Common/3dParty/googletest/googletest/` (nested) to match existing .pri include paths
- The .gitignore already has `googletest/` entry, so the downloaded source won't be tracked
- Full project build fails due to missing V8 dependency - pre-existing issue, not caused by gtest integration
- Building just `--target gtest_smoke_test` is sufficient to verify the integration

### Build Verification
- `cmake -GNinja -B /tmp/gtest-test /opt/git/core` - configures successfully
- `cmake --build /tmp/gtest-test --target gtest_smoke_test` - builds 6 targets (gtest, gtest_main, smoke_test)
- `ctest --test-dir /tmp/gtest-test --output-on-failure` - SmokeTest.Passes: 100% pass

## Task 2: ENABLE_SANITIZERS CMake Option (2026-03-30)

### Patterns & Conventions
- Sanitizer flags added to COMMON_CXX_FLAGS, COMMON_C_FLAGS, and COMMON_LINK_OPTIONS conditionally
- `-fsanitize-minimal-runtime` is clang-only; must guard with `CMAKE_CXX_COMPILER_ID MATCHES "Clang"` for GCC compat
- Full project cmake configure times out (>60s) due to network fetches — use minimal CMakeLists.txt to verify flag injection
- `set_default_options()` already propagates COMMON_CXX_FLAGS, COMMON_C_FLAGS, COMMON_LINK_OPTIONS to targets via generator expressions

### Gotchas
- GCC 12 doesn't support `-fsanitize-minimal-runtime` — only clang-13 does
- Environment variables (ASAN_OPTIONS, UBSAN_OPTIONS) set via `set(ENV{...})` in CMakeLists.txt take effect at configure time, not build time — suppressions are a convention, not enforced at build
- Pre-existing LSP errors in OfficeCryptReader and X2tConverter (missing boost/cryptopp) are unrelated

### Build Verification
- `cmake -GNinja -B /tmp/san-build-test/build /tmp/san-build-test -DENABLE_SANITIZERS=ON` — configure OK, "Sanitizers enabled: ASAN + UBSAN"
- `cmake --build /tmp/san-build-test/build` — builds successfully, binary contains __asan_init
- Default build (no flag) — no sanitizer symbols in binary

## Task 6: libxml2 Audit (2026-03-30)

### Key Findings
- Vendored libxml2 is version 2.9.2 (October 2014), 101 .c files, 69 .h files
- ONLYOFFICE made exactly **2 source modifications** to the libxml2 codebase:
  1. `xmlversion.h` — Template placeholders for version numbers (build-time substitution)
  2. `error.c` — `XML_ERROR_DISABLE_MODE` guard to suppress error output in release builds
- No other source files were modified (confirmed by searching all 170 files for ONLYOFFICE/Ascensio strings)

### Build System Customizations
- 4 separate build systems: CMake (Linux), qmake (Qt desktop), VS2013 (Windows), Emscripten (WASM)
- Release builds use unity/concatenated compilation (`libxml2_all.c` + `libxml2_all2.c`)
- parser.c is compiled separately in a different TU (compilation order dependency)
- `xmlcatalog.c` included in VS2013 but NOT in CMake build (inconsistency)
- `_USE_LIBXML2_READER_` defined in 40+ places but NEVER checked in code — dead legacy define

### libxml2 API Usage
- ONLYOFFICE uses libxml2 primarily through `xmlTextReader*` (streaming reader) APIs
- C14N canonicalization used for XML digital signatures (`xmlC14NExecute`)
- The CXmlWriter wrapper is pure C++ — no libxml2 dependency
- 10 feature flags enabled, ~14 disabled (no threads, no FTP/HTTP, no iconv/ICU, no schemas)

### Risk Assessment for 2.12.x Update
- HIGH: SAX1 deprecated, thread-safety model changed, C14N API may differ
- MEDIUM: error.c patch location, build file updates, unity build regeneration
- LOW: Wrapper code is well-abstracted and thin

### Update Strategy
- Phase 1: Verify C14N API compatibility, check SAX1 deprecation impact
- Phase 2: Drop-in replace source, re-apply error.c patch, fix xmlversion.h
- Phase 3: Test all build targets (Linux, Windows, macOS, WASM)

## Task 13: Secure Random for GUID Generation

- Only project-owned file with std::rand()/srand() was OOXML/Base/Unit.cpp
- FontOTWriter.cpp had bare rand() for Type2 charstring random operand
- All other rand()/srand() hits were vendored (openjpeg, LeptonLib, zlib, agg)
- getrandom() syscall available since Linux 3.17, needs <sys/random.h>
- Must handle EINTR in getrandom() retry loop
- /dev/urandom is appropriate fallback (non-blocking, cryptographically secure)
- Rand() was only used internally by GenerateInt()/GenerateGuid() — safe to make static/remove
- UUID v4 format: version nibble=0x4 at byte[6], variant bits=10x at byte[8]
