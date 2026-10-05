cmake_minimum_required(VERSION 3.10)

project(ooxmlsignature VERSION 1)

option(SUPPORT_OFORM "Enable OFORM support" OFF)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

add_library(ooxmlsignature SHARED)

if(APPLE)
    set_target_properties(ooxmlsignature PROPERTIES
        FRAMEWORK TRUE
        MACOSX_FRAMEWORK_IDENTIFIER asc.euro-office.ooxmlsignature
        MACOSX_FRAMEWORK_BUNDLE_VERSION 1.0
        MACOSX_FRAMEWORK_SHORT_VERSION_STRING 1.0
    )
endif()

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

# Set target version
set_target_properties(ooxmlsignature PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# HEADERS and SOURCES
target_sources(ooxmlsignature PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Certificate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/CertificateCommon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/OOXMLSigner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/OOXMLVerifier.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/XmlCanonicalizator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/XmlRels.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/XmlTransform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Certificate_openssl.h

    ${CMAKE_CURRENT_SOURCE_DIR}/src/XmlTransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CertificateCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OOXMLSigner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OOXMLVerifier.cpp
    
)

target_include_directories(ooxmlsignature PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_INSTALL_DIR}/include
)

target_compile_definitions(ooxmlsignature PRIVATE
    SUPPORT_OPENSSL
    COMMON_OPENSSL_BUILDING
)

if( UNIX )
    remove_definitions(-DUNICODE -D_UNICODE)
endif()

target_link_libraries(ooxmlsignature PRIVATE
    ${OPENSSL_LIBSSL}
    ${OPENSSL_LIBCRYPTO}
    kernel
)

if(WIN32)
    target_compile_definitions(ooxmlsignature PRIVATE SUPPORT_MS_CRYPTO)
    target_sources(ooxmlsignature PRIVATE src/Certificate_mscrypto.h)
    
    target_link_libraries(ooxmlsignature PRIVATE
        Crypt32
        Cryptui
        Advapi32
        Ws2_32
        User32
    )
endif()

if(SUPPORT_OFORM)
    target_compile_definitions(ooxmlsignature PRIVATE SUPPORT_OFORM)
    target_sources(ooxmlsignature PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/Certificate_oform.h)
endif()

if(APPLE)
    copy_framework_to_folder("ooxmlsignature" "${EO_CORE_OUTPUT_DIR}")
else()
    copy_artifacts_to_folder("ooxmlsignature" "${EO_CORE_OUTPUT_DIR}")
endif()

declare_victory( ooxmlsignature )
