# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# Use a core package (scripts/build.sh or scripts/core_package.py fetch)
# instead of building core: common.cmake includes this when
# EO_CORE_PREBUILT_DIR is set. It defines the core targets as IMPORTED
# targets under their usual names, so the `if(NOT TARGET ...)
# add_subdirectory(...)` blocks of the including project skip them, and
# copies the package's x2t, libraries and tools into EO_CORE_OUTPUT_DIR and
# EO_CORE_TOOLS_DIR at build time, where a source build would put them.
# Headers still come from this core checkout, which must be the one the
# package was built from.

# Script mode: the copy step run by eo_core_prebuilt_files below
if(CMAKE_SCRIPT_MODE_FILE)
    # file(COPY) keeps symlinks (ICU's versioned names, frameworks)
    file(GLOB _entries LIST_DIRECTORIES true "${SRC}/*")
    file(MAKE_DIRECTORY "${DEST}")
    file(COPY ${_entries} DESTINATION "${DEST}" USE_SOURCE_PERMISSIONS)
    string(REPLACE "|" ";" _tools "${TOOLS}")
    foreach(_f IN LISTS _tools)
        file(COPY "${TOOLS_SRC}/${_f}" DESTINATION "${TOOLS_DEST}" USE_SOURCE_PERMISSIONS)
    endforeach()
    return()
endif()

get_filename_component(_eo_pkg "${EO_CORE_PREBUILT_DIR}" ABSOLUTE)
set(_eo_targets_file "${_eo_pkg}/lib/eo-core-targets.cmake")
if(NOT EXISTS "${_eo_targets_file}")
    message(FATAL_ERROR "EO_CORE_PREBUILT_DIR=${EO_CORE_PREBUILT_DIR} is not a core package: no lib/eo-core-targets.cmake")
endif()
include("${_eo_targets_file}")
if(NOT EO_CORE_PACKAGE_FORMAT EQUAL 1)
    message(FATAL_ERROR "${_eo_targets_file}: unsupported package format ${EO_CORE_PACKAGE_FORMAT}")
endif()
message(STATUS "Using prebuilt core from ${_eo_pkg}")

set(_eo_tools)
foreach(_t IN LISTS EO_CORE_PACKAGE_TARGETS)
    if(TARGET ${_t})
        continue()
    endif()
    set(_type "${EO_CORE_PACKAGE_${_t}_TYPE}")
    if(_type STREQUAL "EXECUTABLE")
        add_executable(${_t} IMPORTED GLOBAL)
    elseif(_type STREQUAL "STATIC_LIBRARY")
        add_library(${_t} STATIC IMPORTED GLOBAL)
    else()
        add_library(${_t} SHARED IMPORTED GLOBAL)
    endif()
    set_target_properties(${_t} PROPERTIES IMPORTED_LOCATION "${_eo_pkg}/${EO_CORE_PACKAGE_${_t}_FILE}")
    if(EO_CORE_PACKAGE_${_t}_IMPLIB)
        set_target_properties(${_t} PROPERTIES IMPORTED_IMPLIB "${_eo_pkg}/${EO_CORE_PACKAGE_${_t}_IMPLIB}")
    endif()
    if(EO_CORE_PACKAGE_${_t}_FRAMEWORK)
        set_target_properties(${_t} PROPERTIES FRAMEWORK TRUE)
    endif()
    if(EO_CORE_PACKAGE_${_t}_LINK)
        set_target_properties(${_t} PROPERTIES INTERFACE_LINK_LIBRARIES "${EO_CORE_PACKAGE_${_t}_LINK}")
    endif()
    if(EO_CORE_PACKAGE_${_t}_FILE MATCHES "^${EO_CORE_PACKAGE_TOOLS}/([^/]+)$"
       AND NOT EO_CORE_PACKAGE_TOOLS STREQUAL EO_CORE_PACKAGE_BIN)
        list(APPEND _eo_tools "${CMAKE_MATCH_1}")
    endif()
endforeach()

string(REPLACE ";" "|" _eo_tools "${_eo_tools}")
if(NOT TARGET eo_core_prebuilt_files)
    add_custom_target(eo_core_prebuilt_files ALL
        COMMAND ${CMAKE_COMMAND}
            "-DSRC=${_eo_pkg}/${EO_CORE_PACKAGE_BIN}"
            "-DDEST=${EO_CORE_OUTPUT_DIR}"
            "-DTOOLS_SRC=${_eo_pkg}/${EO_CORE_PACKAGE_TOOLS}"
            "-DTOOLS_DEST=${EO_CORE_TOOLS_DIR}"
            "-DTOOLS=${_eo_tools}"
            -P "${CMAKE_CURRENT_LIST_FILE}"
        COMMENT "Copying the prebuilt core to ${EO_CORE_OUTPUT_DIR}"
        VERBATIM)
endif()
