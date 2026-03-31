# Security & Optimization Audit — ONLYOFFICE Core

## TL;DR

> **Quick Summary**: Comprehensive security hardening, performance optimization, test infrastructure build-out, and vendored library updates for the ONLYOFFICE Core C++17 document conversion engine. Fixes critical SSL verification bypass, addresses command injection and SSRF vectors, optimizes hot-path data structures and string operations, establishes ASAN/UBSAN testing, and updates vulnerable vendored dependencies.
> 
> **Deliverables**:
> - SSL/TLS verification re-enabled on Linux with configurable CA bundle
> - SSRF protection via URL scheme/IP whitelist in FileTransporter
> - Command injection fixes in project code (MemoryLimit test, vboxtester)
> - Secure password handling (stdin alternative to CLI args)
> - Secure GUID generation replacing rand() in OOXML module
> - Font cache bounded with sensible default (was unlimited)
> - Hot-path std::map → std::unordered_map (~20 files)
> - SVG parser string building optimized
> - boost::lexical_cast → std::to_wstring/std::from_chars in hot paths
> - RAII conversion in crypto and binary parsing code
> - ASAN/UBSAN CMake build option with suppression files
> - GoogleTest integration and initial test suite
> - Vendored lib updates: zlib 1.3.2, OpenJPEG 2.5.4, libxml2 2.12.x
> - Large file splits for top ~10 project files
> - Coverage reporting and expanded CI pipeline
> 
> **Estimated Effort**: XL (40-60 tasks across 6 waves)
> **Parallel Execution**: YES - 6 waves
> **Critical Path**: Wave 1 (Foundation) → Wave 2 (Critical Security) → Wave 3 (Vendored Libs) → Wave 4 (Performance) → Wave 5 (Code Health) → Wave FINAL (Verification)

---

## Context

### Original Request
User requested a security inspection and optimization plan for the ONLYOFFICE Core codebase at /opt/git/core.

### Interview Summary
**Key Discussions**:
- Codebase is C++17 document conversion engine (NOT a web app) — security/optimization patterns differ from web applications
- User chose: BOTH security AND performance fixes + test infrastructure
- Vendored libraries: YES — update zlib, OpenJPEG, libxml2
- Third-party code: NO — leave LeptonLib/OpenJPEG vendored issues as-is
- Large file splits: YES — split project files over 5K lines
- Test infrastructure: YES — ASAN/UBSAN, coverage, expanded CI

**Research Findings**:
- SSL verification completely disabled on Linux (HIGH)
- ~1,148 std::map instances across ~310 files (not "20+" as initially estimated)
- libxml2 is 2.9.2 from 2014, heavily customized fork
- OpenSSL 1.1.1f is EOL (flagged but explicitly out of scope)
- Zero test coverage infrastructure, no ASAN/UBSAN, no fuzzing
- Three separate copies of zlib in the tree

### Metis Review
**Identified Gaps** (addressed):
- std::map scale massively underestimated (1,148 vs "20+") → Plan targets ~20 hot-path files, rest deferred
- SHA1/MD5 in OOXML/ODF encryption is spec-required → Plan excludes changing defaults, only adds warnings
- libxml2 is a customized fork → Plan includes audit step before update
- OpenSSL EOL not in scope → Explicitly documented as out-of-scope exclusion
- No performance baselines exist → Plan establishes timing baselines in Wave 1
- ASAN will surface hundreds of existing bugs → Plan starts non-blocking, with suppression files
- Three zlib copies must be updated atomically → Plan consolidates in single task
- `const_cast` on `c_str()` in mkstemp() is UB → Fixed as part of SSL task
- WASM compatibility must be preserved → All platform-conditional code uses existing `#if defined()` patterns

---

## Work Objectives

### Core Objective
Harden the ONLYOFFICE Core conversion engine against known security vulnerabilities, improve conversion throughput through data structure and algorithm optimization, and establish a modern test infrastructure with sanitizer support and coverage reporting.

### Concrete Deliverables
- `FileTransporter_curl.cpp` with SSL verification enabled + configurable CA bundle
- URL whitelist/blacklist system in FileTransporter for SSRF protection
- `MemoryLimit/ParentProcess/main.cpp` with input validation
- `vboxtester/main.cpp` with execve() replacing popen()
- `ooxml_crypt/main.cpp` with stdin password reading option
- `OOXML/Base/Unit.cpp` with cryptographic random for GUIDs
- `FontManager.cpp/h` with bounded default cache size (32 fonts)
- ~20 hot-path files with std::unordered_map replacing std::map
- `svg_parser.cpp` with optimized string building
- `CMakeLists.txt` + `common.cmake` with `-DENABLE_SANITIZERS` option
- GoogleTest integrated via `Common/3dParty/googletest/`
- Updated vendored: zlib 1.3.2, OpenJPEG 2.5.4, libxml2 2.12.x
- ~10 large project files split into logical modules
- Coverage reporting via lcov/gcovr in CI

### Definition of Done
- [ ] `cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh` passes
- [ ] `cmake -DENABLE_SANITIZERS=ON -B build-san && cmake --build build-san` succeeds
- [ ] `curl -v https://example.com` from x2t succeeds with SSL verification
- [ ] No new compiler warnings from vendored lib updates
- [ ] All file splits produce identical `.o` symbol tables

### Must Have
- SSL verification enabled on Linux with graceful fallback
- SSRF URL whitelist configurable at runtime
- ASAN/UBSAN build option working in both CI and local dev
- All three zlib copies updated atomically
- OpenJPEG built with `OPJ_USE_SYSTEM_LIBS=ON`
- libxml2 customization audit documented before update
- Existing conversion test passes after every change
- Font cache has bounded default size
- SHA1/MD5 kept as defaults for OOXML/ODF compatibility

### Must NOT Have (Guardrails)
- MUST NOT change OOXML/ODF encryption default algorithms (SHA1/MD5 are spec-required)
- MUST NOT break V8 build (requires clang-13 via GN)
- MUST NOT break WASM/Emscripten build path
- MUST NOT modify vendored LeptonLib or OpenJPEG source code
- MUST NOT change public API of conversion engine (x2t binary interface)
- MUST NOT introduce new external dependencies
- MUST NOT update OpenSSL, libtiff, or JasPer (explicitly out of scope)
- MUST NOT batch all 1,148 std::map replacements — target ~20 hot-path files only
- MUST NOT convert all 600+ new[] to RAII — target crypto and binary parsing only
- MUST NOT introduce async I/O (out of scope)
- MUST NOT change conversion test snapshot baselines
- MUST NOT remove `-O2` from default flags

---

## Verification Strategy

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed. No exceptions.

### Test Decision
- **Infrastructure exists**: PARTIAL — gtest placeholder exists but empty, no CI test framework
- **Automated tests**: Tests-after (establish infra first, then add tests)
- **Framework**: GoogleTest (via `Common/3dParty/googletest/`)
- **If TDD**: N/A — infrastructure must be built first

### QA Policy
Every task MUST include agent-executed QA scenarios.
Evidence saved to `.sisyphus/evidence/task-{N}-{scenario-slug}.{ext}`.

- **C++ Build/Compile**: Use Bash (cmake --build) — Build, check for warnings/errors, run tests
- **Library Updates**: Use Bash (cmake --build + ctest + conversionTest.sh) — Verify build and conversion
- **Security Fixes**: Use Bash (curl, strace) — Verify SSL handshake, test URL blocking, test input sanitization
- **Performance**: Use Bash (time, /usr/bin/time -v) — Measure conversion time and memory before/after
- **File Splits**: Use Bash (nm, diff) — Compare symbol tables before/after

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation — test infra + baselines):
├── Task 1: GoogleTest setup in Common/3dParty/googletest/ [quick]
├── Task 2: ASAN/UBSAN CMake option in common.cmake [quick]
├── Task 3: ASAN/UBSAN suppression files for vendored code [quick]
├── Task 4: ASAN/UBSAN baseline error count [quick]
├── Task 5: Performance baseline timing measurements [quick]
├── Task 6: libxml2 ONLYOFFICE customization audit [unspecified-high]
└── Task 7: Coverage reporting setup (lcov/gcovr) [quick]

Wave 2 (Critical Security — HIGH priority):
├── Task 8: SSL verification fix + configurable CA bundle [deep]
├── Task 9: SSRF URL whitelist in FileTransporter [deep]
├── Task 10: Command injection fix in MemoryLimit test [quick]
├── Task 11: Command injection fix in vboxtester [quick]
├── Task 12: Password stdin alternative for ooxml_crypt [quick]
├── Task 13: Secure GUID generation in OOXML/Base/Unit.cpp [quick]
├── Task 14: mkstemp() const_cast UB fix in FileTransporter [quick]
└── Task 15: Certificate password storage audit in xmlsec [quick]

Wave 3 (Vendored Library Updates):
├── Task 16: zlib 1.2.11 → 1.3.2 (all 3 copies atomically) [deep]
├── Task 17: OpenJPEG 2.4.0 → 2.5.4 with OPJ_USE_SYSTEM_LIBS [deep]
├── Task 18: libxml2 2.9.2 → 2.12.x (after audit from Task 6) [deep]
└── Task 19: Conversion regression test after vendored updates [unspecified-high]

Wave 4 (High-Impact Performance):
├── Task 20: std::map → std::unordered_map in ODF context (~10 files) [unspecified-high]
├── Task 21: std::map → std::unordered_map in OOXML context (~10 files) [unspecified-high]
├── Task 22: SVG parser string building optimization [quick]
├── Task 23: boost::lexical_cast → std::to_wstring in hot paths (~15 files) [unspecified-high]
├── Task 24: Font cache bounded default size [quick]
├── Task 25: String .reserve() additions in serialization hot paths [unspecified-high]
└── Task 26: Double lookup .count()+.at() → .find() pattern [quick]

Wave 5 (Code Health — RAII + File Splits + Remaining):
├── Task 27: RAII conversion in OfficeCryptReader (ECMACryptFile) [unspecified-high]
├── Task 28: RAII conversion in OdfFile (odf_document_impl, draw_frame) [unspecified-high]
├── Task 29: RAII conversion in X2tConverter (ASCConverters) [quick]
├── Task 30: Split ChartFromToBinary.cpp (13K lines) [unspecified-high]
├── Task 31: Split ChartSerialize.cpp (11K lines) [unspecified-high]
├── Task 32: Split BinaryReaderD.cpp + BinaryWriterD.cpp (11K + 10K) [unspecified-high]
├── Task 33: Split BinaryReaderS.cpp + BinaryWriterS.cpp (10K + 9K) [unspecified-high]
├── Task 34: Split Pivots.cpp (8.4K lines) [unspecified-high]
└── Task 35: Additional smaller file splits (5-8K range) [unspecified-high]

Wave FINAL (Verification — 4 parallel reviews):
├── Task F1: Plan compliance audit (oracle)
├── Task F2: Build + ASAN + conversion test review (unspecified-high)
├── Task F3: Real manual QA — all scenarios executed (unspecified-high)
└── Task F4: Scope fidelity check (deep)
-> Present results -> Get explicit user okay

