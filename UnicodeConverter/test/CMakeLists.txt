# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(unicodeconverter_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

# main.cpp and test.pro in this directory are a manual program that reads
# testfiles/; this target only builds the GoogleTest suite.
add_core_gtest(
    NAME    unicodeconverter_test
    SOURCES codepage_test.cpp
    LIBS    UnicodeConverter
    GTEST_MAIN
)
