cmake_minimum_required(VERSION 3.10)

project(allfontsgen)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()

if(NOT TARGET kernel_network)
    add_subdirectory(${CORE_ROOT_DIR}/Common/Network kernel_network)
endif()

if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()

if(NOT TARGET doctrenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/doctrenderer doctrenderer)
endif()

if(NOT TARGET XpsFile)
    add_subdirectory(${CORE_ROOT_DIR}/XpsFile XpsFile)
endif()

if(NOT TARGET DjVuFile)
    add_subdirectory(${CORE_ROOT_DIR}/DjVuFile DjVuFile)
endif()

if(NOT TARGET DocxRenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DocxRenderer DocxRenderer)
endif()

message( "Fetch and build icu (allfontsgen)" )
set(ICU_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/icu")
set(ICU_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu")
set(ICU_MAJOR_VER "74")
set(ICU_MINOR_VER "2")
get_filename_component(ICU_3RDPARTY_DIR_ABS "${ICU_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-build.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/icu" "${ICU_INSTALL_DIR}" ${ICU_MAJOR_VER} ${ICU_MINOR_VER}
    WORKING_DIRECTORY ${ICU_3RDPARTY_DIR_ABS}
)

add_executable(allthemesgen
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set_default_options(allthemesgen)

target_include_directories(allthemesgen PRIVATE
    ${ICU_INSTALL_DIR}/include
)

target_compile_definitions(allthemesgen PRIVATE
    KERNEL_USE_DYNAMIC_LIBRARY
    GRAPHICS_USE_DYNAMIC_LIBRARY
)

target_link_libraries(allthemesgen PUBLIC
    graphics
    kernel
    kernel_network
    UnicodeConverter
    doctrenderer
    PdfFile
    XpsFile
    DjVuFile
    DocxRenderer
    ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
    ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
)