Critical Path: T1 → T2 → T4 → T8 → T16 → T18 → T20 → T22 → T27 → T30 → F1-F4
Parallel Speedup: ~65% faster than sequential
Max Concurrent: 7 (Wave 1), 8 (Wave 2), 4 (Wave 3), 7 (Wave 4), 9 (Wave 5)
```

### Dependency Matrix

| Task | Depends On | Blocks | Wave |
|------|-----------|--------|------|
| 1 | — | 2, 7 | 1 |
| 2 | — | 3, 4 | 1 |
| 3 | — | 4 | 1 |
| 4 | 2, 3 | F2 | 1 |
| 5 | — | 22-26 | 1 |
| 6 | — | 18 | 1 |
| 7 | 1 | F2 | 1 |
| 8 | — | 19 | 2 |
| 9 | — | 19 | 2 |
| 10 | — | — | 2 |
| 11 | — | — | 2 |
| 12 | — | — | 2 |
| 13 | — | — | 2 |
| 14 | — | 8 | 2 |
| 15 | — | — | 2 |
| 16 | 1 | 17, 19 | 3 |
| 17 | 16 | 19 | 3 |
| 18 | 6 | 19 | 3 |
| 19 | 8, 9, 16, 17, 18 | F2 | 3 |
| 20 | — | — | 4 |
| 21 | — | — | 4 |
| 22 | 5 | — | 4 |
| 23 | — | — | 4 |
| 24 | — | — | 4 |
| 25 | — | — | 4 |
| 26 | — | — | 4 |
| 27 | — | — | 5 |
| 28 | — | — | 5 |
| 29 | — | — | 5 |
| 30 | — | — | 5 |
| 31 | — | — | 5 |
| 32 | — | — | 5 |
| 33 | — | — | 5 |
| 34 | — | — | 5 |
| 35 | — | — | 5 |
| F1 | ALL | — | FINAL |
| F2 | 4, 7, 19 | — | FINAL |
| F3 | ALL | — | FINAL |
| F4 | ALL | — | FINAL |

### Agent Dispatch Summary

- **Wave 1**: 7 tasks — T1-T3,T6-T7 → `quick`, T4-T5 → `quick`
- **Wave 2**: 8 tasks — T8-T9 → `deep`, T10-T15 → `quick`
- **Wave 3**: 4 tasks — T16-T18 → `deep`, T19 → `unspecified-high`
- **Wave 4**: 7 tasks — T20-T21,T23,T25 → `unspecified-high`, T22,T24,T26 → `quick`
- **Wave 5**: 9 tasks — T27-T28,T30-T35 → `unspecified-high`, T29 → `quick`
- **FINAL**: 4 tasks — F1 → `oracle`, F2-F3 → `unspecified-high`, F4 → `deep`

---

## TODOs

- [x] 1. Integrate GoogleTest Framework

  **What to do**:
  - Download and integrate GoogleTest source into `Common/3dParty/googletest/` (currently only contains `.gitignore`)
  - Create `Common/3dParty/googletest/CMakeLists.txt` that builds gtest as a static library
  - Add `add_subdirectory` in the root or appropriate parent CMakeLists.txt
  - Create a minimal smoke test to verify gtest integration works: a single `TEST(SmokeTest, Passes)` that asserts `true`
  - Wire `enable_testing()` and `add_test()` into the CMake configuration
  - Verify `ctest` discovers and runs the smoke test

  **Must NOT do**:
  - Do NOT add any project-specific tests yet (that comes later)
  - Do NOT modify existing test applications (x2tTester, StandardTester, etc.)
  - Do NOT change the existing conversion test workflow

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Well-defined setup task, following established CMake patterns
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - `test-driven-development`: Not applicable — setting up infrastructure, not writing tests yet

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 2-7)
  - **Blocks**: Task 7 (coverage depends on gtest)
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `Common/3dParty/googletest/.gitignore` — Confirms directory exists but is empty (only gitignore)
  - `common.cmake:88-122` — `set_default_options()` function pattern for CMake target configuration
  - `CMakeLists.txt:18-27` — Pattern for adding subdirectories

  **API/Type References**:
  - GoogleTest CMake integration docs: `https://google.github.io/googletest/quickstart-cmake.html`

  **WHY Each Reference Matters**:
  - The `.gitignore` confirms the intended location for gtest source
  - `set_default_options()` shows how to apply compiler flags to new targets consistently
  - Root CMakeLists.txt shows the subdirectory inclusion pattern

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: GoogleTest builds and smoke test passes
    Tool: Bash
    Preconditions: Clean build directory
    Steps:
      1. cmake -GNinja -B /tmp/gtest-test /opt/git/core
      2. cmake --build /tmp/gtest-test --target gtest
      3. cmake --build /tmp/gtest-test
      4. ctest --test-dir /tmp/gtest-test --output-on-failure
    Expected Result: All 3 commands succeed, smoke test PASSES
    Failure Indicators: Build errors, ctest finds 0 tests, smoke test FAILS
    Evidence: .sisyphus/evidence/task-1-gtest-smoke.txt

  Scenario: GoogleTest does not break existing build
    Tool: Bash
    Preconditions: gtest integrated
    Steps:
      1. cmake -GNinja -B /tmp/gtest-full /opt/git/core
      2. cmake --build /tmp/gtest-full 2>&1 | tee /tmp/build_output.txt
      3. grep -i "error" /tmp/build_output.txt | grep -v "0 errors" | head -5
    Expected Result: Step 3 returns no output (zero new errors)
    Failure Indicators: New compiler errors not present before integration
    Evidence: .sisyphus/evidence/task-1-no-regression.txt
  ```

  **Commit**: YES
  - Message: `chore(test): integrate GoogleTest framework`
  - Files: `Common/3dParty/googletest/CMakeLists.txt`, `Common/3dParty/googletest/test/smoke_test.cpp`
  - Pre-commit: `cmake --build build && ctest --output-on-failure`

- [x] 2. Add ASAN/UBSAN CMake Build Option

  **What to do**:
  - Add `ENABLE_SANITIZERS` CMake option to `common.cmake` (OFF by default)
  - When enabled, add `-fsanitize=address,undefined -fno-sanitize-recover=all -fsanitize-minimal-runtime` to compile and link flags
  - Add `-DENABLE_SANITIZERS=ON` guard to set sanitizer-specific flags:
    - `-fsanitize=address` (ASAN) — detects memory errors
    - `-fsanitize=undefined` (UBSAN) — detects undefined behavior
    - `-fno-sanitize-recover=all` — abort on first sanitizer error
    - `-fsanitize-minimal-runtime` — smaller binary size
  - Ensure flags are added to both `CMAKE_CXX_FLAGS` and `CMAKE_C_FLAGS` and `CMAKE_EXE_LINKER_FLAGS`
  - Add `-g` flag when sanitizers are enabled (for better stack traces)
  - Preserve existing `-O2` flag (sanitizers work at any optimization level)
  - Document the option in a comment near the CMake option declaration

  **Must NOT do**:
  - Do NOT enable sanitizers by default
  - Do NOT remove `-O2` flag
  - Do NOT change any existing compiler flags
  - Do NOT add sanitizer flags to vendored library builds (those have their own CMakeLists.txt)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Single-file CMake configuration change
  - **Skills**: []
  - **Skills Evaluated but Omitted**:
    - `verification-before-completion`: Will verify build manually

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1, 3-7)
  - **Blocks**: Task 4 (baseline measurement needs sanitizer build)
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `common.cmake:1-6` — CMake guard, project definition, C++17 standard setting
  - `common.cmake:55-66` — `COMMON_CXX_FLAGS` definition (where to add sanitizer flags)
  - `common.cmake:68-80` — `COMMON_C_FLAGS` definition (where to add sanitizer flags)
  - `common.cmake:83-85` — `COMMON_LINK_OPTIONS` (where to add linker sanitizer flags)
  - `common.cmake:88-122` — `set_default_options()` function (apply sanitizer flags per-target)

  **External References**:
  - Clang ASAN docs: `https://clang.llvm.org/docs/AddressSanitizer.html`
  - Clang UBSAN docs: `https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html`

  **WHY Each Reference Matters**:
  - `COMMON_CXX_FLAGS` at line 55-66 shows exactly where sanitizer flags must be appended
  - `COMMON_LINK_OPTIONS` shows linker flags must also include sanitizers
  - `set_default_options()` is the per-target application point

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Sanitizer build succeeds
    Tool: Bash
    Preconditions: Clean build
    Steps:
      1. cmake -GNinja -B /tmp/san-build /opt/git/core -DENABLE_SANITIZERS=ON
      2. cmake --build /tmp/san-build 2>&1 | tail -20
      3. echo $?
    Expected Result: Build succeeds (exit code 0)
    Failure Indicators: CMake configure error, linker error about missing sanitizer runtime
    Evidence: .sisyphus/evidence/task-2-sanitizer-build.txt

  Scenario: Sanitizer build detects intentional UB (smoke test)
    Tool: Bash
    Preconditions: Sanitizer build available
    Steps:
      1. cat > /tmp/ub_test.cpp << 'EOF'
         #include <cstdlib>
         int main() { int x = 10; return x / 0; }
         EOF
      2. clang++ -fsanitize=undefined -fno-sanitize-recover=all /tmp/ub_test.cpp -o /tmp/ub_test 2>&1
      3. /tmp/ub_test 2>&1; echo "exit: $?"
    Expected Result: Program aborts with UBSAN error message, exit code non-zero
    Failure Indicators: Program runs without aborting
    Evidence: .sisyphus/evidence/task-2-ub-detection.txt

  Scenario: Default build (without sanitizers) is unaffected
    Tool: Bash
    Preconditions: Sanitizer option added
    Steps:
      1. cmake -GNinja -B /tmp/default-build /opt/git/core
      2. cmake --build /tmp/default-build 2>&1 | tail -5
      3. nm /tmp/default-build/package/x2t 2>/dev/null | grep -i "asan" | head -3
    Expected Result: Build succeeds, no ASAN symbols in binary
    Failure Indicators: ASAN symbols found in default build
    Evidence: .sisyphus/evidence/task-2-default-unaffected.txt
  ```

  **Commit**: YES
  - Message: `build(sanitizers): add ENABLE_SANITIZERS CMake option`
  - Files: `common.cmake`
  - Pre-commit: `cmake -GNinja -B build-san -DENABLE_SANITIZERS=ON && cmake --build build-san`

- [x] 3. Create ASAN/UBSAN Suppression Files for Vendored Code

  **What to do**:
  - Create suppression file at `.sisyphus/suppressions/asan.supp` for known vendored code ASAN false positives
  - Create suppression file at `.sisyphus/suppressions/ubsan.supp` for known vendored code UBSAN false positives
  - Configure CMake to pass suppression files via `ASAN_OPTIONS` and `UBSAN_OPTIONS` environment variables or `LSAN_OPTIONS`
  - Known suppressions needed:
    - Crypto++ hash functions (deliberate signed integer overflow in hash computations)
    - libxml2 memory management patterns (known leaks in parser cleanup)
    - freetype glyph loading (known buffer overread in some font formats)
    - OpenJPEG JP2 decoding (known issues with certain color space conversions)
  - Add `ASAN_OPTIONS=detect_leaks=1:suppressions=.sisyphus/suppressions/asan.supp` to test environment
  - Add `UBSAN_OPTIONS=suppressions=.sisyphus/suppressions/ubsan.supp:print_stacktrace=1` to test environment

  **Must NOT do**:
  - Do NOT suppress errors in project-owned code (Common/, OOXML/, OdfFile/, etc.)
  - Do NOT suppress ALL errors from any vendored library — only specific known false positives
  - Do NOT use `intercept_*` suppressions that hide real issues

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Creating config files with known patterns
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-2, 4-7)
  - **Blocks**: Task 4 (baseline needs suppressions to avoid noise)
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `common.cmake:55-66` — Where sanitizer flags are set (pass suppression path here)
  - `.github/workflows/build.yml` — CI workflow (where to set ASAN/UBSAN_OPTIONS env vars)

  **External References**:
  - ASAN suppression format: `https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer#suppressions`
  - UBSAN suppression format: `https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html#suppressions`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Suppression files are valid and parseable
    Tool: Bash
    Preconditions: Suppression files created
    Steps:
      1. cat /opt/git/core/.sisyphus/suppressions/asan.supp
      2. cat /opt/git/core/.sisyphus/suppressions/ubsan.supp
      3. grep -c "^" /opt/git/core/.sisyphus/suppressions/asan.supp
      4. grep -c "^" /opt/git/core/.sisyphus/suppressions/ubsan.supp
    Expected Result: Both files exist, are non-empty, contain valid suppression entries
    Failure Indicators: Files empty or contain syntax errors
    Evidence: .sisyphus/evidence/task-3-suppressions.txt
  ```

  **Commit**: YES
  - Message: `build(sanitizers): add ASAN/UBSAN suppression files`
  - Files: `.sisyphus/suppressions/asan.supp`, `.sisyphus/suppressions/ubsan.supp`, `common.cmake`

- [x] 4. Establish ASAN/UBSAN Baseline Error Count

  **What to do**:
  - Build the project with `cmake -DENABLE_SANITIZERS=ON`
  - Run the conversion test suite (`./Test/Applications/x2tTester/conversionTest.sh`) under ASAN/UBSAN
  - Capture and categorize ALL sanitizer errors:
    - Count total ASAN errors (heap-buffer-overflow, use-after-free, memory-leak, etc.)
    - Count total UBSAN errors (signed-integer-overflow, shift-exponent, null-pointer, etc.)
    - Categorize by: vendored code vs project code
    - Categorize by: file and function
  - Save baseline report to `.sisyphus/baselines/sanitizer-baseline.txt`
  - This baseline is used to measure improvement — future fixes should REDUCE this count

  **Must NOT do**:
  - Do NOT fix any errors found (this is measurement only)
  - Do NOT suppress errors that are real bugs in project code
  - Do NOT skip this step — it's critical for measuring progress

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Running builds and capturing output
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Tasks 2, 3)
  - **Parallel Group**: Wave 1 (but sequential after T2, T3)
  - **Blocks**: Task F2 (final verification compares against baseline)
  - **Blocked By**: Task 2, Task 3

  **References**:

  **Pattern References**:
  - `common.cmake` — Sanitizer build configuration
  - `Test/Applications/x2tTester/conversionTest.sh` — Conversion test to run under sanitizers
  - `.sisyphus/suppressions/asan.supp` — Suppression files to use during baseline

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Baseline report generated with categorized errors
    Tool: Bash
    Preconditions: ASAN build available (Task 2), suppressions available (Task 3)
    Steps:
      1. cmake -GNinja -B /tmp/baseline-san /opt/git/core -DENABLE_SANITIZERS=ON
      2. cmake --build /tmp/baseline-san
      3. ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1 ./Test/Applications/x2tTester/conversionTest.sh 2>&1 | tee /tmp/san_output.txt
      4. grep -c "ERROR:" /tmp/san_output.txt
      5. mkdir -p /opt/git/core/.sisyphus/baselines
      6. cp /tmp/san_output.txt /opt/git/core/.sisyphus/baselines/sanitizer-baseline.txt
    Expected Result: Baseline file created with categorized error counts
    Failure Indicators: Build fails, no errors captured (sanitizers not working)
    Evidence: .sisyphus/evidence/task-4-baseline.txt
  ```

  **Commit**: YES
  - Message: `test(sanitizers): establish ASAN/UBSAN baseline error count`
  - Files: `.sisyphus/baselines/sanitizer-baseline.txt`

