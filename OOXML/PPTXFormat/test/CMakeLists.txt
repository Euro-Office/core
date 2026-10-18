# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(PptxCommentsTest)

# This directory is three levels below the repo root (OOXML/PPTXFormat/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Same pattern as OOXML/test: the shared "x2tlib" target (built from
# X2tConverter/build/cmake) already expresses the full converter dependency
# chain, and it already compiles cextracttools.cpp, ASCConverters.cpp and
# OfficeFileFormatChecker2.cpp — so those MUST NOT be compiled in again. The
# only converter source x2tlib does not build is the dylib entry point
# src/dylib/x2t.cpp (which provides X2T_Convert); we add that one to the test.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

# The suite reads its committed fixtures from the working directory; stage
# them next to the build tree (see the POST_BUILD command below).
set(_pptx_stage_dir "${CMAKE_CURRENT_BINARY_DIR}/fixtures")

add_core_gtest(
    NAME    pptx_comments_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${CORE_ROOT_DIR}/X2tConverter/src/dylib/x2t.cpp
    LIBS    x2tlib
    GTEST_MAIN
    WORKING_DIRECTORY "${_pptx_stage_dir}"
)

# x2t.cpp must export X2T_Convert through src/dylib/x2t.h.
target_compile_definitions(pptx_comments_test PRIVATE
    BUILD_X2T_AS_LIBRARY_DYLIB
)

add_custom_command(TARGET pptx_comments_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_pptx_stage_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ExampleFiles" "${_pptx_stage_dir}"
    COMMENT "Staging pptx_comments_test ExampleFiles fixtures"
)

# Same ICU libstdc++ interposition caveat as csv_reader_test (issue #173): the
# third_party ICU shared libs embed libstdc++ statically with default
# visibility, and this binary resolves ICU before the system runtime through
# x2tlib. Linking the system runtime as the FIRST dependency wins the symbol
# race and keeps a single consistent C++ runtime.
target_link_options(pptx_comments_test PRIVATE
    "-Wl,--no-as-needed"
    "-lstdc++"
)
