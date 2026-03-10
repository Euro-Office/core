cmake_minimum_required(VERSION 3.10)

project(core)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

include(${CORE_ROOT_DIR}/common.cmake)

# platform-specific compile flags
if(EMSCRIPTEN)

    add_subdirectory( "${CORE_ROOT_DIR}/OfficeUtils/js" zlib )
    #add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src/wasm/oform_plugin/module" oform_plugin )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src/wasm/hash/" hash )
    add_subdirectory( "${CORE_ROOT_DIR}/Common/3dParty/hunspell" spell )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/fontengine/js" fonts )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/graphics/pro/js/" drawingfile )
else()

    add_compile_options(-march=native)

    add_subdirectory( "${CORE_ROOT_DIR}/X2tConverter/build/cmake" x2t )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/AllFontsGen" allfontsgen )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/allthemesgen" allthemesgen )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/pluginsmanager" pluginsmanager )
    add_subdirectory( "${CORE_ROOT_DIR}/DesktopEditor/doctrenderer/app_builder" docbuilder )
    add_subdirectory( "${CORE_ROOT_DIR}/PdfFile/Resources/CMapMemory" cmapbin )
    add_subdirectory( "${CORE_ROOT_DIR}/Test/Applications/x2tTester" x2ttester )

    #[[
    set( ALL_ARTIFACTS
        UnicodeConverter
        kernel
        kernel_network
        Fb2File
        PdfFile
        HtmlFile2
        EpubFile
        XpsFile
        OFDFile
        DjVuFile
        DocxRenderer
        doctrenderer
        IWorkFile
        HWPFile
        graphics
        x2tlib
        allfontsgen
        allthemesgen
        pluginsmanager
        docbuilder
        x2t
    )

    add_custom_target(copy_artifacts ALL
        COMMENT "Copying all artifacts into ${EO_CORE_OUTPUT_DIR}"
    )

    add_dependencies(copy_artifacts ${ALL_ARTIFACTS})

    foreach(artifact ${ALL_ARTIFACTS})
        add_custom_command(TARGET copy_artifacts POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${EO_CORE_OUTPUT_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${artifact}> "${EO_CORE_OUTPUT_DIR}/"
            COMMENT "Copying ${artifact} to ${EO_CORE_OUTPUT_DIR}"
        )
    endforeach()

    add_custom_command(TARGET copy_artifacts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${EO_CORE_OUTPUT_DIR}"
        COMMAND /bin/sh -c "cp -P \"${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu/lib\"/*.so* \"${EO_CORE_OUTPUT_DIR}/\""
        COMMENT "Copying ICU libs to ${EO_CORE_OUTPUT_DIR}"
    )
    ]]

endif()
