cmake_minimum_required(VERSION 3.10)

project(TxtXmlFormatLib)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

set(TXT_XML_FORMAT_SOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/../../Source")


set(BOOST_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/boost/linux_64")
find_package(Boost REQUIRED COMPONENTS system filesystem regex date_time)

add_library(TxtXmlFormatLib STATIC
	${TXT_XML_FORMAT_SOURCES_DIR}/ConvertDocx2Txt.cpp
    ${TXT_XML_FORMAT_SOURCES_DIR}/ConvertTxt2Docx.cpp
    ${TXT_XML_FORMAT_SOURCES_DIR}/PptxTxtConverter.cpp
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtFormat/File.cpp
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtFormat/TxtFile.cpp
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtRenderer.cpp
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtXmlFile.cpp

    ${TXT_XML_FORMAT_SOURCES_DIR}/ConvertDocx2Txt.h
    ${TXT_XML_FORMAT_SOURCES_DIR}/ConvertTxt2Docx.h
    ${TXT_XML_FORMAT_SOURCES_DIR}/PptxTxtConverter.h
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtFormat/File.h
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtFormat/TxtFile.h
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtRenderer.h
    ${TXT_XML_FORMAT_SOURCES_DIR}/TxtXmlFile.h
)

set_default_options(TxtXmlFormatLib)

target_link_libraries(TxtXmlFormatLib PRIVATE 
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
)

set_property(TARGET TxtXmlFormatLib PROPERTY POSITION_INDEPENDENT_CODE ON)

copy_boost_libs(TxtXmlFormatLib)