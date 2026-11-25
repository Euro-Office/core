# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)
project(hyphen C)

include(GNUInstallDirs)

set(HYPHEN_SRC_DIR "" CACHE PATH "Path to fetched hyphen sources")

if(NOT HYPHEN_SRC_DIR OR NOT EXISTS "${HYPHEN_SRC_DIR}/hyphen.c")
    message(FATAL_ERROR "HYPHEN_SRC_DIR must point to the fetched hyphen source directory")
endif()

add_library(hyphen STATIC
    ${HYPHEN_SRC_DIR}/hyphen.c
    ${HYPHEN_SRC_DIR}/hnjalloc.c
)

set_target_properties(hyphen PROPERTIES C_STANDARD 99)
target_include_directories(hyphen PUBLIC
    $<BUILD_INTERFACE:${HYPHEN_SRC_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_target_properties(hyphen PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(TARGETS hyphen
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    FILES
        ${HYPHEN_SRC_DIR}/hyphen.h
        ${HYPHEN_SRC_DIR}/hnjalloc.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hyphen
)
