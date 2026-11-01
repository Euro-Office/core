# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(officecrypt_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()
if(NOT TARGET CryptoPPLib)
    add_subdirectory(${CORE_ROOT_DIR}/Common/3dParty/cryptopp/project CryptoPPLib)
endif()

# reference_vectors.h is written by generate_vectors.py.
# CryptTransform.cpp needs ConvertUTF32toUTF16, which x2t gets from the OOXML
# code; compile that file into the test.
add_core_gtest(
    NAME    officecrypt_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${CORE_ROOT_DIR}/OOXML/Base/unicode_util.cpp
    LIBS    CryptoPPLib kernel UnicodeConverter
    GTEST_MAIN
)
