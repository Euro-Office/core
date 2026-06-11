cmake_minimum_required(VERSION 3.10)

project(booleanoperations_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies (mirror ADD_DEPENDENCY in BooleanOperations_Unit-tests.pro).
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()
if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()

add_core_gtest(
    NAME    booleanoperations_test
    SOURCES tst_booleanoperations.cpp
    LIBS    kernel graphics UnicodeConverter
    GTEST_MAIN
)
