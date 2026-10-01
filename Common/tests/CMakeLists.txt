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
