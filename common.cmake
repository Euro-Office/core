include_guard(GLOBAL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(EO_CORE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/package" CACHE PATH "Where to place output files (absolute path recommended)")
set(EO_CORE_TOOLS_DIR  "${CMAKE_BINARY_DIR}/package" CACHE PATH "Where to place tools output files (absolute path recommended)")

set(EO_CORE_3RD_PARTY_DIR "${CMAKE_BINARY_DIR}/third_party" CACHE PATH "Where to place and build 3rd party projects (absolute path recommended)")
set(EO_CORE_3RD_PARTY_WORK_DIR "${EO_CORE_3RD_PARTY_DIR}/workdir" CACHE PATH "3rd party work dir for clone and build.")
set(EO_CORE_3RD_PARTY_INSTALL_DIR "${EO_CORE_3RD_PARTY_DIR}/install" CACHE PATH "3rd party install dir.")


include_directories(SYSTEM /build-cache-desktop/vcpkg_installed/x64-linux-dynamic/include)
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
    #-O2 # Remove for debugging
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
    #-O2 #Remove for debugging
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

function(copy_boost_libs artifact)
    add_custom_command(TARGET ${artifact} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${EO_CORE_OUTPUT_DIR}"
        COMMAND /bin/sh -c "cp -P --update=none \"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/boost/linux_64/lib\"/*.so* \"${EO_CORE_OUTPUT_DIR}/\""
        COMMENT "Copying Boost libs to ${EO_CORE_OUTPUT_DIR}"
    )
endfunction()

# Javascript bundler for wasm builds
function(inject_script TARGET_NAME TEMPLATE_FILE OUTPUT_FILE)
    cmake_parse_arguments(ARG "" "MODE" "REPLACEMENTS" ${ARGN})

    if(NOT ARG_MODE)
        set(ARG_MODE "POST_BUILD")
    endif()

    set(_helper "${CMAKE_CURRENT_BINARY_DIR}/inject_script.cmake")
    file(WRITE "${_helper}" [[
        file(READ "${T}" content)
        math(EXPR last "${COUNT} - 1")
        foreach(i RANGE ${last})
            file(READ "${S${i}}" s_data)
            string(REPLACE "${P${i}}" "${s_data}" content "${content}")
        endforeach()
        file(WRITE "${O}" "${content}")
    ]])

    set(_defs)
    set(_count 0)
    set(_pairs ${ARG_REPLACEMENTS})
    set(_script_files)
    while(_pairs)
        list(POP_FRONT _pairs _placeholder _script_file)
        list(APPEND _defs "-D" "P${_count}=${_placeholder}" "-D" "S${_count}=${_script_file}")
        list(APPEND _script_files "${_script_file}")
        math(EXPR _count "${_count} + 1")
    endwhile()

    set(_cmd
        COMMAND ${CMAKE_COMMAND}
            -D "T=${TEMPLATE_FILE}"
            -D "O=${OUTPUT_FILE}"
            -D "COUNT=${_count}"
            ${_defs}
            -P "${_helper}"
        COMMENT "Injecting scripts into ${TEMPLATE_FILE}"
        VERBATIM
    )

    if(ARG_MODE STREQUAL "PRE_BUILD")
        get_filename_component(_out_name "${OUTPUT_FILE}" NAME_WE)

        add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            ${_cmd}
            DEPENDS "${TEMPLATE_FILE}" ${_script_files}
        )
        add_custom_target(${TARGET_NAME}_inject_${_out_name} DEPENDS "${OUTPUT_FILE}")
        add_dependencies(${TARGET_NAME} ${TARGET_NAME}_inject_${_out_name})
    else()
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            ${_cmd}
        )
    endif()
endfunction()