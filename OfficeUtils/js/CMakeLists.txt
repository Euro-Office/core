cmake_minimum_required(VERSION 3.10)
project(zlib)

# -------------------------------------------------
# Ensure Emscripten toolchain
# -------------------------------------------------
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This project must be built with Emscripten (use emcmake).")
endif()


# -------------------------------------------------
# Source files (compile_files_array)
# -------------------------------------------------
set(CORE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include(${CORE_ROOT}/common.cmake)

set(ZLIBNG_WASM_DIR    "${EO_CORE_3RD_PARTY_INSTALL_DIR}/zlib-ng-wasm")
set(ZLIBNG_WASM_INCDIR "${ZLIBNG_WASM_DIR}/include")
set(ZLIBNG_WASM_LIBDIR "${ZLIBNG_WASM_DIR}/lib")

set(SRC
    # ---- project glue ----
    ${CORE_ROOT}/OfficeUtils/src/ZipBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wasm/src/base.cpp
    ${CORE_ROOT}/OfficeUtils/src/ioapibuf.c

    # ---- raster ----
    ${CORE_ROOT}/DesktopEditor/graphics/Image.cpp
    ${CORE_ROOT}/DesktopEditor/graphics/Matrix.cpp
    ${CORE_ROOT}/DesktopEditor/graphics/GraphicsPath.cpp
    ${CORE_ROOT}/DesktopEditor/raster/BgraFrame.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Pnm/PnmFile.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Ras/RasFile.cpp
    ${CORE_ROOT}/DesktopEditor/raster/WebP/WebPFile.cpp
    ${CORE_ROOT}/DesktopEditor/raster/ImageFileFormatChecker.cpp

    ${CORE_ROOT}/DesktopEditor/graphics/pro/js/qt/raster/pro_Fonts_empty.cpp
    ${CORE_ROOT}/DesktopEditor/graphics/pro/js/qt/raster/pro_Graphics_empty.cpp
    ${CORE_ROOT}/DesktopEditor/graphics/pro/js/qt/raster/raster.cpp

    # ---- ci ----
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximaenc.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximaexif.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximage.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximainfo.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximajpg.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximalpha.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximapal.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximasel.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/xmemfile.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximapng.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximabmp.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximatran.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximatif.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/tif_xfile.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximagif.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximaico.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximatga.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximapcx.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximawbmp.cpp
    ${CORE_ROOT}/DesktopEditor/cximage/CxImage/ximapsd.cpp

    # ---- Metafile ----
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/MetaFile.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Common/CClip.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Common/MetaFileTypes.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Common/MetaFileUtils.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Common/CPathConverter.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Common/CPath.cpp
    ${CORE_ROOT}/DesktopEditor/raster/PICT/PICFile.cpp

    # ---- Emf ----
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfInterpretator/CEmfInterpretator.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfInterpretator/CEmfInterpretatorArray.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfInterpretator/CEmfInterpretatorRender.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfInterpretator/CEmfInterpretatorSvg.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfParser/CEmfParser.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfParser/CEmfParserBase.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfParser/CEmfPlusParser.cpp
    #../../DesktopEditor/raster/Metafile/Emf/EmfClip.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfObjects.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfPlusObjects.cpp
    #../../DesktopEditor/raster/Metafile/Emf/EmfPath.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfPlayer.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Emf/EmfFile.cpp

    # ---- wmf ----
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfInterpretator/CWmfInterpretatorSvg.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfInterpretator/CInterpretatorSvgBase.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfInterpretator/CWmfInterpretatorRender.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfInterpretator/cwmfinterpretator.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfParser/CWmfParser.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfParser/CWmfParserBase.cpp
    #../../DesktopEditor/raster/Metafile/Wmf/WmfClip.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfObjects.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Metafile/Wmf/WmfPlayer.cpp

    # ---- common ----
    ${CORE_ROOT}/DesktopEditor/common/File.cpp
    ${CORE_ROOT}/DesktopEditor/common/Base64.cpp
    ${CORE_ROOT}/DesktopEditor/common/StringExt.cpp
    ${CORE_ROOT}/DesktopEditor/common/StringBuilder.cpp
    ${CORE_ROOT}/DesktopEditor/common/../xml/src/xmlwriter.cpp
    #../../DesktopEditor/common/../xml/src/xmllight.cpp

    # ---- agg ----
    ${CORE_ROOT}/DesktopEditor/agg-2.4/src/agg_trans_affine.cpp
    ${CORE_ROOT}/DesktopEditor/agg-2.4/src/agg_bezier_arc.cpp


    ### These are part of full build    
    # ---- jp2 ----
    ${CORE_ROOT}/DesktopEditor/raster/Jp2/J2kFile.cpp
    ${CORE_ROOT}/DesktopEditor/raster/Jp2/Reader.cpp

    # ---- libpsd ----
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/psd.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/file_header.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/color_mode.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/image_resource.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/blend.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/layer_mask.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/image_data.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/stream.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/psd_system.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/color.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/pattern_fill.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/color_balance.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/channel_image.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/gradient_fill.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/invert.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/posterize.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/brightness_contrast.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/solid_color.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/threshold.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/effects.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/selective_color.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/channel_mixer.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/photo_filter.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/type_tool.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/gradient_map.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/hue_saturation.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/levels.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/curves.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/pattern.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/psd_zip.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/descriptor.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/drop_shadow.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/inner_shadow.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/color_overlay.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/outer_glow.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/inner_glow.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/bevel_emboss.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/satin.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/gradient_overlay.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/stroke.c
    ${CORE_ROOT}/DesktopEditor/cximage/libpsd/pattern_overlay.c

    # ---- jbig ----

)


# -------------------------------------------------
# Executable (WASM module)
# -------------------------------------------------
add_executable(${PROJECT_NAME} ${SRC})

# -------------------------------------------------
# Output folder
# -------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${EO_CORE_OUTPUT_DIR}/${PROJECT_NAME}/engine
)

