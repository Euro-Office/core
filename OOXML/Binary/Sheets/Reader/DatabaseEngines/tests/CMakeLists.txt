cmake_minimum_required(VERSION 3.10)

project(database_engines_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# SqliteEngine/DuckDbEngine/MdbEngine are already compiled into x2tlib (as
# part of OOXML/Binary/Sheets/Reader); reuse it instead of recompiling them
# here, which would also pull in sqlite3/duckdb/mdbtools linkage x2tlib's
# own build already resolves via vcpkg. See OOXML/test/CMakeLists.txt for
# the same rationale applied to a different part of this directory tree.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

# The tests also call the vendor C APIs (sqlite3_open, duckdb_open, ...)
# directly, not just through the Sqlite/DuckDb engine wrapper classes --
# x2tlib links those statically/internally without re-exporting them, so
# they need their own find_package()+link here.
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(DuckDB CONFIG REQUIRED)

add_core_gtest(
    NAME    database_engines_test
    SOURCES test_SqliteEngine.cpp
            test_DuckDbEngine.cpp
            test_MdbEngine.cpp
    LIBS    x2tlib
            unofficial::sqlite3::sqlite3
            $<IF:$<TARGET_EXISTS:duckdb>,duckdb,duckdb_static>
    GTEST_MAIN
)

# x2tlib pulls in MdbEngine, which dynamically links libmdbtools.so (built
# as its own shared library by OOXML/Projects/Linux/BinDocument/CMakeLists.txt,
# deliberately not baked into x2tlib -- LGPL mdbtools code must stay
# dynamically linked, not statically bound into this AGPL application). It is
# not collected into EO_CORE_OUTPUT_DIR (the only dir add_core_gtest puts on
# the test's LD_LIBRARY_PATH), so the test fails at load with "libmdbtools.so:
# cannot open shared object file". Extend the test's LD_LIBRARY_PATH to also
# cover the BinDocument build dir (libmdbtools.so) and its bundled Berkeley DB
# install dir (libdb-18.1.so, used by BerkeleyDbEngine). This mirrors
# add_core_gtest's own env (EO_CORE_OUTPUT_DIR + the vcpkg lib dir) and
# overrides it; done at configure time so it does not depend on the target
# being relinked.
set(_mdb_libpath "${CMAKE_BINARY_DIR}/x2t/x2tlib/BinDocument:${CMAKE_BINARY_DIR}/x2t/x2tlib/BinDocument/berkeleydb-install/lib:${EO_CORE_OUTPUT_DIR}")
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_mdb_libpath "${_mdb_libpath}:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
endif()
set_tests_properties(database_engines_test PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${_mdb_libpath}")

# mdbtools.h uses <mdbfakeglib.h> (angle brackets), resolved only via an
# explicit -I, not relative to the including file. The mdbtools CMake
# target's own include dir is PRIVATE (OOXML/Projects/Linux/BinDocument/
# CMakeLists.txt re-adds it itself for the same reason) and does not
# propagate through linking x2tlib, so test_MdbEngine.cpp needs the same
# directory added explicitly here.
target_include_directories(database_engines_test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../mdbtools
)

# MdbEngineSchemaTest's cases GTEST_SKIP themselves when this fixture is
# absent -- see fixtures/README.md. Staged here (not committed, see the
# README) so a later contributor only needs to drop the file in place.
add_custom_command(TARGET database_engines_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:database_engines_test>/OOXML/Binary/Sheets/Reader/DatabaseEngines/tests/fixtures"
    COMMENT "Ensuring database_engines_test fixtures directory exists"
)

# FR-016/T035: static source check that DuckDbEngine.cpp never reintroduces
# a call to the deprecated duckdb_row_count/duckdb_value_varchar functions
# (see research.md R8). A plain grep rather than a build/runtime check, so
# it catches a regression even in a diff that never gets compiled+run.
add_test(
    NAME duckdb_engine_no_deprecated_api
    COMMAND ${CMAKE_COMMAND}
            -DFILE=${CMAKE_CURRENT_SOURCE_DIR}/../DuckDbEngine.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckNoDeprecatedDuckDbApi.cmake
)
