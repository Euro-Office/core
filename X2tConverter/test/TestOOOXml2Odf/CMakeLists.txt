cmake_minimum_required(VERSION 3.10)

project(ooxml2odf_math_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Converts OOXML math (m:oMath) to StarMath/MathML with StarMath::COOXml2Odf,
# which OdfFormatLib uses when it writes ODF. The tests parse OOXML math from
# strings, so there are no fixtures.
#
# OdfFormatLib references the whole OOXML/MS format-library set, which is only
# linked together inside x2tlib: add that subdirectory so the static libraries
# exist, then link them in a group, as OdfFile/Test/test_odf does.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake/library x2tlib)
endif()

add_core_gtest(
    NAME    ooxml2odf_math_test
    SOURCES test.cpp
            ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
    GTEST_MAIN
)

if(APPLE)
    set(_math_group_begin "")
    set(_math_group_end "")
else()
    set(_math_group_begin "-Wl,--start-group")
    set(_math_group_end "-Wl,--end-group")
endif()
target_link_libraries(ooxml2odf_math_test PRIVATE
    ${_math_group_begin}
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
    ${_math_group_end}
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

# test.cpp includes "SmartArt.h" from the PPTX logic directory, like the .pro's
# INCLUDEPATH.
target_include_directories(ooxml2odf_math_test PRIVATE
    "${CORE_ROOT_DIR}/OOXML/PPTXFormat/Logic"
    "${CORE_ROOT_DIR}/OdfFile/Common"
)
