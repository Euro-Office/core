## 2026-03-30 Architectural Decisions
- SHA1/MD5 kept as defaults (OOXML/ODF spec-required)
- std::map → unordered_map: only ~20 hot-path files (not all 1,148)
- RAII: only crypto + binary parsing (not all 600+ new[])
- zlib: 3 copies updated atomically
- OpenJPEG: OPJ_USE_SYSTEM_LIBS=ON
- libxml2: audit-first approach (Task 6 before Task 18)
- OpenSSL/libtiff/JasPer: explicitly out of scope