- [x] 5. Establish Performance Timing Baselines

  **What to do**:
  - Select a small corpus of test documents (use existing test files from `Test/Applications/x2tTester/`)
  - Measure conversion time for each format: DOCX→PDF, XLSX→PDF, PPTX→PDF, ODT→DOCX, ODS→XLSX
  - Use `/usr/bin/time -v` to capture memory usage (max RSS) alongside wall-clock time
  - Run each conversion 3 times and record median
  - Save results to `.sisyphus/baselines/performance-baseline.txt`
  - This baseline is used to verify performance optimizations don't regress and improvements are measurable

  **Must NOT do**:
  - Do NOT modify any source code
  - Do NOT use documents not already in the repository
  - Do NOT skip this step — performance claims need measurable evidence

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Running timing benchmarks, no code changes
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-4, 6-7)
  - **Blocks**: Wave 4 performance tasks (need baselines to verify improvement)
  - **Blocked By**: None (but needs a working build)

  **References**:

  **Pattern References**:
  - `Test/Applications/x2tTester/conversionTest.sh` — Existing test documents and conversion commands
  - `X2tConverter/src/main.cpp` — Entry point for x2t converter
  - `X2tConverter/README.md` — XML configuration format for conversions

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Performance baseline captured
    Tool: Bash
    Preconditions: x2t binary built
    Steps:
      1. ls Test/Applications/x2tTester/data/  # List available test documents
      2. for i in 1 2 3; do /usr/bin/time -v ./build/package/x2t Test/Applications/x2tTester/data/simple.docx.pdf.xml 2>&1 | grep -E "(wall clock|Maximum resident)"; done
      3. mkdir -p /opt/git/core/.sisyphus/baselines
      4. Record results to .sisyphus/baselines/performance-baseline.txt
    Expected Result: Baseline file with timing and memory data for each conversion
    Failure Indicators: x2t binary not found, conversions fail
    Evidence: .sisyphus/evidence/task-5-perf-baseline.txt
  ```

  **Commit**: YES
  - Message: `perf(baseline): establish conversion timing baselines`
  - Files: `.sisyphus/baselines/performance-baseline.txt`

- [x] 6. Audit libxml2 ONLYOFFICE Customizations

  **What to do**:
  - Examine the vendored libxml2 at `DesktopEditor/xml/libxml2/` for ONLYOFFICE-specific modifications
  - Check `xmlversion.h` for template placeholders (`"@LIBXML_VERSION_NUMBER@"` etc.) — these indicate build-time customization
  - Search for ONLYOFFICE-specific patches by comparing key files against upstream libxml2 2.9.2
  - Check the CMakeLists.txt or build configuration for how libxml2 is built (compile flags, defines)
  - Document all findings in `.sisyphus/baselines/libxml2-audit.md`:
    - Custom compile flags and defines
    - Modified source files (if any)
    - Build system customizations
    - Dependencies on specific libxml2 behavior
    - Risk assessment for updating to 2.12.x
  - This audit is a prerequisite for Task 18 (libxml2 update)

  **Must NOT do**:
  - Do NOT modify any libxml2 source files
  - Do NOT update libxml2 yet (that's Task 18)
  - Do NOT make assumptions about what's customized — verify by reading the code

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Requires careful investigation of a large vendored library
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Tasks 1-5, 7)
  - **Blocks**: Task 18 (libxml2 update depends on audit)
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `DesktopEditor/xml/libxml2/` — Vendored libxml2 root directory
  - `DesktopEditor/xml/libxml2/include/libxml/xmlversion.h` — Template placeholders indicating customization
  - `DesktopEditor/xml/` — XML processing code that depends on libxml2

  **External References**:
  - libxml2 changelog: `https://gitlab.gnome.org/GNOME/libxml2/-/blob/master/NEWS`
  - libxml2 2.12.x migration guide: `https://gitlab.gnome.org/GNOME/libxml2/-/blob/master/README.md`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Audit document created with findings
    Tool: Bash
    Preconditions: None
    Steps:
      1. cat /opt/git/core/.sisyphus/baselines/libxml2-audit.md | head -50
      2. grep -c "## " /opt/git/core/.sisyphus/baselines/libxml2-audit.md
    Expected Result: Audit file exists with structured sections covering all areas
    Failure Indicators: File empty, missing sections
    Evidence: .sisyphus/evidence/task-6-libxml2-audit.txt
  ```

  **Commit**: YES
  - Message: `audit(libxml2): document ONLYOFFICE customizations`
  - Files: `.sisyphus/baselines/libxml2-audit.md`

- [x] 7. Setup Coverage Reporting (lcov/gcovr)

  **What to do**:
  - Add `ENABLE_COVERAGE` CMake option to `common.cmake` (OFF by default)
  - When enabled, add `--coverage -fprofile-arcs -ftest-coverage` to compile and link flags
  - Add `lgcov` to link libraries when coverage is enabled
  - Create a CI workflow step (or document) for generating coverage reports using `gcovr`
  - Configure gcovr to exclude vendored code (`3dParty/`, `libxml2/`, `freetype*`, `openjpeg*`, etc.)
  - Generate coverage in XML format for CI integration and HTML for human review
  - Add coverage target to CMake: `add_custom_target(coverage ...)` that runs gcovr

  **Must NOT do**:
  - Do NOT enable coverage by default (it adds significant overhead)
  - Do NOT use lcov (gcovr is simpler and language-agnostic)
  - Do NOT include vendored code in coverage metrics

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: CMake configuration + documentation
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES (but ideally after Task 1 for gtest tests to cover)
  - **Parallel Group**: Wave 1 (with Tasks 1-6)
  - **Blocks**: Task F2 (final verification checks coverage)
  - **Blocked By**: Task 1 (needs gtest for meaningful coverage data)

  **References**:

  **Pattern References**:
  - `common.cmake:55-66` — `COMMON_CXX_FLAGS` (add coverage flags here)
  - `common.cmake:88-122` — `set_default_options()` (apply coverage per-target)
  - `.github/workflows/build.yml` — CI workflow (where to add coverage step)

  **External References**:
  - gcovr documentation: `https://gcovr.com/en/stable/guide.html`
  - GCC coverage flags: `https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Coverage build succeeds and report generates
    Tool: Bash
    Preconditions: gtest integrated (Task 1)
    Steps:
      1. cmake -GNinja -B /tmp/cov-build /opt/git/core -DENABLE_COVERAGE=ON
      2. cmake --build /tmp/cov-build
      3. cmake --build /tmp/cov-build --target coverage 2>&1 | tail -10
      4. ls /tmp/cov-build/coverage.xml 2>/dev/null
    Expected Result: coverage.xml generated with non-zero coverage percentage
    Failure Indicators: Build fails, coverage target not found, empty coverage report
    Evidence: .sisyphus/evidence/task-7-coverage.txt
  ```

  **Commit**: YES
  - Message: `build(coverage): add lcov/gcovr coverage reporting`
  - Files: `common.cmake`, `.github/workflows/build.yml`

