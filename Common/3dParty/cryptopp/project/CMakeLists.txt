cmake_minimum_required(VERSION 3.10)

project(CryptoPPLib)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# set(CRYPTOPP_SOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

message( "Fetch and build CryptoPP" )
set(CRYPTOPP_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/cryptopp")
set(CRYPTOPP_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/cryptopp")
get_filename_component(CRYPTOPP_3RDPARTY_DIR_ABS "${CRYPTOPP_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(CRYPTOPP_INSTALL_DIR_ABS "${CRYPTOPP_INSTALL_DIR}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-fetch.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/cryptopp" "${CRYPTOPP_INSTALL_DIR}"
    WORKING_DIRECTORY ${CRYPTOPP_3RDPARTY_DIR_ABS}
)

add_library(CryptoPPLib STATIC
    ${CORE_ROOT_DIR}/OfficeCryptReader/source/CryptTransform.cpp
    ${CORE_ROOT_DIR}/OfficeCryptReader/source/ECMACryptFile.cpp

    ${CORE_ROOT_DIR}/OfficeCryptReader/source/ECMACryptFile.h
    ${CORE_ROOT_DIR}/OfficeCryptReader/source/CryptTransform.h
)

set_default_options(CryptoPPLib)

set_property(TARGET CryptoPPLib PROPERTY POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(CryptoPPLib PRIVATE
    CRYPTOPPLIB_LIBRARY
    DISABLE_TYPE_MISMATCH
)

if(LINUX)
    target_compile_options(CryptoPPLib PRIVATE
        -UNDEBUG
    )
endif()

target_include_directories(CryptoPPLib PRIVATE
    ${CRYPTOPP_INSTALL_DIR_ABS}/include
)

target_link_libraries(CryptoPPLib PRIVATE
    ${CRYPTOPP_INSTALL_DIR_ABS}/lib/libcryptopp.a
)
