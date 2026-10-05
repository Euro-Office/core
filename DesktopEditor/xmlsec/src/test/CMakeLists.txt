cmake_minimum_required(VERSION 3.10)

project(xmlsec_test)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET ooxmlsignature)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src ooxmlsignature)
endif()

add_executable(xmlsec_test
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set_default_options(xmlsec_test)

target_include_directories(xmlsec_test PRIVATE
    ${ICU_INSTALL_DIR_ABS}/include
)

target_link_libraries(xmlsec_test PRIVATE
    kernel
    ooxmlsignature
    ${LIBICUUC}
    ${LIBICUDATA}
)

copy_artifacts_to_folder("xmlsec_test" "${EO_CORE_OUTPUT_DIR}")

# main.cpp reads/writes test fixtures by bare relative name (cwd-relative,
# not tied to the executable's own location), so they need to sit next to it
# in EO_CORE_OUTPUT_DIR too. legacy-signed.docx is signed by upstream
# ONLYOFFICE Desktop Editors 9.4.0, confirmed (via build_tools' openssl.py)
# to still be pinned to OpenSSL_1_1_1f - committed as a permanent regression
# fixture proving documents signed under OpenSSL 1.1.1 still verify under
# this fork's 4.0.1 (USE_VERIFY_LEGACY in main.cpp).
add_custom_command(TARGET xmlsec_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EO_CORE_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/file.docx" "${EO_CORE_OUTPUT_DIR}/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/legacy-signed.docx" "${EO_CORE_OUTPUT_DIR}/"
    COMMENT "Copying test fixtures to ${EO_CORE_OUTPUT_DIR}"
)

# Register with CTest (see TESTING.md). Unlike the other suites there, this
# one is a plain main()/exit-code test, not GoogleTest - and it must run the
# copy of the binary in EO_CORE_OUTPUT_DIR (not the raw build-tree one from
# $<TARGET_FILE:xmlsec_test>), since that's the only place its fixtures and
# RPATH-resolved deps (kernel, ooxmlsignature.framework) are colocated.
# enable_testing() is technically redundant when reached via the top-level
# CMakeLists.txt (which already calls it), but kept here - harmless to call
# twice - so this suite alone can still be configured and ctest-run standalone.
enable_testing()
add_test(NAME xmlsec_test
    COMMAND "${EO_CORE_OUTPUT_DIR}/$<TARGET_FILE_NAME:xmlsec_test>"
    WORKING_DIRECTORY "${EO_CORE_OUTPUT_DIR}")
set_tests_properties(xmlsec_test PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${EO_CORE_OUTPUT_DIR}:$ENV{LD_LIBRARY_PATH}")

declare_victory( xmlsec_test )
