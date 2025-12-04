cmake_minimum_required(VERSION 3.10)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

include(${CORE_ROOT_DIR}/common.cmake)

project(UnicodeConverter)



message( "Fetch and build icu" )
set(ICU_ROOT_DIR "${CORE_ROOT_DIR}/Common/3dParty/icu")
set(ICU_MAJOR_VER "74")
set(PLATFORM_PREFIX "linux_x64")
set(ICU_INSTALL_DIR "${ICU_ROOT_DIR}/install_${PLATFORM_PREFIX}")
get_filename_component(ICU_ROOT_DIR_ABS "${ICU_ROOT_DIR}" ABSOLUTE)
get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
execute_process(COMMAND ./nc-build.sh
                WORKING_DIRECTORY ${ICU_ROOT_DIR_ABS})



add_library(UnicodeConverter SHARED
    UnicodeConverter.cpp
    UnicodeConverter.h
)

set_default_options(UnicodeConverter)

target_compile_definitions(UnicodeConverter PRIVATE
    UNICODECONVERTER_USE_DYNAMIC_LIBRARY
)

target_include_directories(UnicodeConverter PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ICU_INSTALL_DIR}/include
)

target_link_libraries(UnicodeConverter PUBLIC
    ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
    ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
)
