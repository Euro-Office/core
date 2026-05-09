cmake_minimum_required(VERSION 3.10)
project(StarMathConverter VERSION 1)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Ensure Qt core and gui are NOT included automatically if using a Qt-aware environment
# (In CMake, you simply don't call find_package(Qt...) if you don't need it)

# Remove UNICODE definition if it was inherited from directory properties
remove_definitions(-DUNICODE -D_UNICODE)

# TEMPLATE = lib, CONFIG += shared plugin
# In CMake, MODULE is often used for plugins loaded at runtime, but SHARED is standard 
# for dynamic libraries linked at compile time.
add_library(StarMathConverter SHARED)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

# Set target version
set_target_properties(StarMathConverter PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# HEADERS and SOURCES
target_sources(StarMathConverter PRIVATE
    cconversionsmtoooxml.cpp
    ${CORE_ROOT_DIR}/OOXML/Base/Unit.cpp
	conversionmathformula.cpp 
    cstarmathpars.cpp
    
    TextDirection.h
    cconversionsmtoooxml.h
    ${CORE_ROOT_DIR}/OOXML/Base/Unit.h
	conversionmathformula.h
    cstarmathpars.h
    fontType.h
    typeConversion.h
    typeselements.h
    TFormulaSize.h
)


# ADD_DEPENDENCY(kernel)
# Replaces your custom kernel dependency. 
# Assumes 'kernel' is another CMake target defined elsewhere in your project.
target_link_libraries(StarMathConverter PRIVATE 
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
    kernel
)

# core_windows { ... }
if(WIN32)
    target_compile_definitions(StarMathConverter PRIVATE SUPPORT_MS_CRYPTO)
    target_sources(StarMathConverter PRIVATE src/Certificate_mscrypto.h)
    
    # LIBS += -lcrypt32 -lcryptui -lAdvapi32 -lws2_32 -lUser32
    target_link_libraries(StarMathConverter PRIVATE
        crypt32
        cryptui
        advapi32
        ws2_32
        user32
    )
endif()

copy_artifacts_to_folder("StarMathConverter" "${EO_CORE_OUTPUT_DIR}")

declare_victory( StarMathConverter )
