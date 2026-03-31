# libxml2 Audit: ONLYOFFICE-Specific Modifications

**Date**: 2026-03-30
**Auditor**: Automated (Sisyphus-Junior)
**Vendored Version**: libxml2 2.9.2 (October 16, 2014)
**Target Version**: 2.12.x
**Risk Level**: HIGH

---

## 1. Version Confirmation

- **Source**: `DesktopEditor/xml/libxml2/NEWS` line 13: `2.9.2: Oct 16 2014`
- **Age**: ~11.5 years old at time of audit
- **Files**: 101 `.c` files, 69 `.h` files

---

## 2. Source Code Modifications

### 2.1 `xmlversion.h` — Template Placeholders

**File**: `DesktopEditor/xml/libxml2/include/libxml/xmlversion.h`

This file has been modified from upstream to use build-time template substitution:

| Line | Placeholder | Purpose |
|------|------------|---------|
| 32 | `#define LIBXML_DOTTED_VERSION "1.2.3"` | Generic placeholder version string |
| 40 | `#define LIBXML_VERSION_NUMBER LIBXML_DOTTED_VERSION` | Version as string (not numeric) |
| 50 | `#define LIBXML_VERSION_STRING "@LIBXML_VERSION_NUMBER@"` | CMake template placeholder |
| 57 | `#define LIBXML_VERSION_EXTRA "@LIBXML_VERSION_EXTRA@"` | CMake template placeholder |
| 395 | `#define LIBXML_MODULE_EXTENSION "@MODULE_EXTENSION@"` | CMake template placeholder |

**Impact**: In standard libxml2, `LIBXML_DOTTED_VERSION` would be `"2.9.2"` and `LIBXML_VERSION` would be the numeric `20902`. Here, `LIBXML_VERSION` is the string `"1.2.3"` which means any `xmlCheckVersion()` comparison will fail or behave unexpectedly. The ONLYOFFICE code never calls `LIBXML_TEST_VERSION` or `xmlCheckVersion()`, so this is not a runtime issue, but it's a code smell.

**Key change from upstream**: The `#ifndef LIBXML_VERSION_NUMBER` guard (line 39) was added to allow the build system to override the version externally. This is NOT standard upstream behavior.

### 2.2 `error.c` — XML_ERROR_DISABLE_MODE Patch

**File**: `DesktopEditor/xml/libxml2/error.c`, lines 71-88

```c
#ifndef XML_ERROR_DISABLE_MODE
void XMLCDECL
xmlGenericErrorDefaultFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...) {
    va_list args;
    if (xmlGenericErrorContext == NULL)
        xmlGenericErrorContext = (void *) stderr;
    va_start(args, msg);
    vfprintf((FILE *)xmlGenericErrorContext, msg, args);
    va_end(args);
}
#else
void XMLCDECL
xmlGenericErrorDefaultFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...) {
    // NONE
}
#endif
```

**Purpose**: In release builds, `XML_ERROR_DISABLE_MODE` is defined, which makes the default error handler a no-op. This suppresses all XML parsing error output to stderr in production.

**Risk on update**: This is a single, well-contained modification. Easy to re-apply to a new version. However, the error.c file in libxml2 2.12.x has been significantly restructured, so the patch location may shift.

### 2.3 No Other Source Modifications Detected

The remaining 100 `.c` and 68 `.h` files in `libxml2/` contain:
- No ONLYOFFICE/Ascensio copyright strings
- No ONLYOFFICE-specific `#ifdef` blocks
- No custom IO callbacks registered
- No custom entity loaders

**Conclusion**: The libxml2 source is essentially unmodified upstream 2.9.2 with exactly 2 customizations:
1. Template-ified `xmlversion.h` (version numbers)
2. `XML_ERROR_DISABLE_MODE` guard in `error.c`

---

## 3. Build System Customizations

### 3.1 CMake Build (Linux/macOS/CI)

**File**: `DesktopEditor/xml/build/cmake/CMakeLists.txt`

- Builds libxml2 as a **static library** named `libxml`
- Compiles 43 specific `.c` files (not all upstream files)
- Does NOT compile: `xmlcatalog.c` (but VS2013 does include it)
- Optionally includes ONLYOFFICE XML wrapper code (`xmldom.cpp`, `xmllight.cpp`, `xmlwriter.cpp`)

**Compile Definitions (all builds)**:
```
HAVE_VA_COPY
LIBXML_READER_ENABLED
LIBXML_PUSH_ENABLED
LIBXML_HTML_ENABLED
LIBXML_XPATH_ENABLED
LIBXML_OUTPUT_ENABLED
LIBXML_C14N_ENABLED
LIBXML_SAX1_ENABLED
LIBXML_TREE_ENABLED
LIBXML_XPTR_ENABLED
IN_LIBXML
LIBXML_STATIC
XML_ERROR_DISABLE_MODE  (release only)
```

