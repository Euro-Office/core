cmake_minimum_required(VERSION 3.10)

project(test_odf)

# This directory is three levels under the repository root
# (OdfFile/Test/test_odf).
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# Dependencies. test_odf.pro builds only test.cpp + common.cpp (the
# entrance/motion/audio/interactions modules are commented out) and converts an
# .odp fixture via ConvertODF2OOXml() + COfficeFileFormatChecker.
#
# ConvertODF2OOXml() lives in OdfFormatLib; COfficeFileFormatChecker is compiled
# from OfficeFileFormatChecker2.cpp (+ pole.cpp + unicode_util.cpp) directly into
# the target, mirroring how Test/Applications/x2tTester does it. kernel/graphics/
# UnicodeConverter satisfy the rest of the chain.
if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()
if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()
if(NOT TARGET graphics)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/graphics/cmake graphics)
endif()
if(NOT TARGET OdfFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OdfFile/Projects/Linux OdfFormatLib)
endif()

# The test reads its fixture via the working-dir-relative path
# "ExampleFiles/motion.odp" (TestEnv passes it straight to convert_single()).
# Stage the committed ExampleFiles next to the binary and run the test from that
# staging dir so the relative path resolves.
set(_test_odf_run_dir "${CMAKE_CURRENT_BINARY_DIR}/run")

# test.cpp owns main() (InitGoogleTest + AddGlobalTestEnvironment + RUN_ALL_TESTS)
# -> do NOT pass GTEST_MAIN.
add_core_gtest(
    NAME    test_odf
    SOURCES test.cpp
            common.cpp
            ${CORE_ROOT_DIR}/Common/3dParty/pole/pole.cpp
            ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
            ${CORE_ROOT_DIR}/OOXML/Base/unicode_util.cpp
    LIBS    OdfFormatLib kernel graphics UnicodeConverter
    WORKING_DIRECTORY "${_test_odf_run_dir}"
)

# test_odf.pro adds the core root (for the rewritten relative includes such as
# "Common/OfficeFileFormatChecker.h") and X2tConverter/src on the include path.
target_include_directories(test_odf PRIVATE
    "${CORE_ROOT_DIR}"
    "${CORE_ROOT_DIR}/X2tConverter/src"
)

add_custom_command(TARGET test_odf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_test_odf_run_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ExampleFiles"
            "${_test_odf_run_dir}/ExampleFiles"
    COMMENT "Staging test_odf fixtures"
)
