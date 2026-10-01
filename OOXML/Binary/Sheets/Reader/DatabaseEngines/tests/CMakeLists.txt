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

add_core_gtest(
    NAME    database_engines_test
    SOURCES test_SqliteEngine.cpp
            test_DuckDbEngine.cpp
            test_MdbEngine.cpp
    LIBS    x2tlib
    GTEST_MAIN
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
