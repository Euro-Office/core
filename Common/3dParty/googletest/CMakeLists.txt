cmake_minimum_required(VERSION 3.10)

# Prevent GoogleTest from overriding our compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Build gtest and gtest_main as static libraries (default behavior)
add_subdirectory(googletest)

# Smoke test executable
add_executable(gtest_smoke_test test/smoke_test.cpp)
target_link_libraries(gtest_smoke_test PRIVATE gtest gtest_main)
set_default_options(gtest_smoke_test)

include(GoogleTest)
gtest_discover_tests(gtest_smoke_test)
