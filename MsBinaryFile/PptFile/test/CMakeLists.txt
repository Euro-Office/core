# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(pptfile_test)

# MsBinaryFile/PptFile/test -> three levels under the repository root.
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# The records under test live in PptFormatLib, and its record factory
# (Reader/Records.cpp) reaches the XLS stream classes and the rest of the
# OOXML/MS format-library set, which are only ever linked together inside
# x2tlib. Add the x2tlib "library" subdirectory so those targets exist and link
# them in a group, as OdfFile/Test/test_odf does. In the normal top-level build
# x2t is configured first, so this is a no-op there.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake/library x2tlib)
endif()

# No fixtures: every case builds its compound file in a fresh temp directory.
# COfficeFileFormatChecker is compiled into the target like in test_odf: the
# format libraries reference it, but only x2tlib compiles it.
add_core_gtest(
    NAME    pptfile_test
    SOURCES ExObjStgTest.cpp
            ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
    GTEST_MAIN
)

target_link_libraries(pptfile_test PRIVATE
    -Wl,--start-group
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
    -Wl,--end-group
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