- [ ] 8. Enable SSL Verification with Configurable CA Bundle

  **What to do**:
  - In `Common/Network/FileTransporter/src/FileTransporter_curl.cpp`, REMOVE the `#if defined(__linux__)` block that sets `CURLOPT_SSL_VERIFYPEER` to 0 (lines 117-119 for downloads, 176-179 for uploads)
  - Enable SSL verification on ALL platforms: `CURLOPT_SSL_VERIFYPEER` = 1L, `CURLOPT_SSL_VERIFYHOST` = 2L
  - Add configurable CA bundle path support:
    - Check environment variable `SSL_CERT_FILE` first (standard OpenSSL convention)
    - Fall back to `SSL_CERT_DIR` if set
    - Fall back to system default paths: `/etc/ssl/certs/ca-certificates.crt` (Debian/Ubuntu), `/etc/pki/tls/cert.pem` (RHEL), `/etc/ssl/cert.pem` (Alpine)
    - Use `CURLOPT_CAINFO` to set the CA bundle path
  - Add graceful fallback: if CA bundle not found, LOG a warning but do NOT disable verification (let curl handle the error)
  - Fix the `const_cast` undefined behavior in `mkstemp()` call (line 223) — use a `char[]` buffer instead of `const_cast<char*>(sTempPath.c_str())`
  - Add similar fix to the upload function's temp file creation

  **Must NOT do**:
  - Do NOT hardcode a single CA bundle path
  - Do NOT silently disable SSL verification as fallback
  - Do NOT break the WASM build (WASM doesn't use this code path)
  - Do NOT change the external transport code path (`USE_EXTERNAL_TRANSPORT`)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Critical security fix requiring careful error handling and cross-platform considerations
  - **Skills**: [`systematic-debugging`]
    - `systematic-debugging`: SSL certificate verification involves complex error states

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 9-15)
  - **Blocks**: Task 19 (conversion regression test)
  - **Blocked By**: Task 14 (mkstemp fix should be part of this task)

  **References**:

  **Pattern References**:
  - `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:110-130` — Download function with SSL settings
  - `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:165-185` — Upload function with SSL settings
  - `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:221-223` — mkstemp() with const_cast UB
  - `Common/Network/FileTransporter/src/transport_external.h` — External transport (separate code path)
  - `DesktopEditor/common/File.cpp:1587` — Example of proper `mkstemp()` usage in codebase

  **External References**:
  - libcurl SSL verification: `https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYPEER.html`
  - libcurl CA bundle: `https://curl.se/libcurl/c/CURLOPT_CAINFO.html`
  - OpenSSL CA file locations: `https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_load_verify_locations.html`

  **WHY Each Reference Matters**:
  - Lines 117-119 show the exact code to remove/replace
  - Lines 221-223 show the UB that must be fixed alongside
  - `DesktopEditor/common/File.cpp:1587` shows the correct pattern for mkstemp usage

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: SSL verification enabled — HTTPS download succeeds
    Tool: Bash
    Preconditions: Build with SSL fix
    Steps:
      1. grep -n "CURLOPT_SSL_VERIFYPEER" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
      2. Verify no instance sets it to 0L
      3. grep -n "CURLOPT_SSL_VERIFYHOST" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
      4. Verify it is set to 2L
    Expected Result: VERIFYPEER=1L, VERIFYHOST=2L, no 0L values
    Failure Indicators: Any line still sets VERIFYPEER to 0
    Evidence: .sisyphus/evidence/task-8-ssl-enabled.txt

  Scenario: CA bundle path resolution works
    Tool: Bash
    Preconditions: Build with SSL fix
    Steps:
      1. grep -c "SSL_CERT_FILE\|CURLOPT_CAINFO\|ca-certificates" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
      2. Verify CA bundle path detection logic exists
    Expected Result: At least 3 references to CA bundle handling
    Failure Indicators: No CA bundle path logic found
    Evidence: .sisyphus/evidence/task-8-ca-bundle.txt

  Scenario: mkstemp UB fixed
    Tool: Bash
    Preconditions: Build with fix
    Steps:
      1. grep -n "const_cast" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
      2. Verify no const_cast on c_str() for mkstemp
    Expected Result: No const_cast on c_str() calls
    Failure Indicators: const_cast still present near mkstemp
    Evidence: .sisyphus/evidence/task-8-mkstemp-fix.txt

  Scenario: Conversion test still passes after SSL fix
    Tool: Bash
    Preconditions: Build with SSL fix
    Steps:
      1. ./Test/Applications/x2tTester/conversionTest.sh 2>&1 | tail -5
    Expected Result: All conversions PASS
    Failure Indicators: Any conversion FAIL
    Evidence: .sisyphus/evidence/task-8-conversion-test.txt
  ```

  **Commit**: YES
  - Message: `fix(security): enable SSL verification with configurable CA bundle`
  - Files: `Common/Network/FileTransporter/src/FileTransporter_curl.cpp`
  - Pre-commit: `cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh`

- [ ] 9. Add SSRF URL Whitelist to FileTransporter

  **What to do**:
  - In `Common/Network/FileTransporter/src/FileTransporter_curl.cpp`, add URL validation BEFORE `curl_easy_setopt(curl, CURLOPT_URL, url)`:
    - Parse the URL scheme — only allow `http://` and `https://`
    - Block `file://`, `ftp://`, `gopher://`, `dict://`, and other schemes
    - Resolve hostname and check against private IP ranges:
      - `10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16` (private)
      - `127.0.0.0/8` (loopback)
      - `169.254.0.0/16` (link-local / cloud metadata)
      - `::1`, `fc00::/7`, `fe80::/10` (IPv6 private)
      - `0.0.0.0/8` (current network)
    - Allow `localhost` hostname to be blocked via configuration
  - Make the URL whitelist configurable:
    - Environment variable `ALLOWED_URL_SCHEMES` (default: `http,https`)
    - Environment variable `BLOCK_PRIVATE_IPS` (default: `true`)
  - Apply the same validation to the upload path (`CFileUploader`)
  - Add validation to `transport_external.h` — the `wget_url_validate()` function needs the same private IP checks
  - Log blocked URLs at WARNING level for audit trail

  **Must NOT do**:
  - Do NOT use regex for URL parsing (use a proper URL parser or `curl_url` API)
  - Do NOT hardcode allowed domains (this is a deployment-specific decision)
  - Do NOT block all private IPs unconditionally (some deployments need internal network access)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Security-critical networking code, needs careful IP parsing and error handling
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8, 10-15)
  - **Blocks**: Task 19 (conversion regression test)
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:110-130` — Download with URL usage
  - `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:165-185` — Upload with URL usage
  - `Common/Network/FileTransporter/src/transport_external.h` — `wget_url_validate()` function
  - `Common/Network/FileTransporter/include/FileTransporter.h` — CFileDownloader/CFileUploader API

  **External References**:
  - Private IP ranges: RFC 1918, RFC 6598, RFC 3927
  - curl_url API: `https://curl.se/libcurl/c/curl_url.html`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Private IPs are blocked
    Tool: Bash
    Preconditions: Build with SSRF fix
    Steps:
      1. grep -c "127.0.0\|192.168\|10\.0\.\|169.254\|172.16" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
      2. Verify private IP range checks exist
    Expected Result: All 5 private IP ranges checked
    Failure Indicators: Missing any private IP range
    Evidence: .sisyphus/evidence/task-9-ssrf-private-ips.txt

  Scenario: URL scheme validation exists
    Tool: Bash
    Preconditions: Build with SSRF fix
    Steps:
      1. grep -c "file://\|ftp://\|scheme" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
      2. Verify scheme blocking logic exists
    Expected Result: Scheme validation code present
    Failure Indicators: No scheme checks found
    Evidence: .sisyphus/evidence/task-9-ssrf-schemes.txt

  Scenario: Configurable via environment variable
    Tool: Bash
    Preconditions: Build with SSRF fix
    Steps:
      1. grep -c "ALLOWED_URL_SCHEMES\|BLOCK_PRIVATE_IPS\|getenv" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
    Expected Result: Environment variable configuration present
    Evidence: .sisyphus/evidence/task-9-ssrf-config.txt
  ```

  **Commit**: YES
  - Message: `fix(security): add SSRF URL whitelist to FileTransporter`
  - Files: `Common/Network/FileTransporter/src/FileTransporter_curl.cpp`, `Common/Network/FileTransporter/src/transport_external.h`

- [ ] 10. Fix Command Injection in MemoryLimit ParentProcess

  **What to do**:
  - In `Test/Applications/MemoryLimit/ParentProcess/main.cpp:39`, the `argv[3]` argument is passed directly to `std::system()`
  - Add input validation: whitelist allowed commands or restrict to expected binary name pattern
  - At minimum: reject any argument containing shell metacharacters (`;`, `|`, `&`, `$`, `` ` ``, `(`, `)`, `<`, `>`, `\n`)
  - Better: use `execvp()` with explicit argument array instead of `system()` to avoid shell interpretation entirely
  - Log the validated command before execution for audit trail

  **Must NOT do**:
  - Do NOT remove the functionality (this is a test utility that needs to run child processes)
  - Do NOT use `system()` after fixing — use `execvp()` or `posix_spawn()`

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Single-file fix, well-understood pattern
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (with Tasks 8-9, 11-15)
  - **Blocks**: None
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `Test/Applications/MemoryLimit/ParentProcess/main.cpp:39` — The vulnerable `system()` call
  - `Test/Applications/MemoryLimit/ParentProcess/main.cpp:30-45` — Full context of argument handling

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: system() replaced with exec-style function
    Tool: Bash
    Preconditions: Fix applied
    Steps:
      1. grep -n "system(" Test/Applications/MemoryLimit/ParentProcess/main.cpp
      2. grep -n "execvp\|posix_spawn\|fork" Test/Applications/MemoryLimit/ParentProcess/main.cpp
    Expected Result: No system() calls, exec-style function present
    Failure Indicators: system() still present
    Evidence: .sisyphus/evidence/task-10-cmd-injection-fix.txt
  ```

  **Commit**: YES
  - Message: `fix(security): validate input in MemoryLimit ParentProcess`

- [ ] 11. Fix Command Injection in vboxtester

  **What to do**:
  - In `DesktopEditor/vboxtester/main.cpp:1255-1263`, string concatenation is used to build a command passed to `popen()`/`_wpopen()`
  - Replace `popen()` with `posix_spawn()` (Linux) / `CreateProcess()` (Windows) using explicit argument arrays
  - Ensure `sArgs` is passed as a separate argument, not concatenated into a shell command string
  - This prevents shell metacharacter injection via the arguments

  **Must NOT do**:
  - Do NOT remove the popen functionality (it's needed for capturing output)
  - Do NOT break the Windows build path (`_wpopen`)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Single-file fix, pattern is clear
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `DesktopEditor/vboxtester/main.cpp:1255-1263` — Vulnerable popen() with string concatenation
  - `DesktopEditor/vboxtester/main.cpp:1260` — Linux popen() call
  - `DesktopEditor/vboxtester/main.cpp:1255` — Windows _wpopen() call

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: popen() replaced with safe alternative
    Tool: Bash
    Preconditions: Fix applied
    Steps:
      1. grep -n "popen\|_wpopen" DesktopEditor/vboxtester/main.cpp
      2. grep -n "posix_spawn\|CreateProcess\|fork" DesktopEditor/vboxtester/main.cpp
    Expected Result: popen() removed or guarded, safe alternative present
    Evidence: .sisyphus/evidence/task-11-vboxtester-fix.txt
  ```

  **Commit**: YES
  - Message: `fix(security): replace popen() with execve() in vboxtester`

