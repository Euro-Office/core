cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(TxtXmlFormatLib)

set(TXT_XML_FORMAT_SOURCES_DIR "../../Source")

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

target_compile_definitions(TxtXmlFormatLib PRIVATE
    UNICODE
	_UNICODE
	DONT_WRITE_EMBEDDED_FONTS
)

target_include_directories(TxtXmlFormatLib PRIVATE
	${CORE_ROOT_DIR}/MsBinaryFile/XlsFile/Format
	${OOXML_ROOT_DIR}/XlsbFormat
)
