cmake_minimum_required(VERSION 3.10)

project(doctrenderer_engine_test)

# test/engine is 4 levels under the core root.
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET doctrenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/doctrenderer doctrenderer)
endif()

add_core_gtest(
    NAME    doctrenderer_engine_test
    SOURCES main.cpp
    LIBS    doctrenderer
    GTEST_MAIN
)

# for js_internal/js_base.h and js_internal/js_embed.h
target_include_directories(doctrenderer_engine_test PRIVATE
    ${CORE_ROOT_DIR}/DesktopEditor/doctrenderer
    ${CORE_ROOT_DIR}/DesktopEditor/doctrenderer/js_internal
)

if(LINUX)
    target_link_options(doctrenderer_engine_test PRIVATE
        "-Wl,-unresolved-symbols=ignore-in-shared-libs"
    )
    target_link_libraries(doctrenderer_engine_test PRIVATE dl pthread)
endif()