**Include paths**:
- `xml/build/cmake/` (for config.h)
- `xml/build/qt/` (for qt config.h)
- `xml/libxml2/include/`
- `xml/libxml2/include/libxml/`

### 3.2 Qt/qmake Build (Desktop)

**File**: `DesktopEditor/xml/build/qt/libxml2.pri`

- Release mode: Uses **unity/concatenated compilation** (`libxml2_all.c` + `libxml2_all2.c`)
- Debug mode: Compiles individual `.c` files
- Same defines as CMake, except `XML_ERROR_DISABLE_MODE` is only in release
- **Note**: `libxml2_all.c` does NOT include `parser.c` (commented out on line 19); `parser.c` is in `libxml2_all2.c` separately (for compilation order)

### 3.3 Visual Studio 2013 Build (Windows)

**File**: `DesktopEditor/xml/build/vs2013/libxml2.vcxproj`

- Static library, VS2013 toolset (v120)
- Includes `xmlcatalog.c` (which CMake does not)
- Same include paths pattern
- Preprocessor definitions are minimal (just `%(PreprocessorDefinitions)`)

**File**: `DesktopEditor/xml/build/vs2013/config.h`

Custom Windows config with:
- Platform detection (`HAVE_CTYPE_H`, `HAVE_STDARG_H`, etc.)
- `isinf()`/`isnan()` polyfills for non-MSVC Windows compilers
- `mkdir()` → `_mkdir()` mapping for MSVC/MinGW
- `snprintf()` → `_snprintf()` for MSVC < 1900
- Forces `LIBXML_READER_ENABLED`, `LIBXML_PUSH_ENABLED`, `LIBXML_HTML_ENABLED`

### 3.4 JavaScript/WASM Build (Web)

**File**: `DesktopEditor/graphics/pro/js/CMakeLists.txt`

- Uses `libxml2_all.c` + `libxml2_all2.c` (unity compilation)
- Adds `BUILD_ZLIB_AS_SOURCES` (builds zlib inline)
- Adds `IMAGE_CHECKER_DISABLE_XML` (disables XML-based image checking)
- Same feature defines as CMake build

### 3.5 macOS Xcode Build

**File**: `DesktopEditor/xml/mac/libxml2.xcodeproj/project.pbxproj`
- Defines `_USE_LIBXML2_READER_` and `LIBXML_READER_ENABLED`

---

## 4. ONLYOFFICE XML Wrapper Layer

### 4.1 CXmlLiteReader (Streaming Reader)

**Files**:
- `DesktopEditor/xml/src/xmllight.cpp` — Public API
- `DesktopEditor/xml/src/xmllight_private.h` — Implementation (header-only)

**libxml2 APIs used**:
- `xmlTextReaderPtr` (reader handle)
- `xmlReaderForMemory()` — parse from in-memory buffer
- `xmlFreeTextReader()` — cleanup
- `xmlTextReaderRead()` — advance to next node
- `xmlTextReaderNodeType()` — get current node type
- `xmlTextReaderDepth()` — get current depth
- `xmlTextReaderConstName()` — get element name
- `xmlTextReaderConstValue()` — get text content
- `xmlTextReaderAttributeCount()` — count attributes
- `xmlTextReaderMoveToFirstAttribute()` / `MoveToNextAttribute()` / `MoveToElement()`
- `xmlTextReaderIsEmptyElement()` — check for self-closing tags
- `xmlTextReaderConstPrefix()` — get namespace prefix
- `xmlTextReaderIsDefault()` — check if attribute is default

### 4.2 CXmlNode (DOM-style API)

**File**: `DesktopEditor/xml/src/xmldom.cpp`

**libxml2 APIs used**:
- `xmlTextReaderRead()` — used in `CXmlDOMDocument::Parse()`
- `xmlTextReaderNodeType()` / `xmlTextReaderDepth()` / `xmlTextReaderIsEmptyElement()`
- `xmlSetGenericErrorFunc()` — error suppression (via `IXmlDOMDocument::DisableOutput()`)
- `xmlC14NExecute()` — Canonical XML (via `NSXmlCanonicalizator::Execute()`)
- `xmlOutputBufferCreateIO()` — Custom I/O for C14N output
- `xmlParseMemory()` — Parse XML for canonicalization

### 4.3 CXmlWriter