- [ ] 12. Add Stdin Password Alternative for ooxml_crypt

  **What to do**:
  - In `OfficeCryptReader/ooxml_crypt/main.cpp:292-294`, passwords are accepted via `--password=plaintext` CLI argument (visible in `/proc/*/cmdline`)
  - Add a `--password-file=/path/to/file` option that reads the password from a file
  - Add a `--password-stdin` option that reads the password from stdin (first line)
  - Keep the existing `--password=` option for backward compatibility (but document it as less secure)
  - Add a deprecation warning when `--password=` is used: "WARNING: Password visible in process listing. Use --password-file or --password-stdin instead."

  **Must NOT do**:
  - Do NOT remove `--password=` (backward compatibility)
  - Do NOT change the password format or encoding

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Adding CLI options, well-defined scope
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `OfficeCryptReader/ooxml_crypt/main.cpp:292-294` — Current password CLI argument handling
  - `X2tConverter/src/main.cpp:198-199` — Similar password handling in x2t (consider fixing there too if pattern matches)

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: --password-stdin option works
    Tool: Bash
    Preconditions: ooxml_crypt built
    Steps:
      1. echo "testpass" | ./build/package/ooxml_crypt --password-stdin --encrypt input.docx output.docx
      2. echo $?
    Expected Result: Command succeeds, file encrypted
    Failure Indicators: Unknown option error
    Evidence: .sisyphus/evidence/task-12-stdin-password.txt

  Scenario: --password= still works (backward compat)
    Tool: Bash
    Preconditions: ooxml_crypt built
    Steps:
      1. ./build/package/ooxml_crypt --password=testpass --encrypt input.docx output.docx 2>&1
      2. grep -c "WARNING" in output
    Expected Result: Command succeeds with deprecation warning
    Evidence: .sisyphus/evidence/task-12-backward-compat.txt
  ```

  **Commit**: YES
  - Message: `fix(security): add stdin password option to ooxml_crypt`

- [ ] 13. Replace rand() with Cryptographic Random for GUID Generation

  **What to do**:
  - In `OOXML/Base/Unit.cpp:756-781`, GUIDs are generated using `srand(time(NULL))` + `std::rand()` — predictable output
  - Replace with a cryptographic random source:
    - On Linux: use `getrandom()` syscall (available since Linux 3.17)
    - Fallback: use OpenSSL `RAND_bytes()` (already a dependency via `Common/3dParty/openssl/`)
    - Last resort: use `/dev/urandom`
  - Create a helper function `GenerateSecureRandomBytes(unsigned char* buf, size_t len)` in a common location
  - Use this helper for GUID generation instead of `std::rand()`
  - Also check and fix `PdfFile/SrcWriter/FontOTWriter.cpp:3665` which uses `rand()` for font values

  **Must NOT do**:
  - Do NOT use `std::random_device` (implementation-defined quality, may fall back to rand on some platforms)
  - Do NOT change the GUID format or structure — only the randomness source

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Well-defined replacement, clear target functions
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `OOXML/Base/Unit.cpp:756-781` — GUID generation with `srand(time(NULL))` + `std::rand()`
  - `PdfFile/SrcWriter/FontOTWriter.cpp:3665` — `rand()` for font values
  - `Common/3dParty/cryptopp/osrng.h` — Crypto++ OS random number generator (alternative)

  **External References**:
  - `getrandom()` syscall: `https://man7.org/linux/man-pages/man2/getrandom.2.html`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: rand() removed from GUID generation
    Tool: Bash
    Preconditions: Fix applied
    Steps:
      1. grep -n "std::rand()\|srand(" OOXML/Base/Unit.cpp
      2. grep -n "getrandom\|RAND_bytes\|/dev/urandom" OOXML/Base/Unit.cpp
    Expected Result: No std::rand()/srand() in Unit.cpp, secure alternative present
    Evidence: .sisyphus/evidence/task-13-secure-random.txt
  ```

  **Commit**: YES
  - Message: `fix(security): replace rand() with crypto random for GUIDs`

- [ ] 14. Fix mkstemp() Undefined Behavior in FileTransporter

  **What to do**:
  - In `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:221-223`, `mkstemp(const_cast<char*>(sTempPath.c_str()))` modifies a const string's internal buffer — this is undefined behavior in C++11+
  - Replace with a proper `char[]` buffer:
    ```cpp
    char szTempPath[PATH_MAX];
    snprintf(szTempPath, sizeof(szTempPath), "%s/fileXXXXXX", sTempPath.c_str());
    int fd = mkstemp(szTempPath);
    ```
  - Apply the same fix to the upload function if it has a similar pattern

  **Must NOT do**:
  - Do NOT use `std::string::data()` (still UB if const in C++11)
  - Do NOT use `tmpnam()` (insecure, creates name without creating file — race condition)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Small, well-defined fix
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 8 (should be done as part of SSL fix, but can be separate)
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `Common/Network/FileTransporter/src/FileTransporter_curl.cpp:221-223` — UB mkstemp call
  - `DesktopEditor/common/File.cpp:1587` — Correct mkstemp usage in codebase

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: const_cast removed from mkstemp calls
    Tool: Bash
    Preconditions: Fix applied
    Steps:
      1. grep -n "const_cast" Common/Network/FileTransporter/src/FileTransporter_curl.cpp
    Expected Result: No const_cast near mkstemp
    Evidence: .sisyphus/evidence/task-14-mkstemp-ub-fix.txt
  ```

  **Commit**: YES (groups with Task 8 if done together, otherwise separate)
  - Message: `fix(security): fix mkstemp() undefined behavior`

