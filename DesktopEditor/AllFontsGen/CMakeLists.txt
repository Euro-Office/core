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

message( "Fetch and build icu" )
set(ICU_ROOT_DIR "${CORE_ROOT_DIR}/Common/3dParty/icu")
set(ICU_MAJOR_VER "74")
set(PLATFORM_PREFIX "linux_x64")
set(ICU_INSTALL_DIR "${ICU_ROOT_DIR}/install_${PLATFORM_PREFIX}")
get_filename_component(ICU_ROOT_DIR_ABS "${ICU_ROOT_DIR}" ABSOLUTE)
get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
execute_process(COMMAND ./nc-build.sh
                WORKING_DIRECTORY ${ICU_ROOT_DIR_ABS})

add_executable(allfontsgen
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set_default_options(allfontsgen)

target_include_directories(allfontsgen PRIVATE
    ${ICU_INSTALL_DIR}/include
)


target_compile_definitions(allfontsgen PRIVATE
    KERNEL_USE_DYNAMIC_LIBRARY
    GRAPHICS_USE_DYNAMIC_LIBRARY
)

target_link_libraries(allfontsgen PUBLIC
    UnicodeConverter
    kernel
    graphics
    ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
    ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
)
