# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(systemutils_test)

# This directory is three levels below the repo root (DesktopEditor/common/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

add_core_gtest(
    NAME    systemutils_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/systemutils_test.cpp
    LIBS    kernel UnicodeConverter
    GTEST_MAIN
)
