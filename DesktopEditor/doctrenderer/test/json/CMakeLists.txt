cmake_minimum_required(VERSION 3.10)

project(doctrenderer_json_test)

# test/json is 4 levels under the core root.
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies (mirror ADD_DEPENDENCY(doctrenderer) in test.pro).
if(NOT TARGET doctrenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/doctrenderer doctrenderer)
endif()

# main.cpp guards its own int main() behind #ifndef JSON_GOOGLE_TEST, so with
# the define active the suite has no main() of its own -> use GTEST_MAIN.
add_core_gtest(
    NAME    doctrenderer_json_test
    SOURCES main.cpp
    LIBS    doctrenderer
    GTEST_MAIN
)

# CONFIG += json_google_test -> DEFINES += JSON_GOOGLE_TEST
target_compile_definitions(doctrenderer_json_test PRIVATE
    JSON_GOOGLE_TEST
)

# INCLUDEPATH += ../.. (the doctrenderer source root, for json/serialization.h etc.)
target_include_directories(doctrenderer_json_test PRIVATE
    ${CORE_ROOT_DIR}/DesktopEditor/doctrenderer
)

# core_linux { LIBS += -Wl,-unresolved-symbols=ignore-in-shared-libs -ldl }
if(LINUX)
    target_link_options(doctrenderer_json_test PRIVATE
        "-Wl,-unresolved-symbols=ignore-in-shared-libs"
    )
    target_link_libraries(doctrenderer_json_test PRIVATE dl)
endif()
