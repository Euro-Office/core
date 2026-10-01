# Run via: cmake -DFILE=<path> -P CheckNoDeprecatedDuckDbApi.cmake
# Fails (non-zero exit) if DuckDbEngine.cpp calls either deprecated DuckDB
# C API function (FR-016, T035). A plain string search, not a regex, so a
# comment mentioning the function names (as this repo's own commit
# messages and code comments do, deliberately, to document the fix) does
# not itself trip the check -- it only flags an actual call site, i.e. the
# function name immediately followed by "(".

if(NOT EXISTS "${FILE}")
    message(FATAL_ERROR "CheckNoDeprecatedDuckDbApi: file not found: ${FILE}")
endif()

file(STRINGS "${FILE}" LINES)

set(DEPRECATED_CALLS "duckdb_row_count(" "duckdb_value_varchar(")
set(VIOLATIONS "")

foreach(LINE ${LINES})
    # Skip comment lines (// ...), which may legitimately mention the names.
    string(REGEX REPLACE "//.*$" "" CODE_PART "${LINE}")
    foreach(CALL ${DEPRECATED_CALLS})
        string(FIND "${CODE_PART}" "${CALL}" POS)
        if(NOT POS EQUAL -1)
            list(APPEND VIOLATIONS "${LINE}")
        endif()
    endforeach()
endforeach()

list(LENGTH VIOLATIONS VIOLATION_COUNT)
if(VIOLATION_COUNT GREATER 0)
    message(FATAL_ERROR "DuckDbEngine.cpp calls a deprecated DuckDB API function (FR-016): ${VIOLATIONS}")
endif()
