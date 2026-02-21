cmake_minimum_required(VERSION 3.14)
project(hunspell LANGUAGES CXX)

# ----------------------------------------------------------------------
# Configuration Options
# ----------------------------------------------------------------------
# Replaces: hunspell_build_static { ... } else { ... }
option(HUNSPELL_BUILD_STATIC "Build hunspell as a static library" OFF)

if(HUNSPELL_BUILD_STATIC)
    set(LIB_TYPE STATIC)
else()
    # qmake had CONFIG += plugin, which usually implies a MODULE in CMake 
    # (dynamically loaded at runtime). If you link directly against this 
    # library in your other targets, use SHARED instead.
    set(LIB_TYPE SHARED)
endif()

# Replaces: TARGET = hunspell, TEMPLATE = lib
add_library(hunspell ${LIB_TYPE})

# ----------------------------------------------------------------------
# Include external configurations (from base.pri)
# ----------------------------------------------------------------------
# Replaces: 
# CORE_ROOT_DIR = $$PWD/../../../..
# include($$CORE_ROOT_DIR/Common/base.pri)
set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
# Uncomment the line below if you have converted base.pri to a base.cmake file:
# include("${CORE_ROOT_DIR}/Common/base.cmake" OPTIONAL)

include(${CORE_ROOT_DIR}/common.cmake)

# -------------------------------------------------
# Pre / Post build scripts
# -------------------------------------------------
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-build.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell" "no"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# ----------------------------------------------------------------------
# Defines and Include Paths
# ----------------------------------------------------------------------
# Replaces: building_hunspell { DEFINES += BUILDING_LIBHUNSPELL }
target_compile_definitions(hunspell PRIVATE BUILDING_LIBHUNSPELL)

if(HUNSPELL_BUILD_STATIC)
    # Replaces: hunspell_build_static { DEFINES += HUNSPELL_STATIC }
    # Made PUBLIC so targets linking to this library know it's statically linked
    target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)
endif()

# Replaces: INCLUDEPATH += ...
target_include_directories(hunspell 
    PUBLIC 
        ${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../wasm/src
)

# ----------------------------------------------------------------------
# Sources and Headers
# ----------------------------------------------------------------------
# Replaces: SOURCES += ... and HEADERS += ...
target_sources(hunspell PRIVATE
    # Sources
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/affentry.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/affixmgr.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/csutil.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/filemgr.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hashmgr.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hunspell.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hunzip.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/phonet.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/replist.cxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/suggestmgr.cxx"

    # Headers (Optional for CMake to build, but highly recommended for IDE integration)
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/affentry.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/affixmgr.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/atypes.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/baseaffix.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/csutil.hxx"
    #"${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/dictmgr.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/filemgr.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hashmgr.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/htypes.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hunspell.h"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hunspell.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/hunzip.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/langnum.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/phonet.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/replist.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/suggestmgr.hxx"
    "${EO_CORE_3RD_PARTY_WORK_DIR}/hunspell/src/hunspell/w_char.hxx"
)

# Specify C++ standard if necessary (e.g., C++11 or C++14)
# target_compile_features(hunspell PUBLIC cxx_std_14)