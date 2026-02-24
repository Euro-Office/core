cmake_minimum_required(VERSION 3.15)
project(hunspell_wrapper LANGUAGES CXX)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# 1. Find the Conan-provided Hunspell
find_package(hunspell REQUIRED)

# 2. Define your wrapper library
# This replaces your old 'add_library(hunspell ...)'
add_library(hunspell_wrapper INTERFACE) 

# 4. Include your internal headers
target_include_directories(hunspell_wrapper INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/../wasm/src"
)

# 5. Link to the official Hunspell (handled by Conan)
# This automatically handles the include paths and compiler flags for Hunspell
target_link_libraries(hunspell_wrapper INTERFACE hunspell::hunspell)

# 6. Keep your specific macro if needed
target_compile_definitions(hunspell_wrapper INTERFACE BUILDING_LIBHUNSPELL)