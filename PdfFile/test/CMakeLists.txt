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
