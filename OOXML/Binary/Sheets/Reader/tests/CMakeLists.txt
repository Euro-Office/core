cmake_minimum_required(VERSION 3.10)

project(database_reader_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# DatabaseReader::Read, the engines, and the whole OOX::Spreadsheet object
# model it writes into are already compiled into x2tlib; reuse it rather
# than reconstructing that dependency graph (same rationale as
# OOXML/test/CMakeLists.txt and DatabaseEngines/tests/CMakeLists.txt).
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

# The test also calls sqlite3_exec/etc directly to build its fixture
# database, not just through DatabaseReader -- x2tlib links sqlite3
# statically/internally without re-exporting it, so it needs its own
# find_package()+link here (see DatabaseEngines/tests/CMakeLists.txt for
# the same issue on the engine tests).
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# PPTX::g_Namespaces is a namespace-scope `static` global (internal linkage)
# instantiated in every TU that includes the Xlsx object-model headers, so
# test_DatabaseReader.cpp needs PPTX::Namespaces::Namespaces() -- which lives
# in x2tlib with hidden visibility and is therefore not linkable from here.
# Namespaces.cpp has no dependencies beyond its own header, so compile it
# directly into the test (the .so's hidden copy does not conflict).
add_core_gtest(
    NAME    database_reader_test
    SOURCES test_DatabaseReader.cpp
            ${CORE_ROOT_DIR}/OOXML/PPTXFormat/Namespaces.cpp
    LIBS    x2tlib
            unofficial::sqlite3::sqlite3
)

# Same mdbtools / Berkeley DB runtime-path issue as the engine tests: x2tlib
# pulls in libmdbtools.so and libdb-18.1.so, which are not collected into
# EO_CORE_OUTPUT_DIR. Extend the test's LD_LIBRARY_PATH to their build dirs.
set(_mdb_libpath "${CMAKE_BINARY_DIR}/x2t/x2tlib/BinDocument:${CMAKE_BINARY_DIR}/x2t/x2tlib/BinDocument/berkeleydb-install/lib:${EO_CORE_OUTPUT_DIR}")
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_mdb_libpath "${_mdb_libpath}:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
endif()
set_tests_properties(database_reader_test PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${_mdb_libpath}")

# T006: deploy the committed Berkeley DB fixture next to the test binary, at
# the path FixturePath() in test_DatabaseReader.cpp expects (relative to the
# test's working directory, same convention as DatabaseEngines/tests).
add_custom_command(TARGET database_reader_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_LIST_DIR}/fixtures"
            "$<TARGET_FILE_DIR:database_reader_test>/OOXML/Binary/Sheets/Reader/tests/fixtures"
    COMMENT "Deploying database_reader_test fixtures")
