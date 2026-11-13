# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(PptxLinkedOleTest)

# This directory is three levels below the repo root (OOXML/PPTXFormat/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# x2tlib hides the PPTXFormat classes, so link the format-library group directly,
# as MsBinaryFile/PptFile/test does.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake/library x2tlib)
endif()

add_core_gtest(
    NAME    pptx_linked_ole_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
    GTEST_MAIN
)

if(APPLE)
    set(_pptx_linked_ole_test_group_begin "")
    set(_pptx_linked_ole_test_group_end "")
else()
    set(_pptx_linked_ole_test_group_begin "-Wl,--start-group")
    set(_pptx_linked_ole_test_group_end "-Wl,--end-group")
endif()

target_link_libraries(pptx_linked_ole_test PRIVATE
    ${_pptx_linked_ole_test_group_begin}
    OdfFormatLib
    DocFormatLib
    PptFormatLib
    RtfFormatLib
    TxtXmlFormatLib
    BinDocument
    PPTXFormatLib
    DocxFormatLib
    XlsbFormatLib
    XlsFormatLib
    VbaFormatLib
    CompoundFileLib
    CryptoPPLib
    ${_pptx_linked_ole_test_group_end}
    graphics
    kernel
    UnicodeConverter
    kernel_network
    Fb2File
    PdfFile
    HtmlFile2
    EpubFile
    XpsFile
    OFDFile
    DjVuFile
    doctrenderer
    DocxRenderer
    IWorkFile
    HWPFile
    StarMathConverter
    ooxmlsignature
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
)
