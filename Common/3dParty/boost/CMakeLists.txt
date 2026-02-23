cmake_minimum_required(VERSION 3.15)

project(BoostCustomBuild LANGUAGES CXX)

# --- Options ---
option(BOOST_ENABLE_DEBUG_ITERATORS "Enable debug iterators" OFF) # Defaults to 0 based on your script
set(CMAKE_CXX_STANDARD 14) # Boost 1.72 requires at least C++11, usually C++14/17 for modern usage
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Fetch Boost Source (Replaces git clone logic) ---
include(FetchContent)

message(STATUS "Fetching Boost 1.72.0...")
FetchContent_Declare(
    boost_source
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG        boost-1.72.0
    GIT_SHALLOW    TRUE
    # We only need specific submodules to save time/space
    # GIT_SUBMODULES libs/filesystem libs/system libs/regex libs/date_time libs/config libs/core libs/assert libs/type_traits libs/mpl libs/predef libs/smart_ptr libs/static_assert libs/detail libs/io libs/iterator libs/lexical_cast libs/range libs/bind libs/exception libs/container libs/intrusive libs/move libs/utility libs/winapi libs/algorithm libs/concept
)
FetchContent_MakeAvailable(boost_source)

# --- 2. Helper Macro to Create Targets (Replaces boost_qt.py loop) ---
function(create_boost_target MODULE_NAME)
    set(LIB_NAME "boost_${MODULE_NAME}")
    
    # Mimics: SOURCES += $$files($$PWD/src/*.cpp, true)
    file(GLOB_RECURSE SOURCES 
        "${boost_source_SOURCE_DIR}/libs/${MODULE_NAME}/src/*.cpp"
    )

    add_library(${LIB_NAME} STATIC ${SOURCES})

    set_target_properties(${LIB_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    
    # Mimics: INCLUDEPATH += $$BOOST_SOURCES and $$PWD/include
    target_include_directories(${LIB_NAME} PUBLIC
        "$<BUILD_INTERFACE:${boost_source_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${boost_source_SOURCE_DIR}/libs/${MODULE_NAME}/include>"
        "$<BUILD_INTERFACE:${boost_source_SOURCE_DIR}/libs/io/include>"
        "$<INSTALL_INTERFACE:include>"
    )
    
    # Apply platform specific settings defined in your original python scripts
    
    # Windows specific (from boost.py)
    if(MSVC)
        # Mimics: define=_ITERATOR_DEBUG_LEVEL=0
        if(NOT BOOST_ENABLE_DEBUG_ITERATORS)
            target_compile_definitions(${LIB_NAME} PUBLIC _ITERATOR_DEBUG_LEVEL=0)
        endif()
        # Common Boost MSVC warnings to suppress
        target_compile_options(${LIB_NAME} PRIVATE /wd4503 /wd4996)
    endif()

    # Android specific (from boost_qt.py)
    if(ANDROID)
        # Mimics: DEFINES += "_HAS_AUTO_PTR_ETC=0"
        target_compile_definitions(${LIB_NAME} PUBLIC _HAS_AUTO_PTR_ETC=0)
    endif()

    # Apple (macOS/iOS) specific (from boost.py clang_correct)
    if(APPLE)
        # Mimics: -fcoalesce-templates
        target_compile_options(${LIB_NAME} PUBLIC -fcoalesce-templates)
    endif()

    # Linux specific (from boost.py)
    if(UNIX AND NOT APPLE AND NOT ANDROID)
        # Mimics: cxxflags=-fPIC
        set_property(TARGET ${LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
    endif()
    
    # Installation Rules
    install(TARGETS ${LIB_NAME} 
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endfunction()

# --- 3. Build The Libraries ---

# System (Base dependency)
create_boost_target(system)

# Filesystem (Depends on System)
create_boost_target(filesystem)
target_link_libraries(boost_filesystem PUBLIC boost_system)

# Regex
create_boost_target(regex)
# Regex often needs access to configuration headers found in other modules
target_include_directories(boost_regex PRIVATE ${boost_source_SOURCE_DIR}/libs/regex/src)

# Date_Time
create_boost_target(date_time)

# --- 4. Install Headers (Replaces copy commands) ---
# This installs the headers to include/boost/...
install(DIRECTORY "${boost_source_SOURCE_DIR}/boost" 
    DESTINATION include
    FILES_MATCHING PATTERN "*"
)

message(STATUS "Boost configuration complete.")