**File**: `DesktopEditor/xml/src/xmlwriter.cpp`

- Pure C++ XML writer, does NOT use libxml2's xmlwriter API
- Uses `NSStringUtils::CStringBuilder` for string assembly

### 4.4 xmlencoding.h

**File**: `DesktopEditor/xml/include/xmlencoding.h`

- Standalone encoding detection/conversion utility
- Does NOT use libxml2 APIs directly

---

## 5. External Consumers

### 5.1 Format Libraries Using `_USE_LIBXML2_READER_`

The define `_USE_LIBXML2_READER_` appears in 40+ preprocessor definitions across:
- `OOXML/` — DocxFormatLib, PPTXFormatLib, XlsbFormatLib
- `MsBinaryFile/` — DocFormatLib, PPTFormatLib, XlsFormatLib
- `OdfFile/` — cpxml, Oox2OdfConverter
- `RtfFile/` — RtfFormatLib
- `TxtFile/` — TxtXmlFormatLib
- `XpsFile/` — XpsLib
- `OfficeCryptReader/` — ECMACryptReader
- `X2tConverter/` — X2tTest

**Note**: `_USE_LIBXML2_READER_` is NEVER actually tested with `#ifdef` in any source file. It is a dead/legacy define that likely served a historical purpose (perhaps to switch between XmlLite and libxml2 reader backends on Windows).

### 5.2 xmlsec

- `DesktopEditor/xmlsec/test/windows/main.cpp` uses `xmlDocPtr`, `xmlNodePtr` for XML signature operations

---

## 6. Enabled libxml2 Features

Based on compile definitions across all build systems:

| Feature | Define | Status |
|---------|--------|--------|
| Reader (streaming) | `LIBXML_READER_ENABLED` | **Enabled** |
| Push parser | `LIBXML_PUSH_ENABLED` | **Enabled** |
| HTML parser | `LIBXML_HTML_ENABLED` | **Enabled** |
| XPath | `LIBXML_XPATH_ENABLED` | **Enabled** |
| Output/serialization | `LIBXML_OUTPUT_ENABLED` | **Enabled** |
| Canonicalization (C14N) | `LIBXML_C14N_ENABLED` | **Enabled** |
| SAX1 interface | `LIBXML_SAX1_ENABLED` | **Enabled** |
| Tree (DOM) | `LIBXML_TREE_ENABLED` | **Enabled** |
| XPointer | `LIBXML_XPTR_ENABLED` | **Enabled** |
| Static build | `LIBXML_STATIC` | **Enabled** |
| Inside libxml compilation | `IN_LIBXML` | **Enabled** |
| Error suppression | `XML_ERROR_DISABLE_MODE` | Release only |

**Disabled features** (not defined):
- `LIBXML_THREAD_ENABLED` — threading support
- `LIBXML_FTP_ENABLED` — FTP support
- `LIBXML_HTTP_ENABLED` — HTTP support
- `LIBXML_VALID_ENABLED` — DTD validation
- `LIBXML_CATALOG_ENABLED` — catalog support
- `LIBXML_ICONV_ENABLED` — iconv encoding
- `LIBXML_ICU_ENABLED` — ICU encoding
- `LIBXML_ZLIB_ENABLED` — zlib compression
- `LIBXML_LZMA_ENABLED` — LZMA compression
- `LIBXML_SCHEMAS_ENABLED` — XML Schema
- `LIBXML_MODULES_ENABLED` — dynamic modules
- `LIBXML_WRITER_ENABLED` — xmlWriter API
- `LIBXML_PATTERN_ENABLED` — pattern matching
- `LIBXML_DEBUG_ENABLED` — debugging
- `LIBXML_LEGACY_ENABLED` — deprecated APIs

---

## 7. Risk Assessment for Updating to 2.12.x

### 7.1 HIGH Risk Items

1. **API Breaking Changes**: libxml2 2.12.x has significant API changes:
   - `LIBXML_SAX1_ENABLED` is **deprecated** and may be removed
   - `xmlSAXHandler` structure has changed
   - Many functions have stricter const-correctness
   - Thread-safety model changed (global state removed)
   - `xmlInitParser()` / `xmlCleanupParser()` behavior changed

2. **xmlversion.h Template System**: The ONLYOFFICE version templating approach (`"1.2.3"` as `LIBXML_DOTTED_VERSION`) is fragile. In 2.12.x, `LIBXML_VERSION` must be a numeric value for version checks to work correctly.

3. **C14N API Changes**: `xmlC14NExecute()` signature may have changed. The `NSXmlCanonicalizator::Execute()` in `xmldom.cpp` directly calls this API.

