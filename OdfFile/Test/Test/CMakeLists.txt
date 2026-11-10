# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

cmake_minimum_required(VERSION 3.10)

project(odf_regression_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Regression tests named after the bug numbers they cover: ODT -> DOCX,
# DOCX -> ODT and PPTX -> ODP conversions whose result is inspected through the
# converters' document models. Only a Visual Studio project existed.
#
# OdfFormatLib references the whole OOXML/MS format-library set, which is only
# linked together inside x2tlib: add that subdirectory so the static libraries
# exist, then link them in a group, as OdfFile/Test/test_odf does. The
# vcxproj's extra sources (pole.cpp, unicode_util.cpp, Unit.cpp,
# SystemUtility.cpp) come with DocxFormatLib.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake/library x2tlib)
endif()

# The tests read their fixtures working-dir-relative ("ExampleFiles/44363.odt").
set(_odf_regression_run_dir "${CMAKE_CURRENT_BINARY_DIR}/run")

# src/Main.cpp has its own main(), so no GTEST_MAIN.
add_core_gtest(
    NAME    odf_regression_test
    SOURCES src/Main.cpp
            src/Common.cpp
            src/tests/Test44363.cpp
            src/tests/Test57197.cpp
            src/tests/Test59708.cpp
            src/tests/Test61364.cpp
            src/tests/Test69238.cpp
            ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
    WORKING_DIRECTORY "${_odf_regression_run_dir}"
)

if(APPLE)
    set(_odf_regression_group_begin "")
    set(_odf_regression_group_end "")
else()
    set(_odf_regression_group_begin "-Wl,--start-group")
    set(_odf_regression_group_end "-Wl,--end-group")
endif()
target_link_libraries(odf_regression_test PRIVATE
    ${_odf_regression_group_begin}
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
    ${_odf_regression_group_end}
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

# OdfFormatLib's reader headers include <CPSharedPtr.h> and friends from
# OdfFile/Common.
target_include_directories(odf_regression_test PRIVATE
    "${CORE_ROOT_DIR}/OdfFile/Common"
)

add_custom_command(TARGET odf_regression_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_odf_regression_run_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ExampleFiles"
            "${_odf_regression_run_dir}/ExampleFiles"
    COMMENT "Staging odf_regression_test fixtures"
)
