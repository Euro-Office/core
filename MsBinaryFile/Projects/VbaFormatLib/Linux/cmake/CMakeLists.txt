cmake_minimum_required(VERSION 3.10)

project(VbaFormatLib)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
include(${CORE_ROOT_DIR}/common.cmake)

set(SOURCES_DIR "${CORE_ROOT_DIR}/MsBinaryFile/Common/Vba")


set(BOOST_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/boost/linux_64")
find_package(Boost REQUIRED COMPONENTS system filesystem regex date_time)

add_library(VbaFormatLib STATIC
    ${SOURCES_DIR}/Records.cpp
    ${SOURCES_DIR}/StreamObjects.cpp
    ${SOURCES_DIR}/VbaBinary.cpp
    ${SOURCES_DIR}/VbaReader.cpp

    ${SOURCES_DIR}/Records.h
    ${SOURCES_DIR}/StreamObjects.h
    ${SOURCES_DIR}/VbaBinary.h
    ${SOURCES_DIR}/VbaReader.h
    ${SOURCES_DIR}/VbaRecordType.h
)

set_default_options(VbaFormatLib)

target_link_libraries(VbaFormatLib PRIVATE 
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
)

target_include_directories(VbaFormatLib PRIVATE
    ${CMAKE_SOURCE_DIR}
)

set_property(TARGET VbaFormatLib PROPERTY POSITION_INDEPENDENT_CODE ON)