- [ ] 15. Audit Certificate Password Storage in xmlsec

  **What to do**:
  - Review `DesktopEditor/xmlsec/src/src/Certificate_openssl.h:1029-1035` where certificate passwords are concatenated into `m_id` string in plaintext
  - Assess whether this is a real security risk (who has access to `m_id`? Is it logged? Serialized? Sent over network?)
  - If the password is only held in memory for the lifetime of the certificate object and never persisted/logged, the risk is LOW — document this
  - If the password IS persisted, logged, or transmitted, fix it:
    - Use `SecureString` or zero-on-free buffer pattern
    - Clear the password from memory after use (`explicit_bzero` or `OPENSSL_cleanse`)
  - Add `OPENSSL_cleanse()` calls to clear password memory when certificate object is destroyed
  - Document findings in a comment near the password handling code

  **Must NOT do**:
  - Do NOT introduce a new `SecureString` class (too much scope)
  - Do NOT change the certificate loading API (backward compatibility)
  - Do NOT fix this if it's only in-memory with no persistence (document instead)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Audit and documentation task, potential small fix
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2
  - **Blocks**: None
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `DesktopEditor/xmlsec/src/src/Certificate_openssl.h:1029-1035` — Plaintext password in m_id
  - `DesktopEditor/xmlsec/src/src/Certificate_openssl.h` — Full certificate class (check destructor)

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Password memory cleanup added or documented as low-risk
    Tool: Bash
    Preconditions: Audit complete
    Steps:
      1. grep -n "OPENSSL_cleanse\|explicit_bzero\|secure_string" DesktopEditor/xmlsec/src/src/Certificate_openssl.h
      2. grep -c "TODO\|FIXME\|NOTE\|WARNING.*password" DesktopEditor/xmlsec/src/src/Certificate_openssl.h
    Expected Result: Either cleanup code present or documented as acceptable risk
    Evidence: .sisyphus/evidence/task-15-cert-password-audit.txt
  ```

  **Commit**: YES
  - Message: `fix(security): audit certificate password storage in xmlsec`

- [ ] 16. Update zlib from 1.2.11 to 1.3.2 (All Three Copies Atomically)

  **What to do**:
  - Identify all three zlib copies in the tree:
    - `OfficeUtils/src/zlib-1.2.11/` — Primary copy used by OfficeUtils
    - `cximage/zlib/` — May be a redirect or separate copy
    - OpenJPEG's bundled thirdparty libz (inside `DesktopEditor/raster/Jp2/openjpeg/openjpeg-2.4.0/thirdparty/libz/`)
  - Download zlib 1.3.2 source
  - Replace each copy's source files with the 1.3.2 version
  - Update CMakeLists.txt files that reference zlib paths (version number in directory names)
  - Key API changes to watch for: `z_off_t` is now 64-bit by default (was 32-bit on some platforms)
  - Ensure all three copies are updated in a SINGLE commit (atomic update)
  - Build and run conversion test to verify no regressions

  **Must NOT do**:
  - Do NOT update copies separately (all three in one commit)
  - Do NOT change zlib API usage in project code (zlib 1.3.2 is backward-compatible)
  - Do NOT update OpenJPEG's bundled zlib if `OPJ_USE_SYSTEM_LIBS` is set (Task 17 handles this)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Multi-location update requiring careful verification of all integration points
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 1 for gtest, blocks Task 17)
  - **Parallel Group**: Wave 3 (sequential)
  - **Blocks**: Task 17, Task 19
  - **Blocked By**: Task 1

  **References**:

  **Pattern References**:
  - `OfficeUtils/src/zlib-1.2.11/` — Primary zlib location
  - `OfficeUtils/CMakeLists.txt` — How zlib is referenced in build
  - `DesktopEditor/raster/Jp2/openjpeg/openjpeg-2.4.0/thirdparty/libz/` — OpenJPEG's bundled zlib

  **External References**:
  - zlib 1.3.2 changelog: `https://zlib.net/ChangeLog.html`
  - zlib 1.2.11→1.3.x migration: `z_off_t` now `z_size_t` in some APIs

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: All three zlib copies updated to 1.3.2
    Tool: Bash
    Preconditions: zlib 1.3.2 source downloaded
    Steps:
      1. grep -r "ZLIB_VERSION" OfficeUtils/src/zlib-*/zlib.h 2>/dev/null | head -3
      2. Verify version string is "1.3.2"
      3. ls -la DesktopEditor/raster/Jp2/openjpeg/openjpeg-*/thirdparty/libz/zlib.h 2>/dev/null
    Expected Result: All zlib copies report version 1.3.2
    Failure Indicators: Any copy still shows 1.2.11
    Evidence: .sisyphus/evidence/task-16-zlib-version.txt

  Scenario: Build succeeds after zlib update
    Tool: Bash
    Preconditions: All zlib copies updated
    Steps:
      1. cmake -GNinja -B /tmp/zlib-build /opt/git/core
      2. cmake --build /tmp/zlib-build 2>&1 | tail -10
      3. echo $?
    Expected Result: Build succeeds (exit code 0)
    Evidence: .sisyphus/evidence/task-16-zlib-build.txt
  ```

  **Commit**: YES
  - Message: `deps(zlib): update zlib from 1.2.11 to 1.3.2 (all copies)`
  - Files: `OfficeUtils/src/zlib-1.2.11/` (renamed), related CMakeLists.txt files
  - Pre-commit: `cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh`

- [ ] 17. Update OpenJPEG from 2.4.0 to 2.5.4

  **What to do**:
  - Download OpenJPEG 2.5.4 source
  - Replace `DesktopEditor/raster/Jp2/openjpeg/openjpeg-2.4.0/` with 2.5.4
  - Update CMakeLists.txt references (directory name change from `openjpeg-2.4.0` to `openjpeg-2.5.4`)
  - CRITICAL: Build with `OPJ_USE_SYSTEM_LIBS=ON` to use the project's updated zlib (from Task 16) instead of OpenJPEG's bundled copy
  - Handle API deprecations:
    - `bpp` → `prec` parameter rename in color space API
    - Check for `opj_version()` return value change
  - Update any project code that uses deprecated OpenJPEG APIs
  - Build and run conversion test with JP2 test files

  **Must NOT do**:
  - Do NOT modify OpenJPEG source code (user decision: leave vendored code as-is)
  - Do NOT skip `OPJ_USE_SYSTEM_LIBS=ON` (must use project's zlib)
  - Do NOT update if Task 16 (zlib) hasn't been completed first

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Complex dependency chain with zlib, API deprecations to handle
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 16 for updated zlib)
  - **Parallel Group**: Wave 3 (sequential)
  - **Blocks**: Task 19
  - **Blocked By**: Task 16

  **References**:

  **Pattern References**:
  - `DesktopEditor/raster/Jp2/openjpeg/openjpeg-2.4.0/` — Current OpenJPEG location
  - `DesktopEditor/raster/Jp2/CMakeLists.txt` — How OpenJPEG is referenced in build
  - `OfficeUtils/src/zlib-1.2.11/` — Project zlib (updated in Task 16)

  **External References**:
  - OpenJPEG 2.5.4 changelog: `https://github.com/uclouvain/openjpeg/blob/master/CHANGELOG.md`
  - OPJ_USE_SYSTEM_LIBS: `https://github.com/uclouvain/openjpeg/blob/master/CMakeLists.txt`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: OpenJPEG 2.5.4 builds with system zlib
    Tool: Bash
    Preconditions: zlib 1.3.2 installed (Task 16)
    Steps:
      1. grep -r "OPJ_USE_SYSTEM_LIBS\|OPJ_VERSION_MAJOR" DesktopEditor/raster/Jp2/ 2>/dev/null | head -5
      2. cmake -GNinja -B /tmp/opj-build /opt/git/core -DOPJ_USE_SYSTEM_LIBS=ON
      3. cmake --build /tmp/opj-build 2>&1 | tail -10
    Expected Result: Build succeeds, no zlib symbol conflicts
    Evidence: .sisyphus/evidence/task-17-openjpeg-build.txt
  ```

  **Commit**: YES
  - Message: `deps(openjpeg): update OpenJPEG from 2.4.0 to 2.5.4`
  - Pre-commit: `cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh`

- [ ] 18. Update libxml2 from 2.9.2 to 2.12.x (After Audit)

  **What to do**:
  - Based on the audit from Task 6, update libxml2 to 2.12.x (LTS release)
  - CRITICAL: Before updating, review ALL ONLYOFFICE-specific customizations documented in Task 6's audit
  - Re-apply any necessary ONLYOFFICE patches to the 2.12.x source
  - Update `xmlversion.h` template placeholders if they're used by the build system
  - Handle API changes between 2.9.2 and 2.12.x:
    - `xmlReadMemory` signature changes
    - `xmlNodeDumpOutput` API changes
    - Deprecation of `xmlRegisterNodeDefault`/`xmlDeregisterNodeDefault`
    - New `XML_PARSE_NOCDATA` behavior
  - Update all CMakeLists.txt references to libxml2
  - Test with ALL document formats (DOCX, XLSX, PPTX, ODF) since all use libxml2 for XML parsing
  - This is the HIGHEST RISK vendored update — take extra care

  **Must NOT do**:
  - Do NOT update without completing Task 6 audit first
  - Do NOT skip testing any document format
  - Do NOT modify ONLYOFFICE code to work around libxml2 changes (adapt to new APIs instead)

  **Recommended Agent Profile**:
  - **Category**: `deep`
    - Reason: Highest-risk vendored update, heavily customized library, potential for widespread breakage
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Task 6 audit)
  - **Parallel Group**: Wave 3 (sequential, last task)
  - **Blocks**: Task 19
  - **Blocked By**: Task 6

  **References**:

  **Pattern References**:
  - `DesktopEditor/xml/libxml2/` — Current libxml2 location
  - `DesktopEditor/xml/libxml2/include/libxml/xmlversion.h` — Template placeholders
  - `.sisyphus/baselines/libxml2-audit.md` — ONLYOFFICE customization audit (from Task 6)
  - `DesktopEditor/xml/` — XML processing code that uses libxml2

  **External References**:
  - libxml2 2.12.x migration: `https://gitlab.gnome.org/GNOME/libxml2/-/blob/master/NEWS`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: libxml2 2.12.x builds and all format conversions pass
    Tool: Bash
    Preconditions: Task 6 audit complete, libxml2 2.12.x source ready
    Steps:
      1. grep "LIBXML_VERSION" DesktopEditor/xml/libxml2/include/libxml/xmlversion.h | head -3
      2. cmake -GNinja -B /tmp/xml2-build /opt/git/core
      3. cmake --build /tmp/xml2-build 2>&1 | tail -10
      4. ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Version shows 2.12.x, build succeeds, all conversions pass
    Evidence: .sisyphus/evidence/task-18-libxml2-update.txt
  ```

  **Commit**: YES
  - Message: `deps(libxml2): update libxml2 from 2.9.2 to 2.12.x`
  - Pre-commit: `cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh`

- [ ] 19. Conversion Regression Test After Vendored Updates

  **What to do**:
  - Run the FULL conversion test suite after all vendored library updates (Tasks 16-18)
  - Test ALL supported format conversions, not just the 7 files in the default test:
    - DOCX → PDF, DOCX → ODT, DOCX → RTF
    - XLSX → PDF, XLSX → ODS, XLSX → CSV
    - PPTX → PDF, PPTX → ODP
    - ODT → DOCX, ODS → XLSX, ODP → PPTX
  - Compare output with pre-update baselines (binary diff of output files)
  - If any output differs, document the difference and assess whether it's acceptable:
    - Metadata changes (timestamps, UUIDs) — usually acceptable
    - Formatting changes (font metrics, color values) — needs investigation
    - Content changes (missing text, corrupted data) — blocking issue
  - Save regression test results to `.sisyphus/evidence/task-19-regression/`

  **Must NOT do**:
  - Do NOT skip this test — vendored lib updates can subtly change output
  - Do NOT modify snapshot baselines — only document differences
  - Do NOT proceed to Wave 4 if any conversion produces corrupted output

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Comprehensive testing across many format combinations
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on Tasks 8, 9, 16, 17, 18)
  - **Parallel Group**: Wave 3 (last task)
  - **Blocks**: Wave 4 (performance work shouldn't start until regressions verified)
  - **Blocked By**: Tasks 8, 9, 16, 17, 18

  **References**:

  **Pattern References**:
  - `Test/Applications/x2tTester/conversionTest.sh` — Existing conversion test
  - `Test/Applications/x2tTester/data/` — Test document corpus
  - `X2tConverter/README.md` — Conversion XML configuration format

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: All format conversions produce valid output
    Tool: Bash
    Preconditions: All vendored libs updated
    Steps:
      1. ./Test/Applications/x2tTester/conversionTest.sh 2>&1 | tee /tmp/regression.txt
      2. grep -E "(PASS|FAIL|ERROR)" /tmp/regression.txt
    Expected Result: All conversions PASS or produce valid output
    Failure Indicators: Any conversion FAILS or produces corrupted output
    Evidence: .sisyphus/evidence/task-19-regression.txt
  ```

  **Commit**: NO (verification only, no code changes)

- [ ] 20. Replace std::map with std::unordered_map in ODF Context (~10 files)

  **What to do**:
  - Target the ODF reader/writer context files which are on the hot path for ODF document conversion:
    - `OdfFile/Reader/Format/odfcontext.h` — 8+ std::map members (lines 126, 142, 268, 270, 272, 274, 321-322)
    - `OdfFile/Reader/Format/odf_document_impl.h` — map_encryptions_, map_encryptions_extra_
    - `OdfFile/Reader/Converter/docx_conversion_context.h` — comments_map_, mapChanges_, mapReferences, etc.
    - `OdfFile/Reader/Converter/xlsxconversioncontext.h`
    - `OdfFile/Reader/Converter/xlsx_num_format_context.h`
    - `OdfFile/Reader/Converter/pptx_slide_context.h`
    - `OdfFile/Reader/Converter/pptx_text_context.cpp`
    - `OdfFile/Reader/Converter/headers_footers.cpp`
  - For each file: change `std::map<Key, Value>` to `std::unordered_map<Key, Value>`
  - Add `#include <unordered_map>` where needed
  - Update any code that relies on `std::map` ordering (iterate to check)
  - The codebase already uses `std::unordered_map` in some places (e.g., `xlsxconversioncontext.cpp:70`) — follow that pattern

  **Must NOT do**:
  - Do NOT change ALL 1,148 std::map instances — only the ~10 identified hot-path files
  - Do NOT change maps where ordering matters (check iteration patterns)
  - Do NOT change vendored code maps

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Multi-file refactoring requiring careful analysis of map usage patterns
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4 (with Tasks 21-26)
  - **Blocks**: None
  - **Blocked By**: Task 19

  **References**:

  **Pattern References**:
  - `OdfFile/Reader/Format/odfcontext.h:126,142,268,270,272,274,321-322` — 8 std::map members to convert
  - `OdfFile/Reader/Format/odfcontext.cpp:600-662` — Map usage patterns (lookup, insert, iterate)
  - `OdfFile/Reader/Converter/xlsxconversioncontext.cpp:70` — Example of existing std::unordered_map usage

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: ODF context files use unordered_map
    Tool: Bash
    Preconditions: Refactoring applied
    Steps:
      1. grep -c "std::unordered_map" OdfFile/Reader/Format/odfcontext.h
      2. cmake -GNinja -B /tmp/map-build /opt/git/core && cmake --build /tmp/map-build 2>&1 | tail -5
      3. ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: unordered_map present in header, build succeeds, conversion test passes
    Evidence: .sisyphus/evidence/task-20-odf-maps.txt
  ```

  **Commit**: YES
  - Message: `perf(odf): replace std::map with std::unordered_map in ODF context`
  - Pre-commit: `cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh`

- [ ] 21. Replace std::map with std::unordered_map in OOXML Context (~10 files)

  **What to do**:
  - Target OOXML binary format reader/writer context files:
    - `OOXML/Common/SimpleTypes_Vml.cpp` — High map count
    - `OOXML/XlsxFormat/Worksheets/SheetData.cpp` — Cell lookup maps
    - `OOXML/XlsxFormat/Pivot/Pivots.cpp` — Pivot table maps
    - `OOXML/Binary/Presentation/PPTXWriter.cpp` — Slide maps
    - `OOXML/PPTXFormat/DrawingConverter/ASCOfficeDrawingConverter.cpp` — Drawing maps
    - Other high-count map files identified in the audit
  - Follow the same pattern as Task 20

  **Must NOT do**:
  - Do NOT change maps where ordering is relied upon for output generation
  - Do NOT change vendored code

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Multi-file refactoring in complex binary format code
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 20 — different directories)
  - **Parallel Group**: Wave 4
  - **Blocks**: None
  - **Blocked By**: Task 19

  **References**:

  **Pattern References**:
  - `OOXML/XlsxFormat/Worksheets/SheetData.cpp` — Cell lookup maps (hot path for large XLSX)
  - `OOXML/Common/SimpleTypes_Vml.cpp` — High map count

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: OOXML context files use unordered_map, build passes
    Tool: Bash
    Steps:
      1. grep -c "std::unordered_map" OOXML/XlsxFormat/Worksheets/SheetData.cpp
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Build succeeds, conversions pass
    Evidence: .sisyphus/evidence/task-21-ooxml-maps.txt
  ```

  **Commit**: YES
  - Message: `perf(ooxml): replace std::map with std::unordered_map in OOXML context`

