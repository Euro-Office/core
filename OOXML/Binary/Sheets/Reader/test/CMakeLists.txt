# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(CsvReaderTest)

# This directory is five levels below the repo root
# (OOXML/Binary/Sheets/Reader/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Same pattern as OOXML/DocxFormat/test: the CSV reader is not link-complete
# on its own -- CSVReader.cpp pulls in the XlsxFormat model (CXlsx, worksheet,
# styles), CellFormatController, UnicodeConverter and NSFile, all of which are
# already expressed by the full x2t converter dependency chain, so reuse the
# existing "x2tlib" target instead of re-assembling dependencies one undefined
# symbol at a time.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

# Like OOXML/DocxFormat/test, several symbols are not resolvable from the
# shared x2tlib (hidden visibility inside it): the CSVReader implementation,
# the CellFormatController unit it depends on, and the PPTX::Namespaces
# static-init pulled in by the XlsxFormat model headers. These sources are
# compiled into the test target directly.
set(CELLFORMAT_DIR ${CORE_ROOT_DIR}/OOXML/Binary/Sheets/Reader/CellFormatController)

add_core_gtest(
    NAME    csv_reader_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${CORE_ROOT_DIR}/OOXML/Binary/Sheets/Reader/CSVReader.cpp
            ${CELLFORMAT_DIR}/CellFormatController.cpp
            ${CELLFORMAT_DIR}/CurrencyReader.cpp
            ${CELLFORMAT_DIR}/DateReader.cpp
            ${CELLFORMAT_DIR}/DigitReader.cpp
            ${CELLFORMAT_DIR}/LocalInfo.cpp
            ${CORE_ROOT_DIR}/OOXML/PPTXFormat/Namespaces.cpp
    LIBS    x2tlib
    GTEST_MAIN
)

target_include_directories(csv_reader_test PRIVATE
    ${CORE_ROOT_DIR}
)

# The third_party ICU shared libraries embed libstdc++ statically
# (Common/3dParty/icu/nc-build.sh passes -static-libstdc++) and export its
# symbols, interposing the C++ runtime of any process that resolves ICU
# before the system runtime -- as this binary's dependency order does via
# x2tlib. The std::wregex construction in DigitReader then throws
# std::bad_cast from a mixed-runtime use_facet lookup. Linking the system
# runtime as the FIRST dependency of the executable wins the symbol race
# and restores a single consistent C++ runtime. The root cause in the ICU
# build is to be reported separately.
target_link_options(csv_reader_test PRIVATE
    "-Wl,--no-as-needed"
    "-lstdc++"
)