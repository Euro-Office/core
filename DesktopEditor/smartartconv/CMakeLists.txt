cmake_minimum_required(VERSION 3.10)

project(smartartconv)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET kernel_network)
    add_subdirectory(${CORE_ROOT_DIR}/Common/Network kernel_network)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()

if(NOT TARGET CryptoPPLib)
    add_subdirectory(${CORE_ROOT_DIR}/Common/3dParty/cryptopp/project CryptoPPLib)
endif()

if(NOT TARGET CompoundFileLib)
    add_subdirectory(${CORE_ROOT_DIR}/Common/cfcpp CompoundFileLib)
endif()

if(NOT TARGET XlsFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/MsBinaryFile/Projects/XlsFormatLib/Linux/cmake XlsFormatLib)
endif()

if(NOT TARGET DocxFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/DocxFormatLib DocxFormatLib)
endif()

if(NOT TARGET PPTXFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/PPTXFormatLib PPTXFormatLib)
endif()

if(NOT TARGET XlsbFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/XlsbFormatLib XlsbFormatLib)
endif()

if(NOT TARGET BinDocument)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/BinDocument BinDocument)
endif()

# ICU — required by kernel
message("Fetch and build icu (smartartconv)")
set(ICU_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/icu")
set(ICU_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu")
set(ICU_MAJOR_VER "74")
set(ICU_MINOR_VER "2")
get_filename_component(ICU_3RDPARTY_DIR_ABS "${ICU_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
message(STATUS "ICU script dir: ${ICU_3RDPARTY_DIR_ABS}")
message(STATUS "ICU work dir: ${EO_CORE_3RD_PARTY_WORK_DIR}/icu")
message(STATUS "ICU install dir: ${ICU_INSTALL_DIR_ABS}")
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-build.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/icu" "${ICU_INSTALL_DIR}" ${ICU_MAJOR_VER} ${ICU_MINOR_VER}
    WORKING_DIRECTORY ${ICU_3RDPARTY_DIR_ABS}
    RESULT_VARIABLE ICU_BUILD_RESULT
)
if(NOT ICU_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to fetch/build ICU for smartartconv (exit code: ${ICU_BUILD_RESULT})")
endif()

add_executable(smartartconv
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

set_default_options(smartartconv)

target_include_directories(smartartconv PRIVATE
    ${CORE_ROOT_DIR}
    ${CORE_ROOT_DIR}/OOXML
    ${CORE_ROOT_DIR}/DesktopEditor
)

target_link_libraries(smartartconv PRIVATE
    BinDocument
    PPTXFormatLib
    DocxFormatLib
    XlsbFormatLib
    XlsFormatLib
    CompoundFileLib
    CryptoPPLib
)

target_link_libraries(smartartconv PUBLIC
    graphics
    kernel
    kernel_network
    UnicodeConverter
    ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
    ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
)

copy_artifacts_to_folder("smartartconv" "${EO_CORE_TOOLS_DIR}")
