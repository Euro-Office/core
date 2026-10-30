cmake_minimum_required(VERSION 3.10)

project(doc_fkp_test)

# MsBinaryFile/DocFile/test -> three levels under the repository root.
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# The FKP parsers live in DocFormatLib, which is only ever linked inside x2tlib.
# Add the x2tlib "library" subdirectory (which guard-adds DocFormatLib and the
# libraries it references) so the targets exist. In the normal top-level build
# x2t is configured first, so this is a no-op there.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake/library x2tlib)
endif()

# The suite builds its pages in a temporary compound file through POLE. Like
# x2tTester, compile pole.cpp and its unicode_util.cpp helper into the target
# instead of pulling them out of DocxFormatLib.
add_core_gtest(
    NAME    doc_fkp_test
    SOURCES fkp_bounds_test.cpp
            ${CORE_ROOT_DIR}/Common/3dParty/pole/pole.cpp
            ${CORE_ROOT_DIR}/OOXML/Base/unicode_util.cpp
    GTEST_MAIN
)

# DocFormatLib's MemoryStream logs through Log::warning, which XlsFormatLib
# provides, so both static libraries go into one linker group.
target_link_libraries(doc_fkp_test PRIVATE
    -Wl,--start-group
    DocFormatLib
    XlsFormatLib
    -Wl,--end-group
    kernel
    UnicodeConverter
)
