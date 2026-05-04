cmake_minimum_required(VERSION 3.10)

project(libxml)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT DEFINED ONLY_LIBXML)
    set(ONLY_LIBXML 0)
endif()
set(XML_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(LIBXML2_SOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/../../libxml2")

add_library(libxml STATIC
    ${LIBXML2_SOURCES_DIR}/buf.c
    ${LIBXML2_SOURCES_DIR}/c14n.c
    ${LIBXML2_SOURCES_DIR}/catalog.c
    ${LIBXML2_SOURCES_DIR}/chvalid.c
    ${LIBXML2_SOURCES_DIR}/debugXML.c
    ${LIBXML2_SOURCES_DIR}/dict.c
    ${LIBXML2_SOURCES_DIR}/DOCBparser.c
    ${LIBXML2_SOURCES_DIR}/encoding.c
    ${LIBXML2_SOURCES_DIR}/entities.c
    ${LIBXML2_SOURCES_DIR}/error.c
    ${LIBXML2_SOURCES_DIR}/globals.c
    ${LIBXML2_SOURCES_DIR}/hash.c
    ${LIBXML2_SOURCES_DIR}/HTMLparser.c
    ${LIBXML2_SOURCES_DIR}/HTMLtree.c
    ${LIBXML2_SOURCES_DIR}/legacy.c
    ${LIBXML2_SOURCES_DIR}/list.c
    ${LIBXML2_SOURCES_DIR}/nanoftp.c
    ${LIBXML2_SOURCES_DIR}/nanohttp.c
    ${LIBXML2_SOURCES_DIR}/parserInternals.c
    ${LIBXML2_SOURCES_DIR}/pattern.c
    ${LIBXML2_SOURCES_DIR}/relaxng.c
    ${LIBXML2_SOURCES_DIR}/SAX.c
    ${LIBXML2_SOURCES_DIR}/SAX2.c
    ${LIBXML2_SOURCES_DIR}/schematron.c
    ${LIBXML2_SOURCES_DIR}/threads.c
    ${LIBXML2_SOURCES_DIR}/tree.c
    ${LIBXML2_SOURCES_DIR}/uri.c
    ${LIBXML2_SOURCES_DIR}/xinclude.c
    ${LIBXML2_SOURCES_DIR}/xlink.c
    ${LIBXML2_SOURCES_DIR}/xmlIO.c
    ${LIBXML2_SOURCES_DIR}/xmlmemory.c
    ${LIBXML2_SOURCES_DIR}/xmlmodule.c
    ${LIBXML2_SOURCES_DIR}/xmlreader.c
    ${LIBXML2_SOURCES_DIR}/xmlregexp.c
    ${LIBXML2_SOURCES_DIR}/xmlsave.c
    ${LIBXML2_SOURCES_DIR}/xmlschemas.c
    ${LIBXML2_SOURCES_DIR}/xmlschemastypes.c
    ${LIBXML2_SOURCES_DIR}/xmlstring.c
    ${LIBXML2_SOURCES_DIR}/xmlunicode.c
    ${LIBXML2_SOURCES_DIR}/xmlwriter.c
    ${LIBXML2_SOURCES_DIR}/xpath.c
    ${LIBXML2_SOURCES_DIR}/xpointer.c
    ${LIBXML2_SOURCES_DIR}/valid.c
    ${LIBXML2_SOURCES_DIR}/parser.c
)

set_default_options(libxml)

if(NOT ONLY_LIBXML)
    message("Including XML writer!")
    target_sources(libxml PRIVATE
        ${XML_ROOT_DIR}/src/xmlwriter.cpp
        ${XML_ROOT_DIR}/src/xmllight.cpp
        ${XML_ROOT_DIR}/src/xmldom.cpp

        ${XML_ROOT_DIR}/src/xmllight_private.h
        ${XML_ROOT_DIR}/include/xmlutils.h
        ${XML_ROOT_DIR}/include/xmlwriter.h
    )
endif()

target_compile_definitions(libxml PUBLIC
    HAVE_VA_COPY
    LIBXML_READER_ENABLED
    LIBXML_PUSH_ENABLED
    LIBXML_HTML_ENABLED
    LIBXML_XPATH_ENABLED
    LIBXML_OUTPUT_ENABLED
    LIBXML_C14N_ENABLED
    LIBXML_SAX1_ENABLED
    LIBXML_TREE_ENABLED
    LIBXML_XPTR_ENABLED
    IN_LIBXML
    LIBXML_STATIC
    XML_ERROR_DISABLE_MODE # only release
)

target_include_directories(libxml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../qt
    ${LIBXML2_SOURCES_DIR}/include
    ${LIBXML2_SOURCES_DIR}/include/libxml
)

set_property(TARGET libxml PROPERTY POSITION_INDEPENDENT_CODE ON)

declare_victory( libxml )
