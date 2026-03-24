cmake_minimum_required(VERSION 3.10)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

project(UnicodeConverter)

include(${CORE_ROOT_DIR}/common.cmake)

message( "Fetch and build icu (UnicodeConverter)" )
set(ICU_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/icu")
set(ICU_MAJOR_VER "74")
set(ICU_MINOR_VER "2")
if( LLVM_MINGW_CROSS )
    
    set(ICU_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu_win")
    get_filename_component(ICU_3RDPARTY_DIR_ABS "${ICU_3RDPARTY_DIR}" ABSOLUTE)
    get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
    execute_process(
        COMMAND_ECHO STDOUT
        COMMAND ./nc-build.sh
            "${EO_CORE_3RD_PARTY_WORK_DIR}/icu"
            "${ICU_INSTALL_DIR}"
            ${ICU_MAJOR_VER}
            ${ICU_MINOR_VER}
            full
            windows-crosscompile
        WORKING_DIRECTORY ${ICU_3RDPARTY_DIR_ABS}
    )

else()

    set(ICU_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu")
    get_filename_component(ICU_3RDPARTY_DIR_ABS "${ICU_3RDPARTY_DIR}" ABSOLUTE)
    get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
    execute_process(
        COMMAND_ECHO STDOUT
        COMMAND ./nc-build.sh
            "${EO_CORE_3RD_PARTY_WORK_DIR}/icu"
            "${ICU_INSTALL_DIR}"
            ${ICU_MAJOR_VER}
            ${ICU_MINOR_VER}
            full
            linux
        WORKING_DIRECTORY ${ICU_3RDPARTY_DIR_ABS}
    )

endif()




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

if( LLVM_MINGW_CROSS )

    target_link_libraries(UnicodeConverter PUBLIC
        ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.dll.a
    )

else()

    target_link_libraries(UnicodeConverter PUBLIC
        ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
        ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
    )

endif()

copy_artifacts_to_folder("UnicodeConverter" "${EO_CORE_OUTPUT_DIR}")
copy_icu_libs(UnicodeConverter)

