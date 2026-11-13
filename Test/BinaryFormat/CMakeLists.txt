# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

# The binary format tools are plain Python and need no build: run their
# self-tests and the template snapshots under CTest as well. The enum check
# compares against a git base revision, so it stays in CI (binary-format.yml).
if(NOT PYTHON_BIN)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(PYTHON_BIN "${Python3_EXECUTABLE}")
endif()

add_test(NAME binformat_selftests
    COMMAND "${PYTHON_BIN}" -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}" -p "test_*.py")
add_test(NAME binformat_template_snapshots
    COMMAND "${PYTHON_BIN}" "${CMAKE_CURRENT_SOURCE_DIR}/run_snapshots.py" templates)
# Keep the source tree free of __pycache__.
set_tests_properties(binformat_selftests binformat_template_snapshots
    PROPERTIES ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
