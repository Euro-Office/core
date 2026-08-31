cmake_minimum_required(VERSION 3.10)

project(path_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# NSSystemPath is part of kernel.
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

add_core_gtest(
    NAME    path_test
    SOURCES path.cpp
    LIBS    kernel
    GTEST_MAIN
)