# -------------------------------------------------
# Include directories
# -------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${ZLIBNG_WASM_INCDIR}
    ${ZLIBNG_WASM_INCDIR}/minizip
    ${CORE_ROOT}/OfficeUtils/src
    ${CORE_ROOT}/DesktopEditor/agg-2.4/include
)

# -------------------------------------------------
# Defines
# -------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    __linux__ _LINUX UNIX _QT
    METAFILE_SUPPORT_WMF_EMF
    HAVE_UNISTD_H HAVE_FCNTL_H _ARM_ALIGN_
    CXIMAGE_DISABLE_SUPPORT_MNG
    CXIMAGE_DISABLE_SUPPORT_ADDITIONS
    IMAGE_CHECKER_DISABLE_XML
    _tcsnicmp=strncmp
    _lseek=lseek
    _getcwd=getcwd
    NO_CONSOLE_IO
)

# -------------------------------------------------
# Static libs
# -------------------------------------------------
target_link_directories(${PROJECT_NAME} PRIVATE
    ${ZLIBNG_WASM_LIBDIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    minizip
    z
    jpeg
    png
    tiff
    webp
    webpdemux
)

# -------------------------------------------------
# Compiler flags (non -s ones)
# -------------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    -O3
    -Wno-unused-command-line-argument
    -Wno-register
)


# -------------------------------------------------
# Linker flags (Emscripten -s options)
# -------------------------------------------------
target_link_options(${PROJECT_NAME} PRIVATE
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFILESYSTEM=0"
    "-sENVIRONMENT=web,worker"
    "-sEXPORTED_FUNCTIONS=['_malloc','_free','_Zlib_Malloc','_Zlib_Free','_Zlib_Create','_Zlib_Open','_Zlib_Close','_Zlib_AddFile','_Zlib_RemoveFile','_Zlib_GetPaths','_Zlib_GetFile','_Zlib_Save','_Raster_DecodeFile','_Raster_GetDecodedBuffer','_Raster_GetHeight','_Raster_GetWidth','_Raster_GetStride','_Raster_Destroy','_Raster_EncodeImageData','_Raster_Encode','_Raster_GetEncodedBuffer','_Raster_GetEncodedSize','_Raster_DestroyEncodedData','_Image_GetFormat']"
    "-sEXPORTED_RUNTIME_METHODS=['HEAP8']"
    "-sLEGACY_VM_SUPPORT=1"
    "SHELL:--pre-js ${CORE_ROOT}/Common/js/string_utf8.js"
    "SHELL:--pre-js ${CORE_ROOT}/Common/js/desktop_fetch_new.js"
)

# Create final .js for Wasm
inject_script(
    ${PROJECT_NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/wasm/js/zlib.js"
    "${EO_CORE_OUTPUT_DIR}/${PROJECT_NAME}/engine/${PROJECT_NAME}.js"
    MODE POST_BUILD
    REPLACEMENTS
        "{{{ MODULE }}}"  "${EO_CORE_OUTPUT_DIR}/${PROJECT_NAME}/engine/${PROJECT_NAME}.js"
)

add_custom_target(${PROJECT_NAME}_copy_extra_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/wasm/js/index.html"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../index.html"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/wasm/js/code.js"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../code.js"
    COMMENT "Copying extra files to output"
    VERBATIM
)

declare_victory( ${PROJECT_NAME})
