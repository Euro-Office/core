# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(DjVuTest)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()
if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()
if(NOT TARGET DjVuFile)
    add_subdirectory(${CORE_ROOT_DIR}/DjVuFile DjVuFile)
endif()

add_core_gtest(
    NAME    djvu_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
    LIBS    UnicodeConverter kernel graphics PdfFile DjVuFile
    GTEST_MAIN
)

# Fixtures are read in place (from data/generate.py)
target_compile_definitions(djvu_test PRIVATE
    DJVU_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
