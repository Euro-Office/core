cmake_minimum_required(VERSION 3.10)

project(osign VERSION 1)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

add_library(osign SHARED)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

# Set target version
set_target_properties(osign PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# HEADERS and SOURCES
target_sources(osign PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/osign.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/storage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/certificate.cpp

    # osign.pro pulls this in through "CONFIG += open_ssl_common" (openssl.pri):
    # certificate.cpp calls NSOpenSSL::PBKDF2, AES_*_desktop_GCM and CMemoryData.
    ${CORE_ROOT_DIR}/Common/3dParty/openssl/common/common_openssl.h
    ${CORE_ROOT_DIR}/Common/3dParty/openssl/common/common_openssl.cpp
)

target_include_directories(osign PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_INSTALL_DIR}/include
)

target_compile_definitions(osign PRIVATE
    OSIGN_BUILDING
    SUPPORT_OPENSSL
    COMMON_OPENSSL_BUILDING
)

target_link_libraries(osign PRIVATE
    ${OPENSSL_LIBSSL}
    ${OPENSSL_LIBCRYPTO}
    kernel
)

if(WIN32)
    target_link_libraries(osign PRIVATE
        Crypt32
        Cryptui
        Advapi32
        Ws2_32
        User32
    )
endif()

set_default_options(osign)

copy_artifacts_to_folder("osign" "${EO_CORE_OUTPUT_DIR}")

declare_victory( osign )
