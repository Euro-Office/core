cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(VbaFormatLib)

set(SOURCES_DIR "../../../../Common/Vba")

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

target_include_directories(VbaFormatLib PRIVATE ${CMAKE_SOURCE_DIR})

target_compile_options(VbaFormatLib PRIVATE
    -Wno-register
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wall
    -Wextra
    -Wno-ignored-qualifiers
    -O2
    -fPIC
    -D_REENTRANT
    -DINTVER=2.5.565.0
    -DWIN_INTVER=2,5,565,0
    -DCOPYRIGHT_YEAR=2025
    -DQT_VERSION_6
    -DLINUX
    -D_LINUX
    -DCRYPTOPP_DISABLE_ASM
    -DUNICODE
    -D_UNICODE
    -DDONT_WRITE_EMBEDDED_FONTS
    -DQT_NO_DEBUG
)
