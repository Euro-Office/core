cmake_minimum_required(VERSION 3.10)

project(common_db_format_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

# COfficeFileFormatChecker has no standalone CMake target of its own -- it
# has only ever been built as part of the full x2tlib (which pulls in its
# real dependencies: pole, libxml, OfficeUtils, etc., via x2t's own build
# graph). Reuse x2tlib rather than reconstructing that graph here, mirroring
# OOXML/test/CMakeLists.txt's own rationale for the same reuse.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

add_core_gtest(
    NAME    common_db_format_test
    SOURCES test_OfficeFileFormatChecker.cpp
    LIBS    x2tlib
    GTEST_MAIN
)

# x2tlib pulls in MdbEngine, which dynamically links libmdbtools.so (built
# as its own shared library by OOXML/Projects/Linux/BinDocument/CMakeLists.txt,
# deliberately not baked into x2tlib -- see that file's comment on why: LGPL
# mdbtools code must stay dynamically linked, not statically bound into this
# AGPL application). It is not collected into EO_CORE_OUTPUT_DIR (the only dir
# add_core_gtest puts on the test's LD_LIBRARY_PATH), so the test fails at load
# with "libmdbtools.so: cannot open shared object file". Extend the test's
# LD_LIBRARY_PATH to also cover the BinDocument build dir (libmdbtools.so) and
# its bundled Berkeley DB install dir (libdb-18.1.so, used by
# BerkeleyDbEngine). This mirrors add_core_gtest's own env (EO_CORE_OUTPUT_DIR + the vcpkg
# lib dir) and overrides it; done at configure time so it does not depend on
# the target being relinked.
set(_mdb_libpath "${CMAKE_BINARY_DIR}/x2t/x2tlib/BinDocument:${CMAKE_BINARY_DIR}/x2t/x2tlib/BinDocument/berkeleydb-install/lib:${EO_CORE_OUTPUT_DIR}")
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    set(_mdb_libpath "${_mdb_libpath}:${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
endif()
set_tests_properties(common_db_format_test PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${_mdb_libpath}")
