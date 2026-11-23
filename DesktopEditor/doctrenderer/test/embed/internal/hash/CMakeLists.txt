cmake_minimum_required(VERSION 3.10)

project(doctrenderer_hash_test)

# test/embed/internal/hash is 6 levels under the core root.
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies (mirror ADD_DEPENDENCY(doctrenderer) in test.pro).
if(NOT TARGET doctrenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/doctrenderer doctrenderer)
endif()

# main.cpp has no int main() of its own (no *_GOOGLE_TEST guard) -> use GTEST_MAIN.
# No extra google-test define for this suite.
add_core_gtest(
    NAME    doctrenderer_hash_test
    SOURCES main.cpp
    LIBS    doctrenderer
    GTEST_MAIN
)

# INCLUDEPATH += $$CORE_ROOT_DIR/DesktopEditor/doctrenderer
# (for embed/Default.h, hash.h, js_internal/js_base.h)
target_include_directories(doctrenderer_hash_test PRIVATE
    ${CORE_ROOT_DIR}/DesktopEditor/doctrenderer
)

# core_linux { LIBS += -Wl,-unresolved-symbols=ignore-in-shared-libs -ldl }
if(LINUX)
    target_link_options(doctrenderer_hash_test PRIVATE
        "-Wl,-unresolved-symbols=ignore-in-shared-libs"
    )
    target_link_libraries(doctrenderer_hash_test PRIVATE dl)
endif()
