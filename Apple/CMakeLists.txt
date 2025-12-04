cmake_minimum_required(VERSION 3.10)

project(IWorkFile)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

include(${CORE_ROOT_DIR}/common.cmake)

set(OFFICE_UTILS_ROOT_DIR "${CORE_ROOT_DIR}/OfficeUtils")
set(DESKTOP_EDITOR_ROOT "${CORE_ROOT_DIR}/DesktopEditor")
set(COMMON_3RDPARTY_ROOT "${CORE_ROOT_DIR}/Common/3dParty")

set(ONLY_LIBXML 1)
if(NOT TARGET libxml)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/xml/build/cmake libxml)
endif()

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()




message( "Fetch, patch Apple related 3rd-party repos" )
set(APPLE_3RDPARTY_ROOT "${COMMON_3RDPARTY_ROOT}/apple")
get_filename_component(APPLE_3RDPARTY_ROOT_ABS "${APPLE_3RDPARTY_ROOT}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./fetch.sh
    WORKING_DIRECTORY ${APPLE_3RDPARTY_ROOT_ABS}
)
set(REVENGE_LIB_ROOT "${APPLE_3RDPARTY_ROOT}/librevenge")
set(ODF_LIB_ROOT "${APPLE_3RDPARTY_ROOT}/libodfgen")
set(ETONYEK_LIB_ROOT "${APPLE_3RDPARTY_ROOT}/libetonyek")
set(MDDS_LIB_ROOT "${APPLE_3RDPARTY_ROOT}/mdds")
set(GLM_LIB_ROOT "${APPLE_3RDPARTY_ROOT}/glm")





