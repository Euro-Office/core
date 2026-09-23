cmake_minimum_required(VERSION 3.10)

project(DocxNumberingTest)

# This directory is three levels below the repo root (OOXML/DocxFormat/test).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# DocxFormatLib alone is not link-complete: Numbering.cpp pulls in
# Logic/SectionProperty.h, which reaches into PPTXFormat/Logic (UniFill, Ln,
# EffectProperties, ...) and the XmlUtils reader/writer -- none of which are in
# DocxFormatLib's own source list (that library was only ever consumed as part
# of the full x2t link, which supplies those transitively). Rather than hand-
# assembling the missing pieces one undefined symbol at a time, reuse the
# existing "x2tlib" target the same way core/OOXML/test/CMakeLists.txt does --
# it already expresses the full, working converter dependency chain (graphics,
# kernel, every format static lib, boost, icu). We don't call X2T_Convert, so
# (unlike ooxml_test) we don't need the X2tConverter/src/dylib/x2t.cpp entry
# point or BUILD_X2T_AS_LIBRARY_DYLIB.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake x2t)
endif()

# PPTX::Namespaces::Namespaces() (PPTXFormat/Namespaces.cpp) is instantiated by a
# global/static object reachable from Numbering.h's include chain, but the .cpp
# is not part of x2tlib's own compiled sources either (a small pre-existing gap
# in the CMake port, unrelated to this feature) -- add it directly.
add_core_gtest(
    NAME    docx_numbering_test
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
            ${CORE_ROOT_DIR}/OOXML/PPTXFormat/Namespaces.cpp
    LIBS    x2tlib
    GTEST_MAIN
)

target_include_directories(docx_numbering_test PRIVATE
    ${CORE_ROOT_DIR}
)
