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

target_link_libraries(xmlsec_test PUBLIC
    kernel
    ooxmlsignature
    ${LIBICUUC}
    ${LIBICUDATA}
)

copy_artifacts_to_folder("xmlsec_test" "${EO_CORE_OUTPUT_DIR}")

# main.cpp reads/writes test fixtures by bare relative name (cwd-relative,
# not tied to the executable's own location), so they need to sit next to it
# in EO_CORE_OUTPUT_DIR too. legacy-signed.docx (if present) is signed by
# upstream ONLYOFFICE Desktop Editors 9.4.0, confirmed (via build_tools'
# openssl.py) to still be pinned to OpenSSL_1_1_1f - committed as a
# permanent regression fixture proving documents signed under OpenSSL 1.1.1
# still verify under this fork's 4.0.1 (USE_VERIFY_LEGACY in main.cpp).
set(XMLSEC_TEST_FIXTURE_COPY_COMMANDS
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EO_CORE_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/file.docx" "${EO_CORE_OUTPUT_DIR}/"
)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/legacy-signed.docx")
    list(APPEND XMLSEC_TEST_FIXTURE_COPY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/legacy-signed.docx" "${EO_CORE_OUTPUT_DIR}/"
    )
endif()

add_custom_command(TARGET xmlsec_test POST_BUILD
    ${XMLSEC_TEST_FIXTURE_COPY_COMMANDS}
    COMMENT "Copying test fixtures to ${EO_CORE_OUTPUT_DIR}"
)

declare_victory( xmlsec_test )