add_library(IWorkFile SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/IWork.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/IWork.h

    # OfficeUtils.pri
    ${OFFICE_UTILS_ROOT_DIR}/src/OfficeUtils.cpp
    ${OFFICE_UTILS_ROOT_DIR}/src/ZipBuffer.cpp
    ${OFFICE_UTILS_ROOT_DIR}/src/ZipUtilsCP.cpp
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib_addon.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/adler32.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/compress.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/ioapi.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/ioapibuf.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/miniunz.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/minizip.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/mztools.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/unzip.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/zip.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/crc32.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/deflate.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/gzclose.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/gzlib.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/gzread.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/gzwrite.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/infback.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/inffast.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/inflate.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/inftrees.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/trees.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/uncompr.c
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/zutil.c

    ${OFFICE_UTILS_ROOT_DIR}/src/OfficeUtils.h
    ${OFFICE_UTILS_ROOT_DIR}/src/OfficeUtilsCommon.h
    ${OFFICE_UTILS_ROOT_DIR}/src/ZipBuffer.h
    ${OFFICE_UTILS_ROOT_DIR}/src/ZipFolder.h
    ${OFFICE_UTILS_ROOT_DIR}/src/ZipUtilsCP.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib_addon.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/crypt.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/ioapi.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/ioapibuf.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/mztools.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/unzip.h
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/zip.h

    # apple.pri
    ${REVENGE_LIB_ROOT}/src/lib/RVNGBinaryData.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGCSVSpreadsheetGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGHTMLTextGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGHTMLTextTableStyle.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGHTMLTextTextStyle.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGMemoryStream.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGOLEStream.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGProperty.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGPropertyList.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGPropertyListVector.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGRawDrawingGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGRawGeneratorBase.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGRawPresentationGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGRawSpreadsheetGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGRawTextGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGSVGDrawingGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGSVGPresentationGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGString.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGStringVector.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGTextDrawingGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGTextPresentationGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGTextSpreadsheetGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGTextTextGenerator.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGZipStream.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGDirectoryStream.cpp
    ${REVENGE_LIB_ROOT}/src/lib/RVNGStreamImplementation.cpp

    ${ODF_LIB_ROOT}/src/DocumentElement.cxx
    ${ODF_LIB_ROOT}/src/FilterInternal.cxx
    ${ODF_LIB_ROOT}/src/FontStyle.cxx
    ${ODF_LIB_ROOT}/src/GraphicFunctions.cxx
    ${ODF_LIB_ROOT}/src/GraphicStyle.cxx
    ${ODF_LIB_ROOT}/src/InternalHandler.cxx
    ${ODF_LIB_ROOT}/src/ListStyle.cxx
    ${ODF_LIB_ROOT}/src/OdcGenerator.cxx
    ${ODF_LIB_ROOT}/src/OdfGenerator.cxx
    ${ODF_LIB_ROOT}/src/OdgGenerator.cxx
    ${ODF_LIB_ROOT}/src/OdpGenerator.cxx
    ${ODF_LIB_ROOT}/src/OdsGenerator.cxx
    ${ODF_LIB_ROOT}/src/OdtGenerator.cxx
    ${ODF_LIB_ROOT}/src/PageSpan.cxx
    ${ODF_LIB_ROOT}/src/SectionStyle.cxx
    ${ODF_LIB_ROOT}/src/SheetStyle.cxx
    ${ODF_LIB_ROOT}/src/TableStyle.cxx
    ${ODF_LIB_ROOT}/src/TextRunStyle.cxx
    ${ODF_LIB_ROOT}/test/StringDocumentHandler.cxx

    ${ETONYEK_LIB_ROOT}/src/lib/EtonyekDocument.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWAField.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWAMessage.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWAObjectIndex.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWAReader.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWASnappyStream.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWAText.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKChainedTokenizer.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKChart.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKDictionary.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKDiscardContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKDocumentInterface.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKLanguageManager.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKMemoryStream.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKOutputElements.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKOutputManager.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKParser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPath.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPresentationRedirector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKProperties.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPropertyHandler.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPropertyMap.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKRecorder.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKShape.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKSpreadsheetRedirector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStyle.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStyleStack.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStylesheet.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKSubDirStream.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTableRecorder.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKText.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTextRecorder.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTextRedirector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKToken.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTokenizer.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTokenizerBase.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTransformation.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTypes.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKXMLContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKXMLContextBase.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKXMLParserState.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKZlibStream.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1Dictionary.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1ParserState.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1Token.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2Dictionary.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2ParserState.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2Token.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEY6Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEYCollector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEYProperties.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/KEYTypes.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1Dictionary.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1ParserState.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1Token.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/NUM3Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/NUMCollector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1Dictionary.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1ParserState.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1Token.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAG5Parser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAGCollector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAGProperties.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/PAGTypes.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKBezierElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKBinaryElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKBrContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKCalcEngineContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKCellCommentDrawableInfoElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKChartInfoElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKColorElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKCoreImageFilterDescriptorElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKDataElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFieldElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFillElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFilteredImageElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFormatElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFormulaElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKGeometryElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKGroupElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKHeaderFooterContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKImageContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKImageElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLayoutElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLineElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLineEndElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLinkElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelGeometriesProperty.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelGeometryElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelIndentsProperty.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelTypeinfoElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelTypesProperty.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListTextIndentsProperty.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKMediaElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKMetadataElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKNumberConverter.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPathElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPositionElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPropertyContextBase.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPropertyMapElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKRefContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKShapeContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKSizeElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKSpanElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStringElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStrokeContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStyleContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStyleRefContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStylesContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStylesheetBase.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTableInfoElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabsElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabularInfoElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabularModelElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextBodyElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextLabelElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextStorageElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKUnfilteredElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKWrapElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1ContentElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1FillElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1SpanElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1StylesContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1TableElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY2StyleContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1AnnotationContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1AnnotationElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1FootnotesElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1ShapeContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1StyleContext.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1TextStorageElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1DivElement.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1StringConverter.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/libetonyek_utils.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/libetonyek_xml.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWAParser.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKCollector.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKFormula.cpp
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTable.cpp



    ${REVENGE_LIB_ROOT}/src/lib/RVNGHTMLTextTableStyle.h
    ${REVENGE_LIB_ROOT}/src/lib/RVNGHTMLTextTextStyle.h
    ${REVENGE_LIB_ROOT}/src/lib/RVNGMemoryStream.h
    ${REVENGE_LIB_ROOT}/src/lib/RVNGOLEStream.h
    ${REVENGE_LIB_ROOT}/src/lib/RVNGRawGeneratorBase.h
    ${REVENGE_LIB_ROOT}/src/lib/RVNGZipStream.h
    ${REVENGE_LIB_ROOT}/src/lib/librevenge_internal.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGCSVSpreadsheetGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGHTMLTextGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGRawDrawingGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGRawPresentationGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGRawSpreadsheetGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGRawTextGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGSVGPresentationGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGTextDrawingGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGTextPresentationGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGTextSpreadsheetGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/RVNGTextTextGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/librevenge-generators-api.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-generators/librevenge-generators.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-stream/RVNGDirectoryStream.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-stream/RVNGStream.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-stream/librevenge-stream-api.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-stream/librevenge-stream.h
    ${REVENGE_LIB_ROOT}/inc/librevenge-stream/RVNGStreamImplementation.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGBinaryData.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGDrawingInterface.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGPresentationInterface.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGProperty.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGPropertyList.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGPropertyListVector.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGSVGDrawingGenerator.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGSpreadsheetInterface.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGString.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGStringVector.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/RVNGTextInterface.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/librevenge-api.h
    ${REVENGE_LIB_ROOT}/inc/librevenge/librevenge.h

    ${ODF_LIB_ROOT}/inc/libodfgen/OdfDocumentHandler.hxx
    ${ODF_LIB_ROOT}/inc/libodfgen/OdgGenerator.hxx
    ${ODF_LIB_ROOT}/inc/libodfgen/OdpGenerator.hxx
    ${ODF_LIB_ROOT}/inc/libodfgen/OdsGenerator.hxx
    ${ODF_LIB_ROOT}/inc/libodfgen/OdtGenerator.hxx
    ${ODF_LIB_ROOT}/inc/libodfgen/libodfgen-api.hxx
    ${ODF_LIB_ROOT}/inc/libodfgen/libodfgen.hxx
    ${ODF_LIB_ROOT}/src/DocumentElement.hxx
    ${ODF_LIB_ROOT}/src/FilterInternal.hxx
    ${ODF_LIB_ROOT}/src/FontStyle.hxx
    ${ODF_LIB_ROOT}/src/GraphicFunctions.hxx
    ${ODF_LIB_ROOT}/src/GraphicStyle.hxx
    ${ODF_LIB_ROOT}/src/InternalHandler.hxx
    ${ODF_LIB_ROOT}/src/ListStyle.hxx
    ${ODF_LIB_ROOT}/src/OdcGenerator.hxx
    ${ODF_LIB_ROOT}/src/OdfGenerator.hxx
    ${ODF_LIB_ROOT}/src/PageSpan.hxx
    ${ODF_LIB_ROOT}/src/SectionStyle.hxx
    ${ODF_LIB_ROOT}/src/SheetStyle.hxx
    ${ODF_LIB_ROOT}/src/Style.hxx
    ${ODF_LIB_ROOT}/src/TableStyle.hxx
    ${ODF_LIB_ROOT}/src/TextRunStyle.hxx
    ${ODF_LIB_ROOT}/test/StringDocumentHandler.hxx

    ${ETONYEK_LIB_ROOT}/inc/libetonyek/EtonyekDocument.h
    ${ETONYEK_LIB_ROOT}/inc/libetonyek/libetonyek.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAField.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAMessage.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAObjectIndex.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAObjectType.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAParser.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAReader.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWASnappyStream.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWAText.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKChainedTokenizer.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKChart.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKCollector.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKDictionary.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKDiscardContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKDocumentInterface.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKEnum.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKFormula.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKLanguageManager.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKMemoryStream.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKOutputElements.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKOutputManager.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKParser.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPath.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPath_fwd.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPresentationRedirector.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKProperties.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPropertyHandler.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPropertyInfo.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKPropertyMap.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKRecorder.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKShape.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKSpreadsheetRedirector.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStyle.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStyleStack.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStyle_fwd.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKStylesheet.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKSubDirStream.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTable.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTableRecorder.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKText.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTextRecorder.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTextRedirector.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKText_fwd.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKToken.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTokenInfo.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTokenizer.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTokenizerBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTransformation.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTypes.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKTypes_fwd.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKXMLContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKXMLContextBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKXMLParserState.h
    ${ETONYEK_LIB_ROOT}/src/lib/IWORKZlibStream.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1Dictionary.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1ParserState.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1Token.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY1XMLContextBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2Dictionary.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2ParserState.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2Token.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY2XMLContextBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY6ObjectType.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEY6Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEYCollector.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEYEnum.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEYProperties.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEYTypes.h
    ${ETONYEK_LIB_ROOT}/src/lib/KEYTypes_fwd.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1Dictionary.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1ParserState.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1Token.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM1XMLContextBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM3ObjectType.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUM3Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/NUMCollector.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1Dictionary.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1ParserState.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1Token.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG1XMLContextBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG5ObjectType.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAG5Parser.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAGCollector.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAGEnum.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAGProperties.h
    ${ETONYEK_LIB_ROOT}/src/lib/PAGTypes.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKBezierElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKBinaryElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKBrContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKCalcEngineContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKCellCommentDrawableInfoElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKChartInfoElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKColorElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKContainerContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKCoreImageFilterDescriptorElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKDataElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKDirectCollector.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFieldElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFillElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFilteredImageElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFormatElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKFormulaElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKGeometryElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKGroupElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKHeaderFooterContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKImageContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKImageElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLayoutElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLineElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLineEndElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKLinkElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelGeometriesProperty.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelGeometryElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelIndentsProperty.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelTypeinfoElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListLabelTypesProperty.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKListTextIndentsProperty.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKMediaElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKMetadataElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKMutableArrayElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKNumberConverter.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKNumberElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKNumericPropertyContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPathElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPositionElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPropertyContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPropertyContextBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPropertyMapElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPtrPropertyContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKPushCollector.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKRefContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKShapeContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKSizeElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKSpanElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStringElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStrokeContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStyleContainer.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStyleContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStyleRefContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStylesContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKStylesheetBase.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTableInfoElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabsElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabularInfoElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTabularModelElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextBodyElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextLabelElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKTextStorageElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKUnfilteredElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKValueContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/IWORKWrapElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1ContentElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1DivElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1FillElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1SpanElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1StringConverter.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1StylesContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY1TableElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/KEY2StyleContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1AnnotationContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1AnnotationElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1FootnotesElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1ShapeContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1StyleContext.h
    ${ETONYEK_LIB_ROOT}/src/lib/contexts/PAG1TextStorageElement.h
    ${ETONYEK_LIB_ROOT}/src/lib/libetonyek_utils.h
    ${ETONYEK_LIB_ROOT}/src/lib/libetonyek_xml.h
)

