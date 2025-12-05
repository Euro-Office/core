include_guard(GLOBAL)

# Note: ignored bundle_dylibs

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Do NOT auto-add absolute link directories to RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# Use INSTALL_RPATH even for build-tree binaries
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# Enable color diagnostics but only in interactive terminals
if(CMAKE_GENERATOR MATCHES "Ninja|Unix Makefiles")
    if(DEFINED ENV{TERM})
        # Simple check for common interactive terminals
        if(NOT "$ENV{TERM}" STREQUAL "dumb")
            message(STATUS "Enabling colored diagnostics for interactive terminal")
            set(CMAKE_COLOR_DIAGNOSTICS ON CACHE BOOL "Enable colored compiler output" FORCE)
        endif()
    endif()
endif()

set(COMMON_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")
file(READ "${COMMON_CMAKE_DIR}/Common/version.txt" VERSION_TXT_CONTENT)

set(COMMON_DEFINES
    _LINUX
    _REENTRANT
    CRYPTOPP_DISABLE_ASM
    INTVER=${VERSION_TXT_CONTENT}
    LINUX

    # Not sure about these:
    _UNICODE
    DONT_WRITE_EMBEDDED_FONTS
    UNICODE
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND COMMON_DEFINES
        _DEBUG
    )
endif()



set(COMMON_CXX_FLAGS
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wall
    -Wextra
    -Wno-ignored-qualifiers
    -Wno-register
    -Wno-unused-variable # TODO remove later; These are just here to reduce the clutter
    -Wno-unused-function # TODO remove later; These are just here to reduce the clutter
    -Wno-unused-parameter # TODO remove later; These are just here to reduce the clutter
    -O2
)

set(COMMON_C_FLAGS
    -fvisibility=hidden
    # -fvisibility-inlines-hidden
    -Wall
    -Wextra
    -Wno-ignored-qualifiers
    # -Wno-register
    -Wno-implicit-function-declaration
    -Wno-unused-variable # TODO remove later; These are just here to reduce the clutter
    -Wno-unused-function # TODO remove later; These are just here to reduce the clutter
    -Wno-unused-parameter # TODO remove later; These are just here to reduce the clutter
    -O2
)


set(COMMON_LINK_OPTIONS
    "-Wl,--disable-new-dtags"
)


function(set_default_options target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "set_default_options(): Target '${target}' does not exist yet.")
    endif()

    # Base RPATHs
    set_property(TARGET ${target} PROPERTY BUILD_RPATH "\$ORIGIN;\$ORIGIN/system")
    set_property(TARGET ${target} PROPERTY INSTALL_RPATH "\$ORIGIN;\$ORIGIN/system")

    # Optional: additional runtime paths from env variable RUN_PATH_ADDON
    if(DEFINED ENV{RUN_PATH_ADDON})
        set(RUN_PATH_ADDON "$ENV{RUN_PATH_ADDON}")
        string(REPLACE ";;" ";" RUN_PATH_ADDON_LIST "${RUN_PATH_ADDON}")

        set_property(TARGET ${target} APPEND PROPERTY INSTALL_RPATH "${RUN_PATH_ADDON_LIST}")
    endif()

    # C++ flags
    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:${COMMON_CXX_FLAGS}>
    )

    # C flags
    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:C>:${COMMON_C_FLAGS}>
    )

    target_compile_definitions(${target} PRIVATE
        ${COMMON_DEFINES}
    )

    target_link_options(${target} PRIVATE
        ${COMMON_LINK_OPTIONS}
    )
endfunction()
