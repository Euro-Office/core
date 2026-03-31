include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EO_CORE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/package" CACHE PATH "Where to place output files (absolute path recommended)")
set(EO_CORE_TOOLS_DIR  "${CMAKE_BINARY_DIR}/package" CACHE PATH "Where to place tools output files (absolute path recommended)")

set(EO_CORE_3RD_PARTY_DIR "${CMAKE_BINARY_DIR}/third_party" CACHE PATH "Where to place and build 3rd party projects (absolute path recommended)")
set(EO_CORE_3RD_PARTY_WORK_DIR "${EO_CORE_3RD_PARTY_DIR}/workdir" CACHE PATH "3rd party work dir for clone and build.")
set(EO_CORE_3RD_PARTY_INSTALL_DIR "${EO_CORE_3RD_PARTY_DIR}/install" CACHE PATH "3rd party install dir.")

# Do NOT auto-add absolute link directories to RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# Use INSTALL_RPATH even for build-tree binaries
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# =============================================================================
# Sanitizer Support
# =============================================================================
# Enable AddressSanitizer and UndefinedBehaviorSanitizer for debug builds.
# Usage: cmake -DENABLE_SANITIZERS=ON ...
# Suppression files: .sisyphus/suppressions/asan.supp, ubsan.supp
# =============================================================================
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled: ASAN + UBSAN")
    set(ENABLE_SANITIZER_FLAGS
        -fsanitize=address
        -fsanitize=undefined
        -fno-sanitize-recover=all
        -g
    )
    # -fsanitize-minimal-runtime is clang-only; skip for GCC
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND ENABLE_SANITIZER_FLAGS -fsanitize-minimal-runtime)
    endif()
    # Set environment variable defaults for suppressions
    if(NOT DEFINED ENV{ASAN_OPTIONS})
        set(ENV{ASAN_OPTIONS} "suppressions=.sisyphus/suppressions/asan.supp:detect_leaks=1")
    endif()
    if(NOT DEFINED ENV{UBSAN_OPTIONS})
        set(ENV{UBSAN_OPTIONS} "suppressions=.sisyphus/suppressions/ubsan.supp:print_stacktrace=1")
    endif()
endif()

# =============================================================================
# Coverage Support
# =============================================================================
# Enable code coverage reporting with gcovr.
# Usage: cmake -DENABLE_COVERAGE=ON ...
# Then: cmake --build <build-dir> --target coverage
# =============================================================================
option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE)
    message(STATUS "Coverage enabled: adding --coverage flags")
    set(ENABLE_COVERAGE_FLAGS
        --coverage
        -fprofile-arcs
        -ftest-coverage
        -g
    )
    set(ENABLE_COVERAGE_LINK_FLAGS
        --coverage
        -lgcov
    )
endif()

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

if(ENABLE_SANITIZERS)
    list(APPEND COMMON_CXX_FLAGS ${ENABLE_SANITIZER_FLAGS})
endif()

if(ENABLE_COVERAGE)
    list(APPEND COMMON_CXX_FLAGS ${ENABLE_COVERAGE_FLAGS})
endif()

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

if(ENABLE_SANITIZERS)
    list(APPEND COMMON_C_FLAGS ${ENABLE_SANITIZER_FLAGS})
endif()

if(ENABLE_COVERAGE)
    list(APPEND COMMON_C_FLAGS ${ENABLE_COVERAGE_FLAGS})
endif()


set(COMMON_LINK_OPTIONS
    "-Wl,--disable-new-dtags"
)

if(ENABLE_SANITIZERS)
    list(APPEND COMMON_LINK_OPTIONS
        -fsanitize=address
        -fsanitize=undefined
    )
endif()

if(ENABLE_COVERAGE)
    list(APPEND COMMON_LINK_OPTIONS ${ENABLE_COVERAGE_LINK_FLAGS})
endif()


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


function(copy_artifacts_to_folder artifacts dest_dir)
    foreach(artifact ${artifacts})
        add_custom_command(TARGET ${artifact} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${dest_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${artifact}> "${dest_dir}/"
            COMMENT "Copying ${artifact} to ${dest_dir}"
        )
    endforeach()
endfunction()

function(copy_icu_libs artifact)
    add_custom_command(TARGET ${artifact} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${EO_CORE_OUTPUT_DIR}"
        COMMAND /bin/sh -c "cp -P --update=none \"${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu/lib\"/*.so* \"${EO_CORE_OUTPUT_DIR}/\""
        COMMENT "Copying ICU libs to ${EO_CORE_OUTPUT_DIR}"
    )
endfunction()

# =============================================================================
# Coverage Report Target
# =============================================================================
# Runs gcovr to generate coverage reports (XML + HTML).
# Vendored/third-party code is excluded from coverage metrics.
# =============================================================================
if(ENABLE_COVERAGE)
    find_program(GCOVR_PROGRAM gcovr)
    if(GCOVR_PROGRAM)
        add_custom_target(coverage
            COMMAND ${GCOVR_PROGRAM} -r ${CMAKE_SOURCE_DIR}
                --exclude '.*3dParty.*'
                --exclude '.*libxml2.*'
                --exclude '.*freetype.*'
                --exclude '.*openjpeg.*'
                --exclude '.*googletest.*'
                --xml -o coverage.xml
                --html -o coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report with gcovr"
        )
    else()
        message(WARNING "gcovr not found. Install with: pip install gcovr (or apt install gcovr)")
    endif()
endif()