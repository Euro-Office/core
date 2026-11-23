# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(RtfLexTest)

# This directory is three levels below the repo root (RtfFile/Format/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Same pattern as docx_numbering_test (OOXML/DocxFormat/test): the RTF lexer
# is not link-complete on its own -- RtfLex.cpp pulls in NSFile,
# UnicodeConverter and the XmlUtils helpers, all already expressed by the full
# x2t converter dependency chain, so reuse the existing "x2tlib" target
# instead of re-assembling dependencies one undefined symbol at a time.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

set(RTF_FORMAT_DIR ${CORE_ROOT_DIR}/RtfFile/Format)

add_core_gtest(
    NAME    rtf_lex_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${RTF_FORMAT_DIR}/RtfLex.cpp
            ${RTF_FORMAT_DIR}/RtfToken.cpp
            ${RTF_FORMAT_DIR}/Utils.cpp
    LIBS    x2tlib
    GTEST_MAIN
)

target_include_directories(rtf_lex_test PRIVATE
    ${CORE_ROOT_DIR}
)

# The third_party ICU libraries embed libstdc++ statically and export it
# (issue #173), interposing the C++ runtime of any consumer that resolves ICU
# before the system runtime. Link the system runtime ahead of the converter
# chain so a single consistent runtime is in play (same as csv_reader_test).
# Guarded to Linux: only the Linux CI legs build the test suites today.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(rtf_lex_test PRIVATE
        "-Wl,--no-as-needed" "-lstdc++"
    )
endif()

