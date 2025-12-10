cmake_minimum_required(VERSION 3.10)

project(HtmlFile2)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

include(${CORE_ROOT_DIR}/common.cmake)

set(HTML_3RDPARTY_ROOT "${CORE_ROOT_DIR}/Common/3dParty/html")

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET kernel_network)
    add_subdirectory(${CORE_ROOT_DIR}/Common/Network kernel_network)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()



message( "Fetch and patch Katana parser" )
set(KATANA_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/html")
set(KATANA_INSTALL_DIR "${KATANA_3RDPARTY_DIR}/katana-parser")
get_filename_component(KATANA_3RDPARTY_DIR_ABS "${KATANA_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(KATANA_INSTALL_DIR_ABS "${KATANA_INSTALL_DIR}" ABSOLUTE)
execute_process(COMMAND ./nc-fetch.sh
    WORKING_DIRECTORY ${KATANA_3RDPARTY_DIR_ABS})



message( "Fetch and patch Gumbo parser" )
set(GUMBO_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/html")
set(GUMBO_INSTALL_DIR "${GUMBO_3RDPARTY_DIR}/gumbo-parser")
get_filename_component(GUMBO_3RDPARTY_DIR_ABS "${GUMBO_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(GUMBO_INSTALL_DIR_ABS "${GUMBO_INSTALL_DIR}" ABSOLUTE)
execute_process(COMMAND ./nc-fetch.sh
    WORKING_DIRECTORY ${GUMBO_3RDPARTY_DIR_ABS})



message( "Fetch md" )
set(MD_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/md")
set(MD_INSTALL_DIR "${MD_3RDPARTY_DIR}/md4c")
get_filename_component(MD_3RDPARTY_DIR_ABS "${MD_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(MD_INSTALL_DIR_ABS "${MD_INSTALL_DIR}" ABSOLUTE)
execute_process(COMMAND ./nc-fetch.sh
    WORKING_DIRECTORY ${MD_3RDPARTY_DIR_ABS})



add_library(HtmlFile2 SHARED
    htmlfile2.cpp

    htmlfile2.h
    src/StringFinder.h
    src/Languages.h

    # katana.pri
    ${KATANA_INSTALL_DIR}/src/foundation.c
    ${KATANA_INSTALL_DIR}/src/katana.lex.c
    ${KATANA_INSTALL_DIR}/src/katana.tab.c
    ${KATANA_INSTALL_DIR}/src/parser.c
    ${KATANA_INSTALL_DIR}/src/selector.c
    ${KATANA_INSTALL_DIR}/src/tokenizer.c

    ${KATANA_INSTALL_DIR}/src/foundation.h
    ${KATANA_INSTALL_DIR}/src/katana.h
    ${KATANA_INSTALL_DIR}/src/katana.lex.h
    ${KATANA_INSTALL_DIR}/src/katana.tab.h
    ${KATANA_INSTALL_DIR}/src/parser.h
    ${KATANA_INSTALL_DIR}/src/selector.h
    ${KATANA_INSTALL_DIR}/src/tokenizer.h

    # gumbo.pri
    ${GUMBO_INSTALL_DIR}/src/attribute.c
    ${GUMBO_INSTALL_DIR}/src/char_ref.c
    ${GUMBO_INSTALL_DIR}/src/error.c
    ${GUMBO_INSTALL_DIR}/src/parser.c
    ${GUMBO_INSTALL_DIR}/src/string_buffer.c
    ${GUMBO_INSTALL_DIR}/src/string_piece.c
    ${GUMBO_INSTALL_DIR}/src/tag.c
    ${GUMBO_INSTALL_DIR}/src/tokenizer.c
    ${GUMBO_INSTALL_DIR}/src/utf8.c
    ${GUMBO_INSTALL_DIR}/src/util.c
    ${GUMBO_INSTALL_DIR}/src/vector.c

    ${GUMBO_INSTALL_DIR}/src/attribute.h
    ${GUMBO_INSTALL_DIR}/src/char_ref.h
    ${GUMBO_INSTALL_DIR}/src/error.h
    ${GUMBO_INSTALL_DIR}/src/gumbo.h
    ${GUMBO_INSTALL_DIR}/src/insertion_mode.h
    ${GUMBO_INSTALL_DIR}/src/parser.h
    ${GUMBO_INSTALL_DIR}/src/string_buffer.h
    ${GUMBO_INSTALL_DIR}/src/string_piece.h
    ${GUMBO_INSTALL_DIR}/src/tag_enum.h
    ${GUMBO_INSTALL_DIR}/src/tag_gperf.h
    ${GUMBO_INSTALL_DIR}/src/tag_sizes.h
    ${GUMBO_INSTALL_DIR}/src/tag_strings.h
    ${GUMBO_INSTALL_DIR}/src/token_type.h
    ${GUMBO_INSTALL_DIR}/src/tokenizer_states.h
    ${GUMBO_INSTALL_DIR}/src/tokenizer.h
    ${GUMBO_INSTALL_DIR}/src/utf8.h
    ${GUMBO_INSTALL_DIR}/src/util.h
    ${GUMBO_INSTALL_DIR}/src/vector.h

    # CssCalculator.pri
    ${HTML_3RDPARTY_ROOT}/css/src/CCompiledStyle.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/CCssCalculator_Private.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/CCssCalculator.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/CElement.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/CNode.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/ConstValues.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/CUnitMeasureConverter.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/StaticFunctions.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/StyleProperties.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/xhtml/CDocumentStyle.cpp
    ${HTML_3RDPARTY_ROOT}/css/src/xhtml/CXmlElement.cpp

    ${HTML_3RDPARTY_ROOT}/css/src/CCompiledStyle.h
    ${HTML_3RDPARTY_ROOT}/css/src/CCssCalculator_Private.h
    ${HTML_3RDPARTY_ROOT}/css/src/CCssCalculator.h
    ${HTML_3RDPARTY_ROOT}/css/src/CElement.h
    ${HTML_3RDPARTY_ROOT}/css/src/CNode.h
    ${HTML_3RDPARTY_ROOT}/css/src/ConstValues.h
    ${HTML_3RDPARTY_ROOT}/css/src/CssCalculator_global.h
    ${HTML_3RDPARTY_ROOT}/css/src/CUnitMeasureConverter.h
    ${HTML_3RDPARTY_ROOT}/css/src/StaticFunctions.h
    ${HTML_3RDPARTY_ROOT}/css/src/StyleProperties.h
    ${HTML_3RDPARTY_ROOT}/css/src/xhtml/CDocumentStyle.h
    ${HTML_3RDPARTY_ROOT}/css/src/xhtml/CXmlElement.h


    # md2html.pri
    ${MD_INSTALL_DIR}/../md2html.h
    ${MD_INSTALL_DIR}/src/entity.h
    ${MD_INSTALL_DIR}/src/md4c-html.h
    ${MD_INSTALL_DIR}/src/md4c.h

    ${MD_INSTALL_DIR}/../md2html.cpp
    ${MD_INSTALL_DIR}/src/entity.c
    ${MD_INSTALL_DIR}/src/md4c-html.c
    ${MD_INSTALL_DIR}/src/md4c.c
)

set_default_options(HtmlFile2)

target_include_directories(HtmlFile2 PRIVATE
    ${HTML_3RDPARTY_ROOT}
    ${HTML_3RDPARTY_ROOT}/css # CssCalculator.pri
    ${GUMBO_INSTALL_DIR_ABS}/src
)

target_compile_definitions(HtmlFile2 PRIVATE
    HTMLFILE2_USE_DYNAMIC_LIBRARY
    CSSCALCULATOR_LIBRARY_STATIC
    CSS_CALCULATOR_WITH_XHTML
    DISABLE_RUBY_SUPPORT
    MDCONVERTER_DECL_EXPORT

    # md2html.pri
    MD4C_USE_UTF8
)

# dynamic libs
target_link_libraries(HtmlFile2 PUBLIC
    kernel
    UnicodeConverter
    graphics
    kernel_network
)
