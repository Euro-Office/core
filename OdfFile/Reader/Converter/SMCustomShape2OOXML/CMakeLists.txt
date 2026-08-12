cmake_minimum_required(VERSION 3.10)
project(SMCustomShape2OOXML VERSION 1)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

add_library(SMCustomShape2OOXML SHARED)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

# Set target version
set_target_properties(SMCustomShape2OOXML PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# HEADERS and SOURCES
target_sources(SMCustomShape2OOXML PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/smcustomshapepars.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/customshape.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/smcustomshapepars.h
    ${CMAKE_CURRENT_SOURCE_DIR}/customshape.h
)

target_link_libraries(SMCustomShape2OOXML PRIVATE
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
    kernel
    UnicodeConverter
)

if( UNIX )
    # Remove UNICODE definition if it was inherited from directory properties
    remove_definitions(-DUNICODE -D_UNICODE)
endif()

copy_artifacts_to_folder("SMCustomShape2OOXML" "${EO_CORE_OUTPUT_DIR}")

declare_victory( SMCustomShape2OOXML )