- [ ] 22. Optimize SVG Parser String Building

  **What to do**:
  - In `OdfFile/Reader/Format/svg_parser.cpp:76-191`, the SVG path parser builds number strings character-by-character using `std::wstring(1, char)` temporaries
  - Replace with a single-pass approach: find end of number token, extract via `substr()`
  - Also apply to similar patterns at lines 947-948, 979, 1009, 1045-1046

  **Must NOT do**:
  - Do NOT change the parsing logic or number format interpretation

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Isolated optimization in a single file with clear pattern
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4
  - **Blocks**: None
  - **Blocked By**: Task 5 (need performance baseline)

  **References**:
  - `OdfFile/Reader/Format/svg_parser.cpp:76-191` — Character-by-character building

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: SVG parser no longer creates per-character temporaries
    Tool: Bash
    Steps:
      1. grep -n "std::wstring(1," OdfFile/Reader/Format/svg_parser.cpp | wc -l
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Per-character temporaries reduced, conversions pass
    Evidence: .sisyphus/evidence/task-22-svg-parser.txt
  ```

  **Commit**: YES
  - Message: `perf(svg): optimize SVG parser string building`

- [ ] 23. Replace boost::lexical_cast with std::to_wstring in Hot Paths (~15 files)

  **What to do**:
  - Target top ~15 files with most `boost::lexical_cast<std::wstring>` in hot paths:
    - `OdfFile/Reader/Format/odfcontext.cpp`, `anim_elements.cpp`, `draw_common.cpp`, `svg_parser.cpp`, `draw_shapes.cpp`, `table_xlsx.cpp`
    - `OOXML/XlsxFormat/Worksheets/SheetData.cpp` (15 instances)
    - `OOXML/Common/SimpleTypes_Vml.cpp` (16 instances)
  - Replace `boost::lexical_cast<std::wstring>(numericValue)` with `std::to_wstring(numericValue)`
  - Fix redundant double conversions in `anim_elements.cpp:1445` and `draw_shapes.cpp:935-936`

  **Must NOT do**:
  - Do NOT replace ALL 187 instances — target hot-path files only
  - Do NOT change formatting behavior (decimal precision)

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Multi-file mechanical replacement requiring careful type analysis
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4
  - **Blocks**: None
  - **Blocked By**: None

  **References**:
  - `OdfFile/Reader/Format/odfcontext.cpp:129` — Example lexical_cast usage

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Hot-path files no longer use boost::lexical_cast
    Tool: Bash
    Steps:
      1. grep -c "boost::lexical_cast" OdfFile/Reader/Format/odfcontext.cpp
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Count significantly reduced, conversions pass
    Evidence: .sisyphus/evidence/task-23-lexical-cast.txt
  ```

  **Commit**: YES
  - Message: `perf(cast): replace boost::lexical_cast in hot paths`

- [ ] 24. Set Bounded Default Font Cache Size

  **What to do**:
  - Change default `m_lCacheSize` from `-1` (unlimited) to `32` in `FontManager.cpp`
  - Add `SetCacheSize(32)` calls at conversion entry points if not already set
  - Verify cache eviction handles bounded size correctly

  **Must NOT do**:
  - Do NOT set cache size to 1 (too aggressive)
  - Do NOT change eviction algorithm

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Small, well-defined change
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4
  - **Blocks**: None
  - **Blocked By**: None

  **References**:
  - `DesktopEditor/fontengine/FontManager.cpp:282,307-312` — Cache size default
  - `DesktopEditor/fontengine/FontManager.h:70-75` — m_lCacheSize member

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Font cache has bounded default
    Tool: Bash
    Steps:
      1. grep "m_lCacheSize" DesktopEditor/fontengine/FontManager.cpp | head -5
    Expected Result: Default is positive value (not -1)
    Evidence: .sisyphus/evidence/task-24-font-cache.txt
  ```

  **Commit**: YES
  - Message: `perf(fonts): set bounded default font cache size`

- [ ] 25. Add String .reserve() in Serialization Hot Paths

  **What to do**:
  - Target files with heavy XML attribute string building:
    - `OdfFile/Reader/Format/docx_drawing.cpp:925-928`
    - `OdfFile/Reader/Format/styles.cpp:99-107`
    - `OdfFile/Reader/Format/style_paragraph_properties_docx.cpp:92-100`
    - `OdfFile/Reader/Format/style_paragraph_properties_pptx.cpp:89-97`
  - Add `.reserve()` before string concatenation loops

  **Must NOT do**:
  - Do NOT add reserve everywhere — only identified hot paths

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Multi-file optimization
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4
  - **Blocks**: None
  - **Blocked By**: None

  **References**:
  - `OdfFile/Reader/Format/docx_drawing.cpp:925-928` — CSS style concatenation

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Reserve calls added, conversions pass
    Tool: Bash
    Steps:
      1. grep -c "\.reserve(" OdfFile/Reader/Format/docx_drawing.cpp
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Reserve calls present, conversions pass
    Evidence: .sisyphus/evidence/task-25-string-reserve.txt
  ```

  **Commit**: YES
  - Message: `perf(strings): add string .reserve() in serialization hot paths`

- [ ] 26. Fix Double .count()+.at() Lookup Pattern

  **What to do**:
  - Replace `if (map.count(key)) return map.at(key);` with `auto it = map.find(key); if (it != map.end()) return it->second;`
  - Target: `odfcontext.cpp` (7 instances), `docx_conversion_context.cpp`, `pptx_text_context.cpp`, `headers_footers.cpp`, `odf_number_styles_context.cpp`

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Mechanical replacement
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 4

  **References**:
  - `OdfFile/Reader/Format/odfcontext.cpp:600-601` — Example double lookup

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Double lookups eliminated
    Tool: Bash
    Steps:
      1. grep -A1 "\.count(" OdfFile/Reader/Format/odfcontext.cpp | grep -c "\.at("
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Double lookup count reduced, conversions pass
    Evidence: .sisyphus/evidence/task-26-double-lookup.txt
  ```

  **Commit**: YES
  - Message: `perf(maps): fix double .count()+.at() lookups`

- [ ] 27. RAII Conversion in OfficeCryptReader

  **What to do**:
  - Replace raw `new[]` with `std::vector<unsigned char>` or `std::unique_ptr<unsigned char[]>` in:
    - `OfficeCryptReader/source/ECMACryptFile.cpp` — 20+ allocations
    - `OfficeCryptReader/source/CryptTransform.cpp` — `_buf` class (lines 73, 96, 126)
  - For crypto buffers needing secure cleanup: use custom deleter with `OPENSSL_cleanse`

  **Must NOT do**:
  - Do NOT convert all 600+ new[] — only OfficeCryptReader
  - Do NOT introduce performance regressions in crypto code

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - Reason: Crypto code requires careful handling of secure memory
  - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5

  **References**:
  - `OfficeCryptReader/source/CryptTransform.cpp:64-139` — _buf class
  - `OfficeCryptReader/source/ECMACryptFile.cpp` — Multiple raw new[]

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Raw new[] replaced in OfficeCryptReader
    Tool: Bash
    Steps:
      1. grep -c "new unsigned char\[" OfficeCryptReader/source/ECMACryptFile.cpp
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Count reduced, conversions pass
    Evidence: .sisyphus/evidence/task-27-raii-crypt.txt
  ```

  **Commit**: YES
  - Message: `refactor(raii): convert raw new[] to RAII in OfficeCryptReader`

- [ ] 28. RAII Conversion in OdfFile

  **What to do**:
  - Replace raw `new[]` in:
    - `OdfFile/Reader/Format/odf_document_impl.cpp:416`
    - `OdfFile/Reader/Format/draw_frame.cpp:423`

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5

  **References**:
  - `OdfFile/Reader/Format/odf_document_impl.cpp:416` — Raw new[] for file read

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: RAII applied to OdfFile allocations
    Tool: Bash
    Steps:
      1. grep -c "new unsigned char\[" OdfFile/Reader/Format/odf_document_impl.cpp
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Count reduced, conversions pass
    Evidence: .sisyphus/evidence/task-28-raii-odf.txt
  ```

  **Commit**: YES
  - Message: `refactor(raii): convert raw new[] to RAII in OdfFile`

- [ ] 29. RAII Conversion in X2tConverter

  **What to do**:
  - Replace raw `new[]` in `X2tConverter/src/ASCConverters.cpp:80`

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Single file, single allocation
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5

  **References**:
  - `X2tConverter/src/ASCConverters.cpp:80` — Raw new[] for POLE stream

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: RAII applied to ASCConverters
    Tool: Bash
    Steps:
      1. grep -c "new unsigned char\[" X2tConverter/src/ASCConverters.cpp
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Count = 0, conversions pass
    Evidence: .sisyphus/evidence/task-29-raii-x2t.txt
  ```

  **Commit**: YES
  - Message: `refactor(raii): convert raw new[] to RAII in X2tConverter`

- [ ] 30. Split ChartFromToBinary.cpp (13,051 lines)

  **What to do**:
  - Split `OOXML/Binary/Sheets/Reader/ChartFromToBinary.cpp` into logical modules by chart type
  - Verify identical `.o` symbol tables before/after: `nm build/.../ChartFromToBinary*.o`

  **Must NOT do**:
  - Do NOT change function signatures or class layouts
  - Do NOT break the build

  **Recommended Agent Profile**:
  - **Category**: `unspecified-high`
    - **Skills**: [`systematic-debugging`]

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 5

  **References**:
  - `OOXML/Binary/Sheets/Reader/ChartFromToBinary.cpp` — 13,051 lines

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: File split preserves build
    Tool: Bash
    Steps:
      1. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Build succeeds, conversions pass
    Evidence: .sisyphus/evidence/task-30-chart-split.txt
  ```

  **Commit**: YES
  - Message: `refactor(split): split ChartFromToBinary.cpp into logical modules`

- [ ] 31. Split ChartSerialize.cpp (10,999 lines)

  **What to do**: Split `OOXML/XlsxFormat/Chart/ChartSerialize.cpp` by chart type

  **Recommended Agent Profile**: `unspecified-high`

  **Parallelization**: YES, Wave 5

  **References**: `OOXML/XlsxFormat/Chart/ChartSerialize.cpp`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Build and conversions pass after split
    Tool: Bash
    Steps:
      1. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Build succeeds, conversions pass
    Evidence: .sisyphus/evidence/task-31-chartserialize-split.txt
  ```

  **Commit**: YES, `refactor(split): split ChartSerialize.cpp into logical modules`

