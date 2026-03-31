cmake_minimum_required(VERSION 3.10)

project(openssl_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(OPENSSL_COMMON_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/../common")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

message( "Fetch and build OpenSSL" )
set(OPENSSL_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/openssl")
set(OPENSSL_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/openssl_build_linux_64")
get_filename_component(OPENSSL_3RDPARTY_DIR_ABS "${OPENSSL_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(OPENSSL_INSTALL_DIR_ABS "${OPENSSL_INSTALL_DIR}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-build.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/openssl" "${OPENSSL_INSTALL_DIR}"
    WORKING_DIRECTORY ${OPENSSL_3RDPARTY_DIR_ABS}
)


add_executable(openssl_test
    ${OPENSSL_COMMON_SRC_DIR}/common_openssl.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp

    ${OPENSSL_COMMON_SRC_DIR}/common_openssl.h
)

set_default_options(openssl_test)

target_include_directories(openssl_test PRIVATE
    ${OPENSSL_INSTALL_DIR}/include
)

target_link_libraries(openssl_test PRIVATE
    ${OPENSSL_INSTALL_DIR_ABS}/lib64/libssl.a
    ${OPENSSL_INSTALL_DIR_ABS}/lib64/libcrypto.a
)

target_link_libraries(openssl_test PUBLIC
    kernel
)

copy_icu_libs(openssl_test)



target_link_libraries(openssl_test PRIVATE doctest::doctest)

# Modify rpath so test binary can run properly
get_target_property(old_rpath openssl_test BUILD_RPATH)
if(old_rpath)
    set(new_rpath "${old_rpath}:${EO_CORE_OUTPUT_DIR}")
else()
    set(new_rpath "${EO_CORE_OUTPUT_DIR}")
endif()
set_target_properties(openssl_test PROPERTIES
    BUILD_RPATH "${new_rpath}"
    INSTALL_RPATH "${new_rpath}"
)

register_test_target(openssl_tests)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(openssl_test
     TEST_PREFIX "openssl."
     DISCOVERY_MODE PRE_TEST
)
