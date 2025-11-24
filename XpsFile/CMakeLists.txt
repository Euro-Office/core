cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(XpsFile)

set(CORE_ROOT_DIR "..")

if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()

add_library(XpsFile SHARED
    XpsFile.cpp
    XpsLib/ContextState.cpp
    XpsLib/Document.cpp
    XpsLib/StaticResources.cpp
    XpsLib/Utils.cpp
    XpsLib/WString.cpp
    XpsLib/XpsPage.cpp

    XpsFile.h
    XpsLib/ContextState.h
    XpsLib/Document.h
    XpsLib/FontList.h
    XpsLib/StaticResources.h
    XpsLib/Utils.h
    XpsLib/WString.h
    XpsLib/XpsPage.h
)

target_include_directories(XpsFile PRIVATE
    ${CORE_ROOT_DIR}/DesktopEditor/freetype-2.10.4/include
    ${CORE_ROOT_DIR}/DesktopEditor/freetype-2.10.4/include/freetype
)

target_compile_definitions(XpsFile PRIVATE
    XPS_USE_DYNAMIC_LIBRARY
)

# dynamic libs
target_link_libraries(XpsFile PUBLIC
    kernel
    UnicodeConverter
    graphics
    PdfFile

    # gdi32 # Windows
    # advapi32 # Windows
    # user32 # Windows
    # shell32 # Windows
    # Ole32 # Windows
)