- [ ] 32. Split BinaryReaderD.cpp + BinaryWriterD.cpp (10,790 + 10,217 lines)

  **What to do**: Split both Document binary reader/writer in `OOXML/Binary/Document/`

  **Recommended Agent Profile**: `unspecified-high`

  **Parallelization**: YES, Wave 5

  **References**: `OOXML/Binary/Document/BinReader/BinaryReaderD.cpp`, `OOXML/Binary/Document/BinWriter/BinaryWriterD.cpp`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Build and conversions pass after split
    Tool: Bash
    Steps:
      1. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Build succeeds, conversions pass
    Evidence: .sisyphus/evidence/task-32-binary-doc-split.txt
  ```

  **Commit**: YES, `refactor(split): split BinaryReaderD.cpp and BinaryWriterD.cpp`

- [ ] 33. Split BinaryReaderS.cpp + BinaryWriterS.cpp (9,948 + 9,446 lines)

  **What to do**: Split both Sheets binary reader/writer in `OOXML/Binary/Sheets/`

  **Recommended Agent Profile**: `unspecified-high`

  **Parallelization**: YES, Wave 5

  **References**: `OOXML/Binary/Sheets/Writer/BinaryReaderS.cpp`, `OOXML/Binary/Sheets/Reader/BinaryWriterS.cpp`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Build and conversions pass after split
    Tool: Bash
    Steps:
      1. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Build succeeds, conversions pass
    Evidence: .sisyphus/evidence/task-33-binary-sheet-split.txt
  ```

  **Commit**: YES, `refactor(split): split BinaryReaderS.cpp and BinaryWriterS.cpp`

- [ ] 34. Split Pivots.cpp (8,402 lines)

  **What to do**: Split `OOXML/XlsxFormat/Pivot/Pivots.cpp` by pivot element type

  **Recommended Agent Profile**: `unspecified-high`

  **Parallelization**: YES, Wave 5

  **References**: `OOXML/XlsxFormat/Pivot/Pivots.cpp`

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: Build and conversions pass after split
    Tool: Bash
    Steps:
      1. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: Build succeeds, conversions pass
    Evidence: .sisyphus/evidence/task-34-pivots-split.txt
  ```

  **Commit**: YES, `refactor(split): split Pivots.cpp into logical modules`

- [ ] 35. Split Remaining Large Files (5-8K Range)

  **What to do**:
  - Identify and split remaining project-owned files in 5,000-8,000 line range:
    - `OOXML/PPTXFormat/DrawingConverter/ASCOfficeDrawingConverter.cpp` (6,026)
    - `OOXML/XlsxFormat/Worksheets/SheetData.cpp` (6,000)
    - `DesktopEditor/fontengine/fontconverter/FontFileEncodings.cpp` (5,772)
  - Skip auto-generated data files (CodePage*.h)

  **Must NOT do**:
  - Do NOT split vendored code or auto-generated data

  **Recommended Agent Profile**: `unspecified-high`

  **Parallelization**: YES, Wave 5

  **Acceptance Criteria**:

  **QA Scenarios (MANDATORY):**

  ```
  Scenario: No project file exceeds 5K lines, build passes
    Tool: Bash
    Steps:
      1. find OOXML OdfFile X2tConverter -name "*.cpp" -not -path "*/3dParty/*" -exec wc -l {} \; | sort -rn | head -10
      2. cmake --build build && ./Test/Applications/x2tTester/conversionTest.sh
    Expected Result: No file >5K lines, conversions pass
    Evidence: .sisyphus/evidence/task-35-remaining-splits.txt
  ```

  **Commit**: YES, `refactor(split): split remaining large files (5-8K range)`

---

## Final Verification Wave (MANDATORY — after ALL implementation tasks)

> 4 review agents run in PARALLEL. ALL must APPROVE. Present consolidated results to user and get explicit "okay" before completing.

- [ ] F1. **Plan Compliance Audit** — `oracle`
  Read the plan end-to-end. For each "Must Have": verify implementation exists (read file, curl endpoint, run command). For each "Must NOT Have": search codebase for forbidden patterns — reject with file:line if found. Check evidence files exist in .sisyphus/evidence/. Compare deliverables against plan.
  Output: `Must Have [N/N] | Must NOT Have [N/N] | Tasks [N/N] | VERDICT: APPROVE/REJECT`

- [ ] F2. **Build + ASAN + Conversion Test Review** — `unspecified-high`
  Run `cmake --build build` with default flags — verify zero new warnings. Run `cmake -DENABLE_SANITIZERS=ON -B build-san && cmake --build build-san` — verify ASAN builds. Run `./Test/Applications/x2tTester/conversionTest.sh` — verify all conversions pass. Run `ctest --output-on-failure` if tests exist. Check coverage report generation.
  Output: `Build [PASS/FAIL] | ASAN [PASS/FAIL] | Conversion [PASS/FAIL] | Tests [N pass/N fail] | VERDICT`

- [ ] F3. **Real Manual QA** — `unspecified-high`
  Execute EVERY QA scenario from EVERY task — follow exact steps, capture evidence. Test cross-task integration (SSL + SSRF together, vendored updates + conversion test). Test edge cases: self-signed certs, private IPs, empty documents, large documents. Save to `.sisyphus/evidence/final-qa/`.
  Output: `Scenarios [N/N pass] | Integration [N/N] | Edge Cases [N tested] | VERDICT`

- [ ] F4. **Scope Fidelity Check** — `deep`
  For each task: read "What to do", read actual diff (git log/diff). Verify 1:1 — everything in spec was built (no missing), nothing beyond spec was built (no creep). Check "Must NOT do" compliance. Detect cross-task contamination: Task N touching Task M's files. Flag unaccounted changes.
  Output: `Tasks [N/N compliant] | Contamination [CLEAN/N issues] | Unaccounted [CLEAN/N files] | VERDICT`

---

## Commit Strategy

- **T1**: `chore(test): integrate GoogleTest framework` — Common/3dParty/googletest/
- **T2**: `build(sanitizers): add ENABLE_SANITIZERS CMake option` — common.cmake
- **T3**: `build(sanitizers): add ASAN/UBSAN suppression files` — .sisyphus/suppressions/
- **T4**: `test(sanitizers): establish ASAN/UBSAN baseline error count` — .sisyphus/baselines/
- **T5**: `perf(baseline): establish conversion timing baselines` — .sisyphus/baselines/
- **T6**: `audit(libxml2): document ONLYOFFICE customizations` — DesktopEditor/xml/libxml2/
- **T7**: `build(coverage): add lcov/gcovr coverage reporting` — .github/workflows/
- **T8**: `fix(security): enable SSL verification with configurable CA bundle` — Common/Network/FileTransporter/
- **T9**: `fix(security): add SSRF URL whitelist to FileTransporter` — Common/Network/FileTransporter/
- **T10**: `fix(security): validate input in MemoryLimit ParentProcess` — Test/Applications/MemoryLimit/
- **T11**: `fix(security): replace popen() with execve() in vboxtester` — DesktopEditor/vboxtester/
- **T12**: `fix(security): add stdin password option to ooxml_crypt` — OfficeCryptReader/ooxml_crypt/
- **T13**: `fix(security): replace rand() with crypto random for GUIDs` — OOXML/Base/
- **T14**: `fix(security): fix mkstemp() undefined behavior` — Common/Network/FileTransporter/
- **T15**: `fix(security): audit certificate password storage in xmlsec` — DesktopEditor/xmlsec/
- **T16**: `deps(zlib): update zlib from 1.2.11 to 1.3.2 (all copies)` — OfficeUtils/, cximage/, OpenJPEG/
- **T17**: `deps(openjpeg): update OpenJPEG from 2.4.0 to 2.5.4` — DesktopEditor/raster/Jp2/
- **T18**: `deps(libxml2): update libxml2 from 2.9.2 to 2.12.x` — DesktopEditor/xml/libxml2/
- **T19**: `test(regression): verify conversion after vendored updates` — Test/
- **T20**: `perf(odf): replace std::map with std::unordered_map in ODF context` — OdfFile/
- **T21**: `perf(ooxml): replace std::map with std::unordered_map in OOXML context` — OOXML/
- **T22**: `perf(svg): optimize SVG parser string building` — OdfFile/Reader/Format/
- **T23**: `perf(cast): replace boost::lexical_cast in hot paths` — OdfFile/, OOXML/
- **T24**: `perf(fonts): set bounded default font cache size` — DesktopEditor/fontengine/
- **T25**: `perf(strings): add string .reserve() in serialization hot paths` — OdfFile/, OOXML/
- **T26**: `perf(maps): fix double .count()+.at() lookups` — OdfFile/, OOXML/
- **T27**: `refactor(raii): convert raw new[] to RAII in OfficeCryptReader` — OfficeCryptReader/
- **T28**: `refactor(raii): convert raw new[] to RAII in OdfFile` — OdfFile/
- **T29**: `refactor(raii): convert raw new[] to RAII in X2tConverter` — X2tConverter/
- **T30**: `refactor(split): split ChartFromToBinary.cpp into logical modules` — OOXML/Binary/Sheets/Reader/
- **T31**: `refactor(split): split ChartSerialize.cpp into logical modules` — OOXML/XlsxFormat/Chart/
- **T32**: `refactor(split): split BinaryReaderD.cpp and BinaryWriterD.cpp` — OOXML/Binary/Document/
- **T33**: `refactor(split): split BinaryReaderS.cpp and BinaryWriterS.cpp` — OOXML/Binary/Sheets/
- **T34**: `refactor(split): split Pivots.cpp into logical modules` — OOXML/XlsxFormat/Pivot/
- **T35**: `refactor(split): split remaining large files (5-8K range)` — Various/

---

## Success Criteria

### Verification Commands
```bash
# Build (default)
cmake -GNinja -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build  # Expected: SUCCESS, 0 errors

# Build (sanitizers)
cmake -GNinja -B build-san -DENABLE_SANITIZERS=ON && cmake --build build-san  # Expected: SUCCESS

# Conversion test
./Test/Applications/x2tTester/conversionTest.sh  # Expected: All conversions PASS

# Unit tests (if added)
cmake --build build && ctest --output-on-failure  # Expected: All tests PASS

# SSL verification (after fix)
# Verify curl in x2t connects with SSL verification enabled
strings build/package/x2t | grep -i "ssl_verifypeer"  # Expected: NOT found (verification enabled)

# Font cache size
grep -r "m_lCacheSize" DesktopEditor/fontengine/FontManager.cpp  # Expected: default != -1

# Coverage report
cmake -GNinja -B build-cov -DENABLE_COVERAGE=ON && cmake --build build-cov && ctest --output-on-failure && gcovr -r . --xml -o coverage.xml  # Expected: coverage.xml generated
```

### Final Checklist
- [ ] All "Must Have" present
- [ ] All "Must NOT Have" absent
- [ ] Conversion test passes
- [ ] ASAN build succeeds
- [ ] No new compiler warnings from vendored lib updates
- [ ] File splits produce identical symbol tables
