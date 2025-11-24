cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(kernel)

set(CORE_ROOT_DIR "..")

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET libxml)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/xml/build/cmake libxml)
endif()

if(NOT TARGET OfficeUtils)
    add_subdirectory(${CORE_ROOT_DIR}/OfficeUtils OfficeUtils)
endif()

add_library(kernel SHARED
    kernel_config.h

    # BLOCKER
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/TemporaryCS.h
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/TemporaryCS.cpp

    # THREAD
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/BaseThread.h
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/BaseThreadMonitor.h
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/BaseThread.cpp
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/BaseThreadMonitor.cpp

    # TIMER
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/Timer.h
    ${CORE_ROOT_DIR}/DesktopEditor/graphics/Timer.cpp

    # PATH
    ${CORE_ROOT_DIR}/DesktopEditor/common/Path.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/Path.cpp

    # STRINGS
    ${CORE_ROOT_DIR}/DesktopEditor/common/StringBuilder.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/StringBuilder.cpp
    ${CORE_ROOT_DIR}/DesktopEditor/common/StringExt.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/StringExt.cpp
    ${CORE_ROOT_DIR}/DesktopEditor/common/StringUTF32.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/StringUTF32.cpp

    # BYTE BUILDER
    ${CORE_ROOT_DIR}/DesktopEditor/common/ByteBuilder.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/ByteBuilder.cpp

    # BASE64
    ${CORE_ROOT_DIR}/DesktopEditor/common/Base64.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/Base64.cpp

    # FILE
    ${CORE_ROOT_DIR}/DesktopEditor/common/File.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/File.cpp

    ${CORE_ROOT_DIR}/DesktopEditor/common/StreamWriter.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/StreamWriter.cpp

    # DIRECTORY
    ${CORE_ROOT_DIR}/DesktopEditor/common/Directory.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/Directory.cpp

    # SYSTEM
    ${CORE_ROOT_DIR}/DesktopEditor/common/SystemUtils.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/SystemUtils.cpp

    # PROCESSENV
    ${CORE_ROOT_DIR}/DesktopEditor/common/ProcessEnv.h
    ${CORE_ROOT_DIR}/DesktopEditor/common/ProcessEnv.cpp
)

target_compile_definitions(kernel PRIVATE
    KERNEL_USE_DYNAMIC_LIBRARY_BUILDING
    BUILD_ZLIB_AS_SOURCES
)

# target_include_directories(kernel PRIVATE
# )

# static libs
target_link_libraries(kernel PRIVATE
    libxml
    OfficeUtils
)

# dynamic libs
target_link_libraries(kernel PUBLIC
    UnicodeConverter
)

target_compile_options(kernel PRIVATE
    -include ${CMAKE_CURRENT_SOURCE_DIR}/posix_compat.h
)

# TODO
# set_target_properties(kernel PROPERTIES
#     BUILD_RPATH "/path/to/libs"
#     INSTALL_RPATH "/path/to/libs"
# )
