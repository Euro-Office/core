cmake_minimum_required(VERSION 3.10)

project(pdffile_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies (mirror ADD_DEPENDENCY in test.pro:
#   UnicodeConverter, kernel, graphics, PdfFile, DjVuFile, ooxmlsignature).
# All six map to existing CMake library targets.
if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()
if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()
if(NOT TARGET DjVuFile)
    add_subdirectory(${CORE_ROOT_DIR}/DjVuFile DjVuFile)
endif()
if(NOT TARGET ooxmlsignature)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src ooxmlsignature)
endif()

# The tests build the PDFs they need with CPdfFile's writer: no fixtures.
add_core_gtest(
    NAME    pdffile_test
    SOURCES test.cpp
    LIBS    UnicodeConverter kernel graphics PdfFile DjVuFile ooxmlsignature
    GTEST_MAIN
)

# test.cpp includes PdfWriter.h for the pCodes regression case. That header
# pulls its dependencies through ../../ relative to PdfFile/, so the test needs
# the same include directories the PdfFile target uses.
target_include_directories(pdffile_test PRIVATE
    ${CORE_ROOT_DIR}/PdfFile/lib
    ${CORE_ROOT_DIR}/PdfFile/lib/goo
    ${CORE_ROOT_DIR}/PdfFile/lib/fofi
    ${CORE_ROOT_DIR}/PdfFile/lib/splash
    ${OPENJPEG_SOURCES_DIR}/src/lib/openjp2
    ${CORE_ROOT_DIR}/DesktopEditor/raster/Jp2/openjpeg
    ${FREETYPE_SOURCES_DIR}/include
    ${FREETYPE_SOURCES_DIR}/include/freetype
    ${ZLIBNG_INCLUDEDIR_ABS}
    ${CORE_ROOT_DIR}/OfficeUtils/src
    ${EO_CORE_3RD_PARTY_INSTALL_DIR}/brotli/c/include
)

# A small TrueType font (5 glyphs) for the text tests, read in place
target_compile_definitions(pdffile_test PRIVATE
    PDFFILE_TEST_TRUETYPE_FONT="${CORE_ROOT_DIR}/DesktopEditor/fontengine/test/fonts/EOTestTrueType.ttf"
)
