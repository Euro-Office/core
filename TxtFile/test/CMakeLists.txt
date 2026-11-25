# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(txtfile_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

# TxtFile.cpp is compiled into the test, so the test does not need the whole
# TxtXmlFormatLib. It needs ConvertUTF16toUTF32 and ConvertUTF32toUTF16, which
# x2t gets from the OOXML code.
add_core_gtest(
    NAME    txtfile_test
    SOURCES txtfile_test.cpp
            ${CORE_ROOT_DIR}/TxtFile/Source/TxtFormat/TxtFile.cpp
            ${CORE_ROOT_DIR}/OOXML/Base/unicode_util.cpp
    LIBS    kernel UnicodeConverter
    GTEST_MAIN
)
