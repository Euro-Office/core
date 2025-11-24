cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(Fb2File)

set(CORE_ROOT_DIR "..")
set(HTML_3RDPARTY_ROOT "${CORE_ROOT_DIR}/Common/3dParty/html")

file(READ "${CORE_ROOT_DIR}/Common/version.txt" VERSION_TXT_CONTENT)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()



message( "Fetch and patch Gumbo parser" )
set(GUMBO_INSTALL_DIR "${CORE_ROOT_DIR}/Common/3dParty/html/gumbo-parser")
get_filename_component(GUMBO_INSTALL_DIR_ABS "${GUMBO_INSTALL_DIR}" ABSOLUTE)
execute_process(COMMAND ./nc-fetch.sh
    WORKING_DIRECTORY ${GUMBO_INSTALL_DIR_ABS}/..)



add_library(Fb2File SHARED
    Fb2File.cpp
    Fb2File.h
    template/template.h

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

    ${HTML_3RDPARTY_ROOT}/htmltoxhtml.h
)

target_compile_definitions(Fb2File PRIVATE
    FB2FILE_USE_DYNAMIC_LIBRARY
    INTVER=${VERSION_TXT_CONTENT}
)

target_include_directories(Fb2File PRIVATE
    ${HTML_3RDPARTY_ROOT}
    ${GUMBO_INSTALL_DIR_ABS}/src
)

# dynamic libs
target_link_libraries(Fb2File PUBLIC
    kernel
    UnicodeConverter
    graphics
)
