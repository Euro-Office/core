# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(RasterFormatsTest)

# This directory is three levels below the repo root (DesktopEditor/raster/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

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
    NAME    raster_formats_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
    LIBS    kernel graphics UnicodeConverter jpeg png tiff webp webpmux
    GTEST_MAIN
)

# HEIF fixtures are read in place: heif/ (from heif/generate.py). Core only
# decodes HEIF, so the test cannot build them in memory like the others.
target_compile_definitions(raster_formats_test PRIVATE
    RASTER_TEST_HEIF_DIR="${CMAKE_CURRENT_SOURCE_DIR}/heif"
)
