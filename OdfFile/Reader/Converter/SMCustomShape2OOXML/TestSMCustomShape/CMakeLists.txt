cmake_minimum_required(VERSION 3.10)

project(starmath_smcustomshape_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies (mirror ADD_DEPENDENCY in TestSMCustomShape.pro / SMCustomShape2OOXML.pri).
if(NOT TARGET SMCustomShape2OOXML)
    add_subdirectory(${CORE_ROOT_DIR}/OdfFile/Reader/Converter/SMCustomShape2OOXML SMCustomShape2OOXML)
endif()

add_core_gtest(
    NAME    starmath_smcustomshape_test
    SOURCES main.cpp
    LIBS    SMCustomShape2OOXML kernel UnicodeConverter
    GTEST_MAIN
)
