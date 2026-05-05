cmake_minimum_required(VERSION 3.10)
project(ooxmlsignature VERSION 1)

# Custom option to replace the `support_oform` scope
option(SUPPORT_OFORM "Enable OFORM support" OFF)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Ensure Qt core and gui are NOT included automatically if using a Qt-aware environment
# (In CMake, you simply don't call find_package(Qt...) if you don't need it)

# Remove UNICODE definition if it was inherited from directory properties
remove_definitions(-DUNICODE -D_UNICODE)

# TEMPLATE = lib, CONFIG += shared plugin
# In CMake, MODULE is often used for plugins loaded at runtime, but SHARED is standard 
# for dynamic libraries linked at compile time.
add_library(ooxmlsignature SHARED)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

set(OPENSSL_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/openssl")
get_filename_component(OPENSSL_INSTALL_DIR_ABS "${OPENSSL_INSTALL_DIR}" ABSOLUTE)

# Set target version
set_target_properties(ooxmlsignature PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# HEADERS and SOURCES
target_sources(ooxmlsignature PRIVATE
    include/Certificate.h
    include/CertificateCommon.h
    include/OOXMLSigner.h
    include/OOXMLVerifier.h
    #src/XmlCanonicalizator.h
    src/XmlRels.h
    src/XmlTransform.h
    src/common.h
    src/XmlTransform.cpp
    src/CertificateCommon.cpp
    src/OOXMLSigner.cpp
    src/OOXMLVerifier.cpp
    
    # Global Openssl header
    src/Certificate_openssl.h
)

# Add include directories so the compiler can find the headers
target_include_directories(ooxmlsignature PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_INSTALL_DIR}/include
)

target_link_libraries(ooxmlsignature PRIVATE
    ${OPENSSL_INSTALL_DIR_ABS}/lib/libssl.a
    ${OPENSSL_INSTALL_DIR_ABS}/lib/libcrypto.a
)

# Global Defines
target_compile_definitions(ooxmlsignature PRIVATE
    SUPPORT_OPENSSL
)

# ADD_DEPENDENCY(kernel)
# Replaces your custom kernel dependency. 
# Assumes 'kernel' is another CMake target defined elsewhere in your project.
target_link_libraries(ooxmlsignature PRIVATE kernel)


# core_windows { ... }
if(WIN32)
    target_compile_definitions(ooxmlsignature PRIVATE SUPPORT_MS_CRYPTO)
    target_sources(ooxmlsignature PRIVATE src/Certificate_mscrypto.h)
    
    # LIBS += -lcrypt32 -lcryptui -lAdvapi32 -lws2_32 -lUser32
    target_link_libraries(ooxmlsignature PRIVATE
        crypt32
        cryptui
        advapi32
        ws2_32
        user32
    )
endif()

# support_oform { ... }
if(SUPPORT_OFORM)
    target_compile_definitions(ooxmlsignature PRIVATE SUPPORT_OFORM)
    target_sources(ooxmlsignature PRIVATE src/Certificate_oform.h)
endif()

copy_artifacts_to_folder("ooxmlsignature" "${EO_CORE_OUTPUT_DIR}")

declare_victory( ooxmlsignature )
