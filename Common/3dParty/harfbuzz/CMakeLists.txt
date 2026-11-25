# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)
project(harfbuzz CXX)

include(GNUInstallDirs)

set(HARFBUZZ_SRC_DIR "" CACHE PATH "Path to fetched harfbuzz sources")
set(FREETYPE_INCLUDE_DIR "" CACHE PATH "Path to FreeType include directory (for hb-ft.cc)")

if(NOT HARFBUZZ_SRC_DIR OR NOT EXISTS "${HARFBUZZ_SRC_DIR}/src/hb-common.cc")
    message(FATAL_ERROR "HARFBUZZ_SRC_DIR must point to the fetched harfbuzz source directory")
endif()
if(NOT FREETYPE_INCLUDE_DIR)
    message(FATAL_ERROR "FREETYPE_INCLUDE_DIR must point to FreeType's include directory containing ft2build.h")
endif()

# Glob the individual hb-*.cc sources.
# Exclude:
#   test-*.cc  — in-tree unit tests that live alongside the sources
#   harfbuzz*.cc — amalgamation/unity files that #include the individual sources;
#                  compiling both causes duplicate-symbol link errors.
file(GLOB HARFBUZZ_SOURCES "${HARFBUZZ_SRC_DIR}/src/hb-*.cc")

# graph/gsubgpos-context.cc is a separate compilation unit (not #included by repacker).
if(EXISTS "${HARFBUZZ_SRC_DIR}/src/graph/gsubgpos-context.cc")
    list(APPEND HARFBUZZ_SOURCES "${HARFBUZZ_SRC_DIR}/src/graph/gsubgpos-context.cc")
endif()

add_library(harfbuzz STATIC ${HARFBUZZ_SOURCES})

set_target_properties(harfbuzz PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)

target_compile_definitions(harfbuzz PRIVATE
    HAVE_FREETYPE=1
    HB_NO_VISIBILITY
)

target_include_directories(harfbuzz PUBLIC
    $<BUILD_INTERFACE:${HARFBUZZ_SRC_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/harfbuzz>
)

target_include_directories(harfbuzz PRIVATE
    ${FREETYPE_INCLUDE_DIR}
)

install(TARGETS harfbuzz ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${HARFBUZZ_SRC_DIR}/src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/harfbuzz
    FILES_MATCHING PATTERN "*.h"
)