4. **XMLReader API**: The `xmlTextReader*` APIs are the core dependency. While still present in 2.12.x, behavior around error handling and entity expansion has changed.

### 7.2 MEDIUM Risk Items

5. **Error Handling**: `XML_ERROR_DISABLE_MODE` patch location may shift in the restructured error.c. Easy to re-apply but needs verification.

6. **Build System**: The custom config.h files (qt/config.h, vs2013/config.h) may need updates for new platform requirements in 2.12.x.

7. **Unity Build Split**: `libxml2_all.c` / `libxml2_all2.c` may need regeneration as source file lists change in 2.12.x.

8. **Symbol Visibility**: `xmlexports.h` is unmodified but the 2.12.x version has a different export model. The `LIBXML_STATIC` define should handle this but needs testing.

### 7.3 LOW Risk Items

9. **Wrapper Code**: The ONLYOFFICE CXmlLiteReader/CXmlNode wrappers are thin and well-abstracted. If the underlying libxml2 APIs are compatible, the wrappers should work without changes.

10. **Dead Defines**: `_USE_LIBXML2_READER_` is never checked in code. It can be cleaned up during the update.

---

## 8. Recommended Update Strategy

### Phase 1: Preparation
1. **Generate diff** between ONLYOFFICE's xmlversion.h and error.c vs upstream 2.9.2 to confirm the exact modifications
2. **Test `XML_ERROR_DISABLE_MODE`** equivalent in 2.12.x (check if `xmlSetGenericErrorFunc(NULL, NULL)` is sufficient)
3. **Audit C14N API** compatibility in 2.12.x
4. **Review `LIBXML_SAX1_ENABLED`** deprecation impact

### Phase 2: Implementation
1. **Drop-in replacement**: Replace `DesktopEditor/xml/libxml2/` with 2.12.x source
2. **Re-apply error.c patch**: Wrap `xmlGenericErrorDefaultFunc` with `XML_ERROR_DISABLE_MODE` guard
3. **Fix xmlversion.h**: Either:
   - Use proper upstream `xmlversion.h` with actual version numbers
   - OR keep template approach but fix `LIBXML_VERSION` to be numeric
4. **Update source file list**: Update CMakeLists.txt, .pri, .vcxproj with new/removed files
5. **Update unity build files**: Regenerate `libxml2_all.c` / `libxml2_all2.c`
6. **Update config.h files**: Adjust platform detection for 2.12.x requirements
7. **Remove `LIBXML_SAX1_ENABLED`**: If deprecated in 2.12.x, test without it

### Phase 3: Verification
1. Build all targets: Linux (CMake), Windows (VS), macOS (Xcode), JS/WASM
2. Run existing XML tests
3. Test OOXML parsing (docx, xlsx, pptx)
4. Test ODF parsing
5. Test XML canonicalization (digital signatures)
6. Verify error suppression works in release builds

---

## 9. Summary of ONLYOFFICE-Specific Customizations

| # | Type | File | Description | Re-apply Difficulty |
|---|------|------|-------------|-------------------|
| 1 | Source mod | `libxml2/error.c` | `XML_ERROR_DISABLE_MODE` suppresses error output | Easy |
| 2 | Source mod | `libxml2/include/libxml/xmlversion.h` | Template placeholders for version numbers | Medium (restructure needed) |
| 3 | Build | `xml/build/cmake/CMakeLists.txt` | Custom CMake build with selective source files | Medium |
| 4 | Build | `xml/build/qt/libxml2.pri` | qmake build with unity compilation | Medium |
| 5 | Build | `xml/build/qt/libxml2_all.c` / `libxml2_all2.c` | Concatenated source for unity build | Easy (regenerate) |
| 6 | Build | `xml/build/qt/config.h` | Windows platform config | Easy |
| 7 | Build | `xml/build/vs2013/config.h` | Windows platform config (with NEED_SOCKETS) | Easy |
| 8 | Build | `xml/build/vs2013/libxml2.vcxproj` | VS2013 project file | Medium |
| 9 | Wrapper | `xml/src/xmllight.cpp` / `xmllight_private.h` | CXmlLiteReader (streaming XML reader) | N/A (no changes needed) |
| 10 | Wrapper | `xml/src/xmldom.cpp` | CXmlNode (DOM API) + C14N canonicalization | May need C14N API updates |
| 11 | Wrapper | `xml/src/xmlwriter.cpp` | CXmlWriter (pure C++, no libxml2 deps) | N/A |
| 12 | Dead define | Across 19 project files | `_USE_LIBXML2_READER_` — defined but never checked | Remove during update |
