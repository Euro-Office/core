# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(OpenJpegTest)

# This directory is five levels below the repo root (DesktopEditor/raster/Jp2/openjpeg/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# opj_bgraframe.cpp uses NSFile::CFileBinary for its file-based entry points
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

set(OPENJPEG_ROOT_DIR "${CORE_ROOT_DIR}/DesktopEditor/raster/Jp2/openjpeg")
set(OPENJPEG_LIB_JP2 "${OPENJPEG_SOURCES_DIR}/src/lib/openjp2")

# Same openjp2 sources and wrapper as PdfFile
add_core_gtest(
    NAME    openjpeg_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${OPENJPEG_ROOT_DIR}/opj_bgraframe.cpp
            ${OPENJPEG_LIB_JP2}/bio.c
            ${OPENJPEG_LIB_JP2}/cidx_manager.c
            ${OPENJPEG_LIB_JP2}/cio.c
            ${OPENJPEG_LIB_JP2}/dwt.c
            ${OPENJPEG_LIB_JP2}/event.c
            ${OPENJPEG_LIB_JP2}/function_list.c
            ${OPENJPEG_LIB_JP2}/ht_dec.c
            ${OPENJPEG_LIB_JP2}/image.c
            ${OPENJPEG_LIB_JP2}/invert.c
            ${OPENJPEG_LIB_JP2}/j2k.c
            ${OPENJPEG_LIB_JP2}/jp2.c
            ${OPENJPEG_LIB_JP2}/mct.c
            ${OPENJPEG_LIB_JP2}/mqc.c
            ${OPENJPEG_LIB_JP2}/openjpeg.c
            ${OPENJPEG_LIB_JP2}/opj_clock.c
            ${OPENJPEG_LIB_JP2}/opj_malloc.c
            ${OPENJPEG_LIB_JP2}/phix_manager.c
            ${OPENJPEG_LIB_JP2}/pi.c
            ${OPENJPEG_LIB_JP2}/ppix_manager.c
            ${OPENJPEG_LIB_JP2}/sparse_array.c
            ${OPENJPEG_LIB_JP2}/t1.c
            ${OPENJPEG_LIB_JP2}/t2.c
            ${OPENJPEG_LIB_JP2}/tcd.c
            ${OPENJPEG_LIB_JP2}/tgt.c
            ${OPENJPEG_LIB_JP2}/thix_manager.c
            ${OPENJPEG_LIB_JP2}/thread.c
            ${OPENJPEG_LIB_JP2}/tpix_manager.c
    LIBS    kernel
    GTEST_MAIN
)

target_include_directories(openjpeg_test PRIVATE
    ${OPENJPEG_LIB_JP2}
    ${OPENJPEG_ROOT_DIR}
)

target_compile_definitions(openjpeg_test PRIVATE
    USE_JPIP
    OPJ_STATIC
)