set_default_options(IWorkFile)

if(WIN32)
    target_sources(IWorkFile PRIVATE
        ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11/contrib/minizip/iowin32.c # OfficeUtils.pri
    )
endif()

target_include_directories(IWorkFile PRIVATE
    ${OFFICE_UTILS_ROOT_DIR}/src/zlib-1.2.11

    ${CMAKE_CURRENT_SOURCE_DIR}

    ${OFFICE_UTILS_ROOT_DIR}/src # OfficeUtils.pri

    ${DESKTOP_EDITOR_ROOT}/xml/libxml2/include

    # apple.pri
    ${APPLE_3RDPARTY_ROOT}
    ${APPLE_3RDPARTY_ROOT}/headers
    ${REVENGE_LIB_ROOT}/inc
    ${ODF_LIB_ROOT}/inc
    ${ETONYEK_LIB_ROOT}/inc
    ${ETONYEK_LIB_ROOT}/src/lib
    ${ETONYEK_LIB_ROOT}/src/lib/contexts
    ${MDDS_LIB_ROOT}/include
    ${GLM_LIB_ROOT}
)

if(APPLE)
    target_compile_definitions(IWorkFile PRIVATE
        unix
    )
endif()

target_compile_definitions(IWorkFile PRIVATE
    IWORK_USE_DYNAMIC_LIBRARY
    BUILD_ZLIB_AS_SOURCES # OfficeUtils.pri
)

# static libs
target_link_libraries(IWorkFile PUBLIC
    libxml
)

# shared libs
target_link_libraries(IWorkFile PUBLIC
    kernel
    UnicodeConverter
)
