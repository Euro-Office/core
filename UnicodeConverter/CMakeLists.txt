cmake_minimum_required(VERSION 3.10)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

include(${CORE_ROOT_DIR}/common.cmake)

project(UnicodeConverter)

set(ICU_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu")
get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)

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
    ${ICU_INSTALL_DIR_ABS}/include
)

target_link_libraries(UnicodeConverter PUBLIC
    ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
    ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
)

copy_artifacts_to_folder("UnicodeConverter" "${EO_CORE_OUTPUT_DIR}")
copy_icu_libs(UnicodeConverter)

declare_victory( UnicodeConverter )
