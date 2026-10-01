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

add_core_gtest(
    NAME    database_reader_test
    SOURCES test_DatabaseReader.cpp
    LIBS    x2tlib
)
