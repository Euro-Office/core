# toolchain-llvm-mingw.cmake
# Cross-compiling to Windows x86_64 from Linux using LLVM-MinGW

# Target system name & processor
SET(CMAKE_SYSTEM_NAME Windows)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)

# Path to your LLVM-MinGW folder
if( NOT DEFINED MINGW_PATH )
    SET(MINGW_PATH "llvm-mingw-20260311-msvcrt-ubuntu-22.04-x86_64")
endif()

# Path to MinGW Boost (the directory should contain a boost dir with all the includes)
if( NOT DEFINED MINGW_BOOST_PATH )
    SET(MINGW_BOOST_PATH "boost_1_90_0")
endif()

# C and C++ compilers
SET(CMAKE_C_COMPILER   "${MINGW_PATH}/bin/x86_64-w64-mingw32-gcc")
SET(CMAKE_CXX_COMPILER "${MINGW_PATH}/bin/x86_64-w64-mingw32-g++")

# Archiver and related tools
SET(CMAKE_AR  "${MINGW_PATH}/bin/x86_64-w64-mingw32-llvm-ar")
SET(CMAKE_RANLIB "${MINGW_PATH}/bin/x86_64-w64-mingw32-ranlib")
SET(CMAKE_LINKER "${MINGW_PATH}/bin/x86_64-w64-mingw32-ld")

# Optional: where to find Windows libraries/includes (for FindPackage)
SET(CMAKE_FIND_ROOT_PATH "${MINGW_PATH}/x86_64-w64-mingw32")

# Tell CMake to search root path first
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(LLVM_MINGW_CROSS TRUE)

include_directories(
    "${MINGW_BOOST_PATH}"
)
