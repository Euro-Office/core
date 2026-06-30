cmake_minimum_required(VERSION 3.10)

project(test_odf)

# OdfFile/Test/test_odf -> three levels under the repository root.
set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

# test_odf.pro builds only test.cpp + common.cpp (the entrance/motion/audio/
# interactions modules are commented out) and converts an .odp fixture via
# ConvertODF2OOXml() + COfficeFileFormatChecker.
#
# ConvertODF2OOXml() lives in OdfFormatLib, whose ODF->OOXML writer references
# the whole OOXML/MS format-library set (XmlUtils, SimpleTypes, OOX/PPTX/XLS/XLSB
# classes, ...). Those static libraries reference each other and are only ever
# linked together inside x2tlib. Add the x2tlib "library" subdirectory (which
# guard-adds every one of those targets) so they exist, then link the static
# archives below inside a --start-group, mirroring x2tlib's own link. In the
# normal top-level build x2t is configured first, so this is a no-op there.
if(NOT TARGET x2tlib)
    add_subdirectory(${CORE_ROOT_DIR}/X2tConverter/build/cmake/library x2tlib)
endif()

# The test reads its fixture via the working-dir-relative path
# "ExampleFiles/motion.odp" (TestEnv passes it straight to convert_single()).
# Stage the committed ExampleFiles next to the binary and run from that dir.
set(_test_odf_run_dir "${CMAKE_CURRENT_BINARY_DIR}/run")

# test.cpp owns main() (InitGoogleTest + AddGlobalTestEnvironment + RUN_ALL_TESTS)
# -> do NOT pass GTEST_MAIN. COfficeFileFormatChecker is compiled from
# OfficeFileFormatChecker2.cpp directly into the target (no format library builds
# it; x2tlib compiles it the same way). FILE_FORMAT_CHECKER_WITH_MACRO is left
# undefined to avoid the heavy PPT dependency chain. unicode_util.cpp/pole.cpp are
# NOT compiled here -- DocxFormatLib provides them, so doing so would duplicate.
add_core_gtest(
    NAME    test_odf
    SOURCES test.cpp
            common.cpp
            ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
    WORKING_DIRECTORY "${_test_odf_run_dir}"
)

# The OOXML/MS format static libraries reference one another, so wrap them in a
# linker group; the remaining converter libraries are shared and linked after.
# This mirrors the static + dynamic lib lists of target_link_libraries(x2tlib).
target_link_libraries(test_odf PRIVATE
    -Wl,--start-group
    OdfFormatLib
    DocFormatLib
    PptFormatLib
    RtfFormatLib
    TxtXmlFormatLib
    BinDocument
    PPTXFormatLib
    DocxFormatLib
    XlsbFormatLib
    XlsFormatLib
    VbaFormatLib
    CompoundFileLib
    CryptoPPLib
    -Wl,--end-group
    graphics
    kernel
    UnicodeConverter
    kernel_network
    Fb2File
    PdfFile
    HtmlFile2
    EpubFile
    XpsFile
    OFDFile
    DjVuFile
    doctrenderer
    DocxRenderer
    IWorkFile
    HWPFile
    StarMathConverter
    ooxmlsignature
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
)

# test_odf.pro adds the core root (for rewritten relative includes such as
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
