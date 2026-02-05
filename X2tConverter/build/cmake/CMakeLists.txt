cmake_minimum_required(VERSION 3.10)

project(x2t)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

set(X2T_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

add_subdirectory(library)

add_executable(x2t
    ${X2T_ROOT_DIR}/src/main.cpp
)

set_default_options(x2t)

# dynamic libs
target_link_libraries(x2t PRIVATE
    x2tlib
)

copy_artifacts_to_folder("x2t" "${EO_CORE_OUTPUT_DIR}")