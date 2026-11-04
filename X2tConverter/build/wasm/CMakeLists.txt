# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only
#
# x2t for WebAssembly: the whole converter library graph (x2tlib) linked into a
# single x2t.wasm that runs in a Web Worker or in node. Used by the editor's
# client-only ("local session") mode, where the browser converts documents to
# the editor's binary format itself instead of a Document Server doing it.
#
# Added from the root CMakeLists.txt when EO_WASM_X2T is on, or configured on
# its own: emcmake cmake -S X2tConverter/build/wasm -B build-x2t-wasm
#
# Everything the native graph takes from native third-party builds is wired
# here, directory-scoped, so the sdkjs wasm modules are not affected:
# ICU from the Emscripten port, zlib-ng/minizip, OpenSSL, libxml2 and Boost
# from their *-wasm third-party builds, and no V8 (see
# DesktopEditor/doctrenderer/wasm/doctrenderer_stub.cpp).

cmake_minimum_required(VERSION 3.20)

project(x2t_wasm)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "x2t_wasm must be built with Emscripten (use emcmake).")
endif()

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(X2T_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

# The converter sources use the Linux code paths (_LINUX, LINUX, ...), exactly
# like the sdkjs wasm modules do by defining __linux__/_LINUX themselves.
set(LINUX TRUE)
set(EO_WASM_X2T ON)

include(${CORE_ROOT_DIR}/common.cmake)

# -------------------------------------------------
# Third-party wiring for the converter graph
# -------------------------------------------------

# ICU: Emscripten port. UnicodeConverter etc. reference ${LIBICUUC}/${LIBICUDATA}
# and ${ICU_INSTALL_DIR_ABS}/include, which are satisfied by the port flags.
set(ICU_INSTALL_DIR_ABS "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu-wasm")
set(LIBICUUC "")
set(LIBICUDATA "")
set(LIBICUI "")

# zlib-ng + minizip-ng (zlib/minizip compatible API), built for wasm
set(ZLIBNG_INSTALL_DIR_ABS "${EO_CORE_3RD_PARTY_INSTALL_DIR}/zlib-ng-wasm")
set(ZLIBNG_INCLUDEDIR_ABS "${ZLIBNG_INSTALL_DIR_ABS}/include")
set(ZLIBNG_LIBDIR_ABS "${ZLIBNG_INSTALL_DIR_ABS}/lib")
if(NOT TARGET minizip)
    add_library(minizip STATIC IMPORTED GLOBAL)
    set_target_properties(minizip PROPERTIES
        IMPORTED_LOCATION "${ZLIBNG_LIBDIR_ABS}/libminizip.a"
    )
endif()
if(NOT TARGET z)
    add_library(z STATIC IMPORTED GLOBAL)
    set_target_properties(z PROPERTIES
        IMPORTED_LOCATION "${ZLIBNG_LIBDIR_ABS}/libz.a"
    )
endif()

# OpenSSL (openssl-hash is the wasm build; common.cmake already resolved it)
set(OPENSSL_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/openssl-hash")
set(OPENSSL_INSTALL_DIR_ABS "${OPENSSL_WASM_INSTALL_DIR_ABS}")
set(OPENSSL_LIBSSL "${OPENSSL_WASM_LIBSSL}")
set(OPENSSL_LIBCRYPTO "${OPENSSL_WASM_LIBCRYPTO}")

# Boost (Common/3dParty/boost-wasm): static libs + CMake package config
set(BOOST_INSTALL_DIR_ABS "${EO_CORE_3RD_PARTY_INSTALL_DIR}/boost-wasm")
# The Emscripten toolchain only searches its sysroot for packages by default
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(Boost_ROOT "${BOOST_INSTALL_DIR_ABS}")
set(BOOST_ROOT "${BOOST_INSTALL_DIR_ABS}")
list(APPEND CMAKE_PREFIX_PATH "${BOOST_INSTALL_DIR_ABS}")
include_directories("${BOOST_INSTALL_DIR_ABS}/include")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost REQUIRED COMPONENTS system filesystem regex date_time)

# No V8 in the browser build
set(V8_INSTALL_DIR_ABS "")
set(V8_MONOLITH "")

# -------------------------------------------------
# Compiler and linker flags for the whole graph
# -------------------------------------------------
add_compile_options(
    -sUSE_ICU=1
    -fexceptions
    -Wno-register
)
add_compile_definitions(
    __linux__
    HAVE_UNISTD_H
    _RWSTD_NO_SETRLIMIT
    # flex lexers (katana): never probe isatty() on the in-memory input
    YY_NEVER_INTERACTIVE
)
add_link_options(
    -sUSE_ICU=1
)

# -------------------------------------------------
# The converter library and the executable
# -------------------------------------------------
add_subdirectory(${X2T_ROOT_DIR}/build/cmake/library x2tlib)

add_executable(x2t
    ${X2T_ROOT_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wasm_main.cpp
)

set_default_options(x2t)

target_link_libraries(x2t PRIVATE
    x2tlib
)

set(X2T_EXPORTED_FUNCTIONS
    _main
    _main1
    _malloc
    _free
)
list(JOIN X2T_EXPORTED_FUNCTIONS "," X2T_EXPORTED_FUNCTIONS_STR)

target_link_options(x2t PRIVATE
    -sWASM=1
    -sMODULARIZE=1
    -sEXPORT_NAME=createX2tModule
    -sENVIRONMENT=web,worker,node
    -sALLOW_MEMORY_GROWTH=1
    -sINITIAL_MEMORY=128MB
    -sSTACK_SIZE=8MB
    -sFORCE_FILESYSTEM=1
    -sNO_DISABLE_EXCEPTION_CATCHING
    -sEMULATE_FUNCTION_POINTER_CASTS=1
    # Formats skipped in library/CMakeLists.txt resolve to run-time traps
    -sERROR_ON_UNDEFINED_SYMBOLS=0
    # The native graph is a set of shared libraries that may each carry a copy
    # of a helper (zlib, format checker); in one static link the first one wins
    -Wl,--allow-multiple-definition
    -sASSERTIONS=0
    "-sEXPORTED_FUNCTIONS=[${X2T_EXPORTED_FUNCTIONS_STR}]"
    "-sEXPORTED_RUNTIME_METHODS=[ccall,cwrap,FS,HEAPU8]"
    "SHELL:--pre-js ${CMAKE_CURRENT_SOURCE_DIR}/js/pre.js"
)

# -------------------------------------------------
# Output: <package>/x2t/{x2t.js,x2t.wasm,converter.js,x2t-worker.js}
# -------------------------------------------------
set(X2T_WASM_OUTPUT_DIR "${EO_CORE_OUTPUT_DIR}/x2t")

set_target_properties(x2t PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${X2T_WASM_OUTPUT_DIR}"
    SUFFIX ".js"
)

add_custom_command(TARGET x2t POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${X2T_WASM_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/js/converter.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/js/x2t-worker.js"
        "${X2T_WASM_OUTPUT_DIR}/"
    COMMENT "Copying the x2t JS driver to ${X2T_WASM_OUTPUT_DIR}"
)

declare_victory( x2t )
