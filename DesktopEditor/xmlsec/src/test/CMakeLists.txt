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

# main.cpp reads/writes test fixtures by bare relative name (cwd-relative,
# not tied to the executable's own location), so they sit next to the test
# binary, which runs there. Not in EO_CORE_OUTPUT_DIR: that is what ships.
# legacy-signed.docx is signed by upstream
# ONLYOFFICE Desktop Editors 9.4.0, confirmed (via build_tools' openssl.py)
# to still be pinned to OpenSSL_1_1_1f - committed as a permanent regression
# fixture proving documents signed under OpenSSL 1.1.1 still verify under
# this fork's 4.0.1 (USE_VERIFY_LEGACY in main.cpp).
add_custom_command(TARGET xmlsec_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/file.docx" "$<TARGET_FILE_DIR:xmlsec_test>/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/legacy-signed.docx" "$<TARGET_FILE_DIR:xmlsec_test>/"
    COMMENT "Copying test fixtures next to xmlsec_test"
)

# Register with CTest (see TESTING.md). Unlike the other suites there, this
# one is a plain main()/exit-code test, not GoogleTest. Like them, it loads
# the core libraries (kernel, ooxmlsignature) from EO_CORE_OUTPUT_DIR.
# enable_testing() is technically redundant when reached via the top-level
# CMakeLists.txt (which already calls it), but kept here - harmless to call
# twice - so this suite alone can still be configured and ctest-run standalone.
enable_testing()
add_test(NAME xmlsec_test
    COMMAND xmlsec_test
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:xmlsec_test>")
if(APPLE)
    set_tests_properties(xmlsec_test PROPERTIES
        ENVIRONMENT "DYLD_LIBRARY_PATH=${EO_CORE_OUTPUT_DIR}:$ENV{DYLD_LIBRARY_PATH};DYLD_FRAMEWORK_PATH=${EO_CORE_OUTPUT_DIR}:$ENV{DYLD_FRAMEWORK_PATH}")
else()
    set_tests_properties(xmlsec_test PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${EO_CORE_OUTPUT_DIR}:$ENV{LD_LIBRARY_PATH}")
endif()

declare_victory( xmlsec_test )
