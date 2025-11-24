cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(x2t)

set(X2T_ROOT_DIR "../..")
set(CORE_ROOT_DIR "../../..")

file(READ "${CORE_ROOT_DIR}/Common/version.txt" VERSION_TXT_CONTENT)

add_subdirectory(library/static)

add_executable(x2t
    ${X2T_ROOT_DIR}/src/main.cpp
)

target_compile_definitions(x2t PRIVATE
    INTVER=${VERSION_TXT_CONTENT}
)


# static libs
target_link_libraries(x2t PRIVATE
    x2tlib
)

