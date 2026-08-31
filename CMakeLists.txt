cmake_minimum_required(VERSION 3.10)

project(core)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

include(${CORE_ROOT_DIR}/common.cmake)

option(EO_BUILD_TESTS "Build C++ unit tests and register them with CTest" OFF)

if(EMSCRIPTEN)

    add_subdirectory( "${CORE_ROOT_DIR}/OfficeUtils/js" zlib )
    #add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src/wasm/oform_plugin/module" oform_plugin )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src/wasm/hash/" hash )
    add_subdirectory( "${CORE_ROOT_DIR}/Common/3dParty/hunspell" spell )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/fontengine/js" fonts )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/graphics/pro/js/" drawingfile )

else()

    add_subdirectory( "${CORE_ROOT_DIR}/X2tConverter/build/cmake" x2t )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/AllFontsGen" allfontsgen )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/allthemesgen" allthemesgen )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/pluginsmanager" pluginsmanager )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/doctrenderer/app_builder" docbuilder )
    add_subdirectory( "${CORE_ROOT_DIR}/PdfFile/Resources/CMapMemory" cmapbin )
    add_subdirectory( "${CORE_ROOT_DIR}/Test/Applications/x2tTester" x2ttester )

    if(EO_BUILD_TESTS)
        enable_testing()
        find_package(GTest CONFIG REQUIRED)

        add_subdirectory( "${CORE_ROOT_DIR}/Common/cfcpp/test" cfcpp_test )
        add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/graphics/tests/BooleanOperations_Unit-tests" booleanoperations_test )
        add_subdirectory( "${CORE_ROOT_DIR}/OdfFile/Reader/Converter/StarMath2OOXML/TestSMConverter" starmath_smconverter_test )
        add_subdirectory( "${CORE_ROOT_DIR}/OdfFile/Reader/Converter/StarMath2OOXML/TestEQNtoOOXML" starmath_eqntoooxml_test )
        add_subdirectory( "${CORE_ROOT_DIR}/OOXML/test" ooxml_test )
        add_subdirectory( "${CORE_ROOT_DIR}/OfficeUtils/tests" officeutils_test )
        add_subdirectory( "${CORE_ROOT_DIR}/OdfFile/Reader/Converter/SMCustomShape2OOXML/TestSMCustomShape" starmath_smcustomshape_test )
        add_subdirectory( "${CORE_ROOT_DIR}/OdfFile/Test/test_odf" test_odf )

        # doctrenderer (V8) gtest suites. The V8 monolith is built in CI by
        # common.cmake (build_3rdparty.py) and is self-contained at run time:
        # the x64-linux build sets v8_use_external_startup_data=false (snapshot
        # embedded in the monolith) and v8_enable_i18n_support=false (no external
        # icudtl.dat), so CJSContext initializes with no external runtime data
        # files. These suites build all JS inline via runScript() — no fixtures.
        add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/doctrenderer/test/json" doctrenderer_json_test )
        add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/doctrenderer/test/js_internal" doctrenderer_jsinternal_test )
        add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/doctrenderer/test/embed/internal/hash" doctrenderer_hash_test )
    endif()

endif()
