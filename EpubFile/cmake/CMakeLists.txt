cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(EpubFile)

set(CORE_ROOT_DIR "../..")
set(EPUB_ROOT_DIR "..")

if(NOT TARGET HtmlFile2)
    add_subdirectory(${CORE_ROOT_DIR}/HtmlFile2 HtmlFile2)
endif()

add_library(EpubFile SHARED
    ${EPUB_ROOT_DIR}/src/CBookContentItem.cpp
    ${EPUB_ROOT_DIR}/src/CBookInfo.cpp
    ${EPUB_ROOT_DIR}/src/CBookItem.cpp
    ${EPUB_ROOT_DIR}/src/CBookToc.cpp
    ${EPUB_ROOT_DIR}/src/CEpubFile.cpp

    ${EPUB_ROOT_DIR}/CEpubFile.h
    ${EPUB_ROOT_DIR}/src/CBookToc.h
    ${EPUB_ROOT_DIR}/src/CBookContentItem.h
    ${EPUB_ROOT_DIR}/src/CBookItem.h
    ${EPUB_ROOT_DIR}/src/CBookInfo.h
    ${EPUB_ROOT_DIR}/src/EpubFile_global.h
)

target_compile_definitions(EpubFile PRIVATE
    EPUBFILE_USE_DYNAMIC_LIBRARY
)

target_include_directories(EpubFile PRIVATE
    ${EPUB_ROOT_DIR}
)

# dynamic libs
target_link_libraries(EpubFile PUBLIC
    kernel
    graphics
    HtmlFile2
)
