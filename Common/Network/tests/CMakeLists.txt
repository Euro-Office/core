# SPDX-FileCopyrightText: 2026 Euro-Office contributors
# SPDX-License-Identifier: AGPL-3.0-only

add_core_gtest(
    NAME     kernel_network_socketio_url_test
    SOURCES  ${CMAKE_CURRENT_LIST_DIR}/socketio_url_test.cpp
    GTEST_MAIN
)
