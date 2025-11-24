cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(MsBinaryFile)

add_subdirectory(Projects/DocFormatLib/Linux/cmake)
add_subdirectory(Projects/VbaFormatLib/Linux/cmake)
add_subdirectory(Projects/PPTFormatLib/Linux/cmake)
add_subdirectory(Projects/XlsFormatLib/Linux/cmake)
