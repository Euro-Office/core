cmake_minimum_required(VERSION 3.10)

project(DjVuFile)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
    
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()

add_library(DjVuFile SHARED
    DjVu.cpp
    DjVuFileImplementation.cpp
    ${DJVULIBRE_SOURCES}

    DjVu.h
    DjVuFileImplementation.h
)

set_default_options(DjVuFile)

target_compile_definitions(DjVuFile PRIVATE
    DJVU_USE_DYNAMIC_LIBRARY
    ${DJVULIBRE_DEFINITIONS}
)

if( WIN32 )
    target_compile_definitions(DjVuFile PRIVATE WIN32)
endif()

target_include_directories(DjVuFile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DJVULIBRE_SOURCES_DIR}
)

# libdjvu decodes in threads
find_package(Threads REQUIRED)
target_link_libraries(DjVuFile PRIVATE Threads::Threads)

target_link_libraries(DjVuFile PUBLIC
    graphics
    kernel
    UnicodeConverter
    PdfFile
)

copy_artifacts_to_folder("DjVuFile" "${EO_CORE_OUTPUT_DIR}")

declare_victory( DjVuFile )
