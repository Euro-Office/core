cmake_minimum_required(VERSION 3.16)

project(cmapbin LANGUAGES CXX)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

# Define the executable
add_executable(CMapSerialize
    CMapSerialize.cpp
)

set_default_options(CMapSerialize)

# Equivalent to ADD_DEPENDENCY(UnicodeConverter, kernel)
# You'll need to make sure these targets are defined in your base.cmake
target_link_libraries(CMapSerialize PRIVATE 
    UnicodeConverter 
    kernel
)

copy_artifacts_to_folder("CMapSerialize" "${EO_CORE_OUTPUT_DIR}")

set(ARTIFACT_FILE "${EO_CORE_OUTPUT_DIR}/cmap.bin")

add_custom_command(
    OUTPUT "${ARTIFACT_FILE}"
    COMMAND ${EO_CORE_OUTPUT_DIR}/CMapSerialize "${CMAKE_CURRENT_SOURCE_DIR}/../CMap/CMap"
    COMMAND rm ${EO_CORE_OUTPUT_DIR}/CMapSerialize
    DEPENDS CMapSerialize
    COMMENT "Executing CMapSerialize to create ${ARTIFACT_FILE}"
    VERBATIM
)

add_custom_target(cmapbin ALL
    DEPENDS "${ARTIFACT_FILE}"
)
