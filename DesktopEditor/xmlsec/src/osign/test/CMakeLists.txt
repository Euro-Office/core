cmake_minimum_required(VERSION 3.10)

project(osign_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies (mirror ADD_DEPENDENCY in test.pro).
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET osign)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src/osign/lib osign)
endif()

add_core_gtest(
    NAME    osign_test
    SOURCES main.cpp
    LIBS    kernel osign
    GTEST_MAIN
)
