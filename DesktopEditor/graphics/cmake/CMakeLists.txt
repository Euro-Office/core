cmake_minimum_required(VERSION 3.10)

project(graphics)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

include(${CORE_ROOT_DIR}/common.cmake)

set(GRAPHICS_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
set(DESKTOP_EDITOR_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
set(OFFICE_UTILS_ROOT_DIR "${CORE_ROOT_DIR}/OfficeUtils")
set(METAFILE_SOURCER_DIR "${DESKTOP_EDITOR_ROOT_DIR}/raster/Metafile")
set(CSS_ROOT_DIR "${CORE_ROOT_DIR}/Common/3dParty/html/css")

set(FONT_ENGINE_SOURCES_DIR "${DESKTOP_EDITOR_ROOT_DIR}/fontengine")

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

set(KATANA_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/html/katana-parser")
set(GUMBO_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/html/gumbo-parser")
get_filename_component(KATANA_INSTALL_DIR_ABS "${KATANA_INSTALL_DIR}" ABSOLUTE)
get_filename_component(GUMBO_INSTALL_DIR_ABS "${GUMBO_INSTALL_DIR}" ABSOLUTE)

set(HARFBUZZ_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/harfbuzz")
get_filename_component(HARFBUZZ_INSTALL_DIR_ABS "${HARFBUZZ_INSTALL_DIR}" ABSOLUTE)

set(BROTLI_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/brotli")
get_filename_component(BROTLI_INSTALL_DIR_ABS "${BROTLI_INSTALL_DIR}" ABSOLUTE)

set(HYPHEN_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/hyphen")
get_filename_component(HYPHEN_INSTALL_DIR_ABS "${HYPHEN_INSTALL_DIR}" ABSOLUTE)

add_library(graphics SHARED
    ${METAFILE_SOURCER_DIR}/MetaFile.h
    ${METAFILE_SOURCER_DIR}/Common/MetaFile.h
    ${METAFILE_SOURCER_DIR}/Common/IOutputDevice.h
    ${METAFILE_SOURCER_DIR}/Common/MetaFileTypes.h
    ${METAFILE_SOURCER_DIR}/Common/MetaFileObjects.h
    ${METAFILE_SOURCER_DIR}/Common/MetaFileRenderer.h
    ${METAFILE_SOURCER_DIR}/Common/MetaFileUtils.h

    ${METAFILE_SOURCER_DIR}/MetaFile.cpp
	${METAFILE_SOURCER_DIR}/Common/MetaFileTypes.cpp
	${METAFILE_SOURCER_DIR}/Common/MetaFileUtils.cpp

    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_arc.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_arrowhead.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_bezier_arc.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_bspline.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_curves.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_image_filters.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_line_aa_basics.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_line_profile_aa.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_trans_affine.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_vcgen_bspline.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_vcgen_contour.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_vcgen_dash.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_vcgen_markers_term.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_vcgen_smooth_poly1.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/src/agg_vcgen_stroke.cpp

    ${GRAPHICS_ROOT_DIR}/AlphaMask.cpp
    ${GRAPHICS_ROOT_DIR}/ArrowHead.cpp
    ${GRAPHICS_ROOT_DIR}/BooleanOperations.cpp
    ${GRAPHICS_ROOT_DIR}/Brush.cpp
    ${GRAPHICS_ROOT_DIR}/Clip.cpp
    ${GRAPHICS_ROOT_DIR}/commands/AnnotField.cpp
    ${GRAPHICS_ROOT_DIR}/commands/DocInfo.cpp
    ${GRAPHICS_ROOT_DIR}/commands/FormField.cpp
    ${GRAPHICS_ROOT_DIR}/Graphics.cpp
    ${GRAPHICS_ROOT_DIR}/GraphicsLayer.cpp
    ${GRAPHICS_ROOT_DIR}/GraphicsPath.cpp
    ${GRAPHICS_ROOT_DIR}/GraphicsRenderer.cpp
    ${GRAPHICS_ROOT_DIR}/Matrix.cpp
    ${GRAPHICS_ROOT_DIR}/MetafileToGraphicsRenderer.cpp
    ${GRAPHICS_ROOT_DIR}/MetafileToRenderer.cpp
    ${GRAPHICS_ROOT_DIR}/MetafileToRendererReader.cpp
    ${GRAPHICS_ROOT_DIR}/pro/officedrawingfile.cpp
    ${GRAPHICS_ROOT_DIR}/pro/pro_Graphics.cpp
    ${GRAPHICS_ROOT_DIR}/pro/pro_Image.cpp

    ${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/include/test_grads/custom_gradients.h

    ${GRAPHICS_ROOT_DIR}/AlphaMask_p.h
    ${GRAPHICS_ROOT_DIR}/AlphaMask.h
    ${GRAPHICS_ROOT_DIR}/ArrowHead.h
    ${GRAPHICS_ROOT_DIR}/boolean_operations_math.h
    ${GRAPHICS_ROOT_DIR}/BooleanOperations.h
    ${GRAPHICS_ROOT_DIR}/Brush.h
    ${GRAPHICS_ROOT_DIR}/Clip.h
    ${GRAPHICS_ROOT_DIR}/Color.h
    ${GRAPHICS_ROOT_DIR}/commands/AnnotField.h
    ${GRAPHICS_ROOT_DIR}/commands/DocInfo.h
    ${GRAPHICS_ROOT_DIR}/commands/FormField.h
    ${GRAPHICS_ROOT_DIR}/config.h
    ${GRAPHICS_ROOT_DIR}/Defines.h
    ${GRAPHICS_ROOT_DIR}/Graphics.h
    ${GRAPHICS_ROOT_DIR}/Graphics.h
    ${GRAPHICS_ROOT_DIR}/GraphicsLayer.h
    ${GRAPHICS_ROOT_DIR}/GraphicsLayerBlend.h
    ${GRAPHICS_ROOT_DIR}/GraphicsPath_private.h
    ${GRAPHICS_ROOT_DIR}/GraphicsPath.h
    ${GRAPHICS_ROOT_DIR}/GraphicsRenderer.h
    ${GRAPHICS_ROOT_DIR}/ImageFilesCache.h
    ${GRAPHICS_ROOT_DIR}/Matrix_private.h
    ${GRAPHICS_ROOT_DIR}/Matrix.h
    ${GRAPHICS_ROOT_DIR}/MetafileToGraphicsRenderer.h
    ${GRAPHICS_ROOT_DIR}/MetafileToRenderer.h
    ${GRAPHICS_ROOT_DIR}/MetafileToRendererCheck.h
    ${GRAPHICS_ROOT_DIR}/MetafileToRendererReader.h
    ${GRAPHICS_ROOT_DIR}/pro/Graphics.h
    ${GRAPHICS_ROOT_DIR}/pro/Image.h
    ${GRAPHICS_ROOT_DIR}/pro/officedrawingfile.h
    ${GRAPHICS_ROOT_DIR}/shading_info.h
    ${GRAPHICS_ROOT_DIR}/structures.h

    # raster.pri
    ${DESKTOP_EDITOR_ROOT_DIR}/graphics/Image.cpp
	${DESKTOP_EDITOR_ROOT_DIR}/raster/BgraFrame.cpp
	${DESKTOP_EDITOR_ROOT_DIR}/raster/Pnm/PnmFile.cpp
	${DESKTOP_EDITOR_ROOT_DIR}/raster/Ras/RasFile.cpp
	${DESKTOP_EDITOR_ROOT_DIR}/raster/WebP/WebPFile.cpp
	${DESKTOP_EDITOR_ROOT_DIR}/raster/heif/heif.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/raster/ImageFileFormatChecker.cpp

    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/tif_xfile.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximabmp.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximadsp.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximaenc.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximaexif.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximage.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximagif.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximahist.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximaico.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximainfo.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximaint.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximajpg.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximalpha.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximalyr.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximapal.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximapcx.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximapng.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximapsd.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximasel.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximatga.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximath.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximatif.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximatran.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximawbmp.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximawmf.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/ximawnd.cpp
    ${DESKTOP_EDITOR_ROOT_DIR}/cximage/CxImage/xmemfile.cpp

    # !disable_cximage_all {
        ${DESKTOP_EDITOR_ROOT_DIR}/raster/JBig2/source/JBig2File.cpp
        ${JBIG2ENC_SOURCES}
        ${DESKTOP_EDITOR_ROOT_DIR}/raster/Jp2/J2kFile.cpp
        ${DESKTOP_EDITOR_ROOT_DIR}/raster/Jp2/Reader.cpp
        ${DESKTOP_EDITOR_ROOT_DIR}/raster/PICT/PICFile.cpp
        ${GRAPHICS_ROOT_DIR}/pro/libpsd_pri.c
        ${GRAPHICS_ROOT_DIR}/pro/libpsd_pri2.c
        ${GRAPHICS_ROOT_DIR}/pro/libpsd_pri3.c
    #}

    ${DESKTOP_EDITOR_ROOT_DIR}/graphics/Image.h
	${DESKTOP_EDITOR_ROOT_DIR}/raster/BgraFrame.h
	${DESKTOP_EDITOR_ROOT_DIR}/raster/ImageFileFormatChecker.h

    # metafile.pri

    # !metafile_disable_wmf_emf {
        ${METAFILE_SOURCER_DIR}/Common/CPathConverter.cpp
        ${METAFILE_SOURCER_DIR}/Common/CPath.cpp
        ${METAFILE_SOURCER_DIR}/Common/CClip.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretator.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorArray.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorRender.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorSvg.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfParser.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfParserBase.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfPlusParser.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfObjects.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfPlusObjects.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfPlayer.cpp
        ${METAFILE_SOURCER_DIR}/Emf/EmfFile.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfObjects.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfPlayer.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CInterpretatorSvgBase.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretatorSvg.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretatorRender.cpp
        # ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretator.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/cwmfinterpretator.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfParser/CWmfParser.cpp
        ${METAFILE_SOURCER_DIR}/Wmf/WmfParser/CWmfParserBase.cpp

        ${METAFILE_SOURCER_DIR}/Common/CPathConverter.h
        ${METAFILE_SOURCER_DIR}/Common/CPath.h
        ${METAFILE_SOURCER_DIR}/Common/CClip.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretator.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorArray.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorBase.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorRender.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorSvg.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfParser.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfParserBase.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfPlusParser.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfTypes.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfObjects.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfPlusObjects.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfPlayer.h
        ${METAFILE_SOURCER_DIR}/Emf/EmfFile.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfObjects.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfPlayer.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfTypes.h
        # ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretator.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/cwmfinterpretator.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretatorBase.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CInterpretatorSvgBase.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretatorSvg.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfInterpretator/CWmfInterpretatorRender.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfParser/CWmfParser.h
        ${METAFILE_SOURCER_DIR}/Wmf/WmfParser/CWmfParserBase.h

        # !metafile_disable_wmf_emf_xml {
            ${METAFILE_SOURCER_DIR}/CXmlOutput.cpp
            ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorXml.cpp
            ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfxParser.cpp

            ${METAFILE_SOURCER_DIR}/CXmlOutput.h
            ${METAFILE_SOURCER_DIR}/Emf/EmfInterpretator/CEmfInterpretatorXml.h
            ${METAFILE_SOURCER_DIR}/Emf/EmfParser/CEmfxParser.h
        # }
    # }

    # !metafile_disable_svg {
        ${METAFILE_SOURCER_DIR}/svg/CSvgFile.cpp
        ${METAFILE_SOURCER_DIR}/svg/CSvgParser.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgReader.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CContainer.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CGradient.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CClipPath.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CMask.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CMarker.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CPattern.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CSymbol.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CSwitch.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CImage.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CLine.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CRect.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CCircle.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CEllipse.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CPath.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CText.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CUse.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CPolyline.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CFont.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CObjectBase.cpp
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CStyle.cpp

        ${METAFILE_SOURCER_DIR}/svg/SvgTypes.h
        ${METAFILE_SOURCER_DIR}/svg/CSvgFile.h
        ${METAFILE_SOURCER_DIR}/svg/CSvgParser.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CContainer.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CGradient.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CClipPath.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CMask.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CPattern.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CSymbol.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CSwitch.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CMarker.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CImage.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CLine.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CRect.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CCircle.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CEllipse.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CPath.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CText.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CUse.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CPolyline.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CFont.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CStyle.h
        ${METAFILE_SOURCER_DIR}/svg/SvgObjects/CObjectBase.h
        ${METAFILE_SOURCER_DIR}/svg/SvgReader.h
        ${METAFILE_SOURCER_DIR}/svg/SvgUtils.h
    # }

    # !metafile_disable_svm {
        ${METAFILE_SOURCER_DIR}/StarView/SvmFile.cpp
        ${METAFILE_SOURCER_DIR}/StarView/SvmObjects.cpp
        ${METAFILE_SOURCER_DIR}/StarView/SvmPlayer.cpp

        ${METAFILE_SOURCER_DIR}/StarView/SvmFile.h
        ${METAFILE_SOURCER_DIR}/StarView/SvmObjects.h
        ${METAFILE_SOURCER_DIR}/StarView/SvmPlayer.h
    # }

    # CssCalculator.pri
    # css_calculator_without_xhtml {
        ${CSS_ROOT_DIR}/src/CCssCalculator_Private.cpp
		${CSS_ROOT_DIR}/src/CElement.cpp
		${CSS_ROOT_DIR}/src/CNode.cpp
		${CSS_ROOT_DIR}/src/ConstValues.cpp
		${CSS_ROOT_DIR}/src/StaticFunctions.cpp
		${CSS_ROOT_DIR}/src/StyleProperties.cpp
		${CSS_ROOT_DIR}/src/CUnitMeasureConverter.cpp    

        ${CSS_ROOT_DIR}/src/CCssCalculator_Private.h
		${CSS_ROOT_DIR}/src/CElement.h
		${CSS_ROOT_DIR}/src/CNode.h
		${CSS_ROOT_DIR}/src/ConstValues.h
		${CSS_ROOT_DIR}/src/StaticFunctions.h
		${CSS_ROOT_DIR}/src/StyleProperties.h
		${CSS_ROOT_DIR}/src/CUnitMeasureConverter.h
    # }

    # katana.pri
    ${KATANA_INSTALL_DIR_ABS}/src/foundation.c
    ${KATANA_INSTALL_DIR_ABS}/src/katana.lex.c
    ${KATANA_INSTALL_DIR_ABS}/src/katana.tab.c
    ${KATANA_INSTALL_DIR_ABS}/src/parser.c
    ${KATANA_INSTALL_DIR_ABS}/src/selector.c
    ${KATANA_INSTALL_DIR_ABS}/src/tokenizer.c

    ${KATANA_INSTALL_DIR_ABS}/src/foundation.h
    ${KATANA_INSTALL_DIR_ABS}/src/katana.h
    ${KATANA_INSTALL_DIR_ABS}/src/katana.lex.h
    ${KATANA_INSTALL_DIR_ABS}/src/katana.tab.h
    ${KATANA_INSTALL_DIR_ABS}/src/parser.h
    ${KATANA_INSTALL_DIR_ABS}/src/selector.h
    ${KATANA_INSTALL_DIR_ABS}/src/tokenizer.h

    ${FREETYPE_SOURCES_DIR}/src/autofit/autofit.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftbase.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftbbox.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftbdf.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftbitmap.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftcid.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftdebug.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftfstype.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftgasp.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftglyph.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftgxval.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftinit.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftmm.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftotval.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftpatent.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftpfr.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftstroke.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftsynth.c
    ${FREETYPE_SOURCES_DIR}/src/base/fttype1.c
    ${FREETYPE_SOURCES_DIR}/src/base/ftwinfnt.c
    ${FREETYPE_SOURCES_DIR}/src/bdf/bdf.c
    ${FREETYPE_SOURCES_DIR}/src/bzip2/ftbzip2.c
    ${FREETYPE_SOURCES_DIR}/src/cache/ftcache.c
    ${FREETYPE_SOURCES_DIR}/src/cff/cff.c
    ${FREETYPE_SOURCES_DIR}/src/cid/type1cid.c
    ${FREETYPE_SOURCES_DIR}/src/gzip/ftgzip.c
    ${FREETYPE_SOURCES_DIR}/src/hvf/hvf.c
    ${FREETYPE_SOURCES_DIR}/src/lzw/ftlzw.c
    ${FREETYPE_SOURCES_DIR}/src/pcf/pcf.c
    ${FREETYPE_SOURCES_DIR}/src/pfr/pfr.c
    ${FREETYPE_SOURCES_DIR}/src/psaux/psaux.c
    ${FREETYPE_SOURCES_DIR}/src/pshinter/pshinter.c
    ${FREETYPE_SOURCES_DIR}/src/psnames/psnames.c
    ${FREETYPE_SOURCES_DIR}/src/raster/raster.c
    ${FREETYPE_SOURCES_DIR}/src/sdf/sdf.c
    ${FREETYPE_SOURCES_DIR}/src/sfnt/sfnt.c
    ${FREETYPE_SOURCES_DIR}/src/smooth/smooth.c
    ${FREETYPE_SOURCES_DIR}/src/svg/svg.c
    ${FREETYPE_SOURCES_DIR}/src/truetype/truetype.c
    ${FREETYPE_SOURCES_DIR}/src/type1/type1.c
    ${FREETYPE_SOURCES_DIR}/src/type42/type42.c
    ${FREETYPE_SOURCES_DIR}/src/winfonts/winfnt.c
 
    # fontengine.pri
	${FONT_ENGINE_SOURCES_DIR}/fontconverter/FontConverter.cpp
	${FONT_ENGINE_SOURCES_DIR}/fontconverter/FontFileEncodings.cpp
	${FONT_ENGINE_SOURCES_DIR}/fontconverter/FontFileTrueType.cpp
	${FONT_ENGINE_SOURCES_DIR}/fontconverter/FontFileType1.cpp
	${FONT_ENGINE_SOURCES_DIR}/fontconverter/FontFileType1C.cpp
	${FONT_ENGINE_SOURCES_DIR}/fontconverter/Hash.cpp
    ${FONT_ENGINE_SOURCES_DIR}/ApplicationFonts.cpp
    ${FONT_ENGINE_SOURCES_DIR}/ApplicationFontsWorker.cpp
    ${FONT_ENGINE_SOURCES_DIR}/fontconverter/StringExt.cpp
    ${FONT_ENGINE_SOURCES_DIR}/FontFile.cpp
    ${FONT_ENGINE_SOURCES_DIR}/FontManager.cpp
    ${FONT_ENGINE_SOURCES_DIR}/FontPath.cpp
    ${FONT_ENGINE_SOURCES_DIR}/GlyphString.cpp
    ${FONT_ENGINE_SOURCES_DIR}/TextHyphen.cpp
    ${FONT_ENGINE_SOURCES_DIR}/TextShaper.cpp
    ${GRAPHICS_ROOT_DIR}/pro/pro_Fonts.cpp

    ${FONT_ENGINE_SOURCES_DIR}/ApplicationFonts.h
    ${FONT_ENGINE_SOURCES_DIR}/ApplicationFontsWorker.h
    ${FONT_ENGINE_SOURCES_DIR}/FontConverter.h
    ${FONT_ENGINE_SOURCES_DIR}/FontFile.h
    ${FONT_ENGINE_SOURCES_DIR}/FontManager.h
    ${FONT_ENGINE_SOURCES_DIR}/FontPath.h
    ${FONT_ENGINE_SOURCES_DIR}/GlyphString.h
    ${FONT_ENGINE_SOURCES_DIR}/languages.h
    ${FONT_ENGINE_SOURCES_DIR}/TextHyphen.h
    ${FONT_ENGINE_SOURCES_DIR}/TextShaper.h
    ${GRAPHICS_ROOT_DIR}/pro/Fonts.h

    # deprecated!!! remove after 6.5 ------------------
    # I have doubts, current version is 9.1.0 and it's still here...
    ${FONT_ENGINE_SOURCES_DIR}/FontsAssistant.h
    ${FONT_ENGINE_SOURCES_DIR}/FontsAssistant.cpp

    # harfbuzz.pri
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-map.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-blob.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer-serialize.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer-verify.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cairo-utils.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cairo.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-common.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-coretext.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-directwrite.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-draw.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-face-builder.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-face.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-fallback-shape.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-font.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ft.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-gdi.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-glib.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-gobject-structs.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-graphite2.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-icu.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-map.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-number.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cff1-table.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cff2-table.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-color.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-face.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-font.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-map.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-math.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-meta.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-metrics.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-name.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape-fallback.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape-normalize.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-default.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-hangul.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-hebrew.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-indic-table.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-indic.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-khmer.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-myanmar.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-syllabic.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-thai.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-use.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-vowel-constraints.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-tag.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-outline.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-paint-extents.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-paint.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-set.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shape-plan.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shape.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shaper.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-static.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-style.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-cff-common.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-cff1.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-cff2.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-input.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-instancer-solver.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-plan.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-repacker.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ucd.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-unicode.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-uniscribe.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-shape.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-blob.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cairo.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-common.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-coretext.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-deprecated.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-directwrite.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-draw.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-face.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-font.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ft.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-gdi.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-glib.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-gobject-structs.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-gobject.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-graphite2.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-icu.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-map.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-color.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-deprecated.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-font.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-math.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-meta.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-metrics.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-name.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-paint.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-set.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shape-plan.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shape.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-style.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-repacker.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-unicode.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-uniscribe.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-version.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb.h
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-ankr-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-bsln-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-feat-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-just-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-kerx-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-morx-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-opbd-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout-trak-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-layout.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-ltag-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-aat-map.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-algs.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-array.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-atomic.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-bimap.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-bit-page.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-bit-set-invertible.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-bit-set.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-blob.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer-deserialize-json.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer-deserialize-text-glyphs.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer-deserialize-text-unicode.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-buffer.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cache.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cairo-utils.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cff-interp-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cff-interp-cs-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cff-interp-dict-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cff1-interp-cs.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cff2-interp-cs.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-config.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-cplusplus.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-debug.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-dispatch.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-draw.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-face.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-font.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ft-colr.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-iter.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-kern.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-limits.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-machinery.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-map.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-meta.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ms-feature-ranges.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-multimap.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-mutex.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-null.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-number-parser.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-number.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-object.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-open-file.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-open-type.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cff-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cff1-std-str.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cff1-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cff2-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-cmap-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-face-table-list.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-face.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-gasp-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-glyf-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-hdmx-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-head-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-hhea-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-hmtx-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-kern-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-base-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-gdef-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-gpos-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-gsub-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-gsubgpos.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout-jstf-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-layout.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-map.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-math-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-maxp-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-meta-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-metrics.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-name-language-static.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-name-language.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-name-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-os2-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-os2-unicode-ranges.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-post-macroman.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-post-table-v2subset.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-post-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape-fallback.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape-normalize.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shape.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic-fallback.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic-joining-list.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic-pua.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic-win1256.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-arabic.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-indic-machine.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-indic.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-khmer-machine.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-myanmar-machine.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-syllabic.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-use-machine.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-use-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper-vowel-constraints.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-shaper.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-stat-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-tag-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-avar-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-cvar-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-fvar-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-gvar-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-hvar-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-var-mvar-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ot-vorg-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-outline.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-paint-extents.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-paint.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-pool.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-priority-queue.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-repacker.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-sanitize.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-serialize.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-set-digest.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-set.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shape-plan.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shaper-impl.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shaper-list.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-shaper.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-string-array.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-accelerator.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-cff-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-input.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-instancer-solver.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-plan-member-list.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset-plan.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-subset.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-ucd-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-unicode-emoji-table.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-unicode.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-utf.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-vector.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-blob.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-buffer.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-common.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-face.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-font.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-list.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api-shape.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb-wasm-api.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/hb.hh

    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/gsubgpos-context.cc
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/gsubgpos-context.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/classdef-graph.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/coverage-graph.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/graph.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/gsubgpos-graph.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/markbasepos-graph.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/pairpos-graph.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/serialize.hh
    ${HARFBUZZ_INSTALL_DIR_ABS}/src/graph/split-helpers.hh

    # brotli.pri
    ${BROTLI_INSTALL_DIR_ABS}/c/common/constants.c
    ${BROTLI_INSTALL_DIR_ABS}/c/common/context.c
    ${BROTLI_INSTALL_DIR_ABS}/c/common/dictionary.c
    ${BROTLI_INSTALL_DIR_ABS}/c/common/platform.c
    ${BROTLI_INSTALL_DIR_ABS}/c/common/shared_dictionary.c
    ${BROTLI_INSTALL_DIR_ABS}/c/common/transform.c
    ${BROTLI_INSTALL_DIR_ABS}/c/dec/bit_reader.c
    ${BROTLI_INSTALL_DIR_ABS}/c/dec/decode.c
    ${BROTLI_INSTALL_DIR_ABS}/c/dec/huffman.c
    ${BROTLI_INSTALL_DIR_ABS}/c/dec/prefix.c
    ${BROTLI_INSTALL_DIR_ABS}/c/dec/state.c
    ${BROTLI_INSTALL_DIR_ABS}/c/dec/static_init.c
)

if( APPLE )
    target_sources( graphics PRIVATE
        ${FONT_ENGINE_SOURCES_DIR}/ApplicationFonts_mac.mm
    )
    set_source_files_properties(
        ${FONT_ENGINE_SOURCES_DIR}/ApplicationFonts_mac.mm
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )
endif()

set_default_options(graphics)

if (WIN32)
    target_sources(graphics PRIVATE ${FREETYPE_SOURCES_DIR}/src/base/ftsystem.c) # freetype.pri
else()
    target_sources(graphics PRIVATE ${FREETYPE_SOURCES_DIR}/builds/unix/ftsystem.c) # freetype.pri
    target_compile_definitions(graphics PRIVATE
        HAVE_UNISTD_H
        HAVE_FCNTL_H
    )
endif()

target_compile_definitions(graphics PRIVATE
    GRAPHICS_USE_DYNAMIC_LIBRARY_BUILDING
    DISABLE_IMAGE_EXCEPTIONS

    # metafile.pri
    METAFILE_SUPPORT_WMF_EMF # !metafile_disable_wmf_emf
    METAFILE_SUPPORT_WMF_EMF_XML # !metafile_disable_wmf_emf_xml
    METAFILE_SUPPORT_SVG # !metafile_disable_svg
    METAFILE_SUPPORT_SVM # !metafile_disable_svm

    # freetype.pri
    FT_CONFIG_OPTION_SYSTEM_ZLIB
    FT2_BUILD_LIBRARY

    # textshaper.pri
    SUPPORT_HARFBUZZ_SHAPER

    # harfbuzz.pri
    HAVE_FREETYPE
    HB_NO_VISIBILITY

    _lseek=lseek

    # brotli.pri
    FT_CONFIG_OPTION_USE_BROTLI

    # HEIC and AVIF through libheif (CXIMAGE_SUPPORT_HEIF)
    SUPPORT_LIB_HEIF_SOURCES
)

if(LINUX)
    target_compile_definitions(graphics PRIVATE
        _LINUX
    )
endif()

if( MSVC )
    target_compile_definitions(graphics PRIVATE
        DLL_EXPORT # freetype dll export
        WIN32_LEAN_AND_MEAN
    )
endif()

target_include_directories(graphics PRIVATE
    ${EO_CORE_3RD_PARTY_INSTALL_DIR}
    ${KATANA_INSTALL_DIR_ABS}/src
	${DESKTOP_EDITOR_ROOT_DIR}/agg-2.4/include
    ${ZLIBNG_INCLUDEDIR_ABS}

    # freetype.pri
    ${FREETYPE_SOURCES_DIR}/include
    ${FREETYPE_SOURCES_DIR}/include/freetype

    # harfbuzz.pri
    ${HARFBUZZ_INSTALL_DIR_ABS}
    ${HARFBUZZ_INSTALL_DIR_ABS}/src

    # brotli.pri
    ${BROTLI_INSTALL_DIR_ABS}/c/include

    ${JBIG2ENC_SOURCES_DIR}
)

# jbig2enc reports its version through jbig2_version()
set_source_files_properties(${JBIG2ENC_SOURCES} PROPERTIES COMPILE_DEFINITIONS "VERSION=\"0.32\"")

if( MSVC )
    target_include_directories(graphics PRIVATE
        ${KATANA_INSTALL_DIR_ABS}/visualc/include
    )
endif()

target_precompile_headers(graphics PRIVATE
    ${CORE_ROOT_DIR}/msvc_fix.h
)

if(LINUX)
    target_compile_options(graphics PRIVATE
        -Wno-narrowing
        -UUNICODE
        -U_UNICODE
        -fpermissive
        -UHAVE_IO_H
    )

    # Re-enable exporting symbols
    target_compile_options(graphics PRIVATE
        -fvisibility=default
        -fvisibility-inlines-hidden
    )

    # Don't leak the bundled third-party C libraries (libpng, zlib, ...),
    # they would interpose the system ones used by GTK/gdk-pixbuf
    target_link_options(graphics PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_LIST_DIR}/graphics.map"
    )
    set_property(TARGET graphics APPEND PROPERTY
        LINK_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/graphics.map"
    )
endif()

# dynamic libs


target_link_libraries(graphics PUBLIC
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time    
    kernel
    UnicodeConverter
)

target_link_libraries(graphics PUBLIC z)
target_link_libraries(graphics PRIVATE jpeg png tiff webp webpdemux heif leptonica)

if(WIN32)
    target_link_libraries(graphics PUBLIC
        Advapi32
        Shell32
        User32
        # htonl/htons in jbig2enc; libdcr used to pull it in via #pragma comment
        Ws2_32
    )
endif()

if( APPLE )
    find_library( FOUNDATION_LIBRARY Foundation REQUIRED )
    find_library( CORETEXT_LIBRARY CoreText REQUIRED )
    target_link_libraries( graphics PRIVATE ${FOUNDATION_LIBRARY} ${CORETEXT_LIBRARY} )
endif()

copy_artifacts_to_folder("graphics" "${EO_CORE_OUTPUT_DIR}")

declare_victory( graphics )
