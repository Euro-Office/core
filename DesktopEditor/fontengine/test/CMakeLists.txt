# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(FontEngineTest)

# This directory is three levels below the repo root (DesktopEditor/fontengine/test).
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
    NAME    fontengine_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
    LIBS    kernel graphics UnicodeConverter
    GTEST_MAIN
)

# Fixtures are read in place: fonts/ (from fonts/generate.py) and a Type 1
# font that PdfFile already ships
target_compile_definitions(fontengine_test PRIVATE
    FONTENGINE_TEST_FONTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fonts"
    FONTENGINE_TEST_TYPE1_FONT="${CORE_ROOT_DIR}/PdfFile/Resources/Fonts/n019003l.pfb"
)

# The name picker test includes ApplicationFonts.h, which includes FreeType's headers
target_include_directories(fontengine_test PRIVATE
    ${FREETYPE_SOURCES_DIR}/include
    ${FREETYPE_SOURCES_DIR}/include/freetype
)
