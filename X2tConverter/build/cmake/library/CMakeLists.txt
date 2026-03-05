cmake_minimum_required(VERSION 3.10)

project(x2tlib)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

set(X2T_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

find_package(Boost REQUIRED COMPONENTS system filesystem regex date_time)

if(NOT TARGET CryptoPPLib)
    add_subdirectory(${CORE_ROOT_DIR}/Common/3dParty/cryptopp/project CryptoPPLib)
endif()

if(NOT TARGET CompoundFileLib)
    add_subdirectory(${CORE_ROOT_DIR}/Common/cfcpp CompoundFileLib)
endif()

if(NOT TARGET DocFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/MsBinaryFile/Projects/DocFormatLib/Linux/cmake DocFormatLib)
endif()

if(NOT TARGET PptFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/MsBinaryFile/Projects/PPTFormatLib/Linux/cmake PptFormatLib)
endif()

if(NOT TARGET VbaFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/MsBinaryFile/Projects/VbaFormatLib/Linux/cmake VbaFormatLib)
endif()

if(NOT TARGET XlsFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/MsBinaryFile/Projects/XlsFormatLib/Linux/cmake XlsFormatLib)
endif()

if(NOT TARGET OdfFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OdfFile/Projects/Linux OdfFormatLib)
endif()

if(NOT TARGET BinDocument)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/BinDocument BinDocument)
endif()

if(NOT TARGET DocxFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/DocxFormatLib DocxFormatLib)
endif()

if(NOT TARGET PPTXFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/PPTXFormatLib PPTXFormatLib)
endif()

if(NOT TARGET XlsbFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/OOXML/Projects/Linux/XlsbFormatLib XlsbFormatLib)
endif()

if(NOT TARGET RtfFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/RtfFile/Projects/Linux RtfFormatLib)
endif()

if(NOT TARGET TxtXmlFormatLib)
    add_subdirectory(${CORE_ROOT_DIR}/TxtFile/Projects/Linux TxtXmlFormatLib)
endif()

if(NOT TARGET UnicodeConverter)
    add_subdirectory(${CORE_ROOT_DIR}/UnicodeConverter UnicodeConverter)
endif()

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

if(NOT TARGET kernel_network)
    add_subdirectory(${CORE_ROOT_DIR}/Common/Network kernel_network)
endif()

if(NOT TARGET Fb2File)
    add_subdirectory(${CORE_ROOT_DIR}/Fb2File Fb2File)
endif()

if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()

if(NOT TARGET HtmlFile2)
    add_subdirectory(${CORE_ROOT_DIR}/HtmlFile2 HtmlFile2)
endif()

if(NOT TARGET EpubFile)
    add_subdirectory(${CORE_ROOT_DIR}/EpubFile/cmake EpubFile)
endif()

if(NOT TARGET XpsFile)
    add_subdirectory(${CORE_ROOT_DIR}/XpsFile XpsFile)
endif()

if(NOT TARGET OFDFile)
    add_subdirectory(${CORE_ROOT_DIR}/OFDFile OFDFile)
endif()

if(NOT TARGET DjVuFile)
    add_subdirectory(${CORE_ROOT_DIR}/DjVuFile DjVuFile)
endif()

if(NOT TARGET DocxRenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DocxRenderer DocxRenderer)
endif()

if(NOT TARGET doctrenderer)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/doctrenderer doctrenderer)
endif()

if(NOT TARGET IWorkFile)
    add_subdirectory(${CORE_ROOT_DIR}/Apple IWorkFile)
endif()

if(NOT TARGET HWPFile)
    add_subdirectory(${CORE_ROOT_DIR}/HwpFile HWPFile)
endif()

if(NOT TARGET StarMathConverter)
    add_subdirectory(${CORE_ROOT_DIR}/OdfFile/Reader/Converter/StarMath2OOXML StarMathConverter)
endif()

if(NOT TARGET ooxmlsignature)
    add_subdirectory(${CORE_ROOT_DIR}/DesktopEditor/xmlsec/src ooxmlsignature)
endif()

message( "Fetch and build icu (x2t_lib)" )
set(ICU_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/icu")
set(ICU_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/icu")
set(ICU_MAJOR_VER "74")
set(ICU_MINOR_VER "2")
get_filename_component(ICU_3RDPARTY_DIR_ABS "${ICU_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(ICU_INSTALL_DIR_ABS "${ICU_INSTALL_DIR}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-build.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/icu" "${ICU_INSTALL_DIR}" ${ICU_MAJOR_VER} ${ICU_MINOR_VER}
    WORKING_DIRECTORY ${ICU_3RDPARTY_DIR_ABS}
)



add_library(x2tlib SHARED
    ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker2.cpp
	${X2T_ROOT_DIR}/src/cextracttools.cpp
	${X2T_ROOT_DIR}/src/ASCConverters.cpp

    ${CORE_ROOT_DIR}/Common/OfficeFileFormatChecker.h
    ${X2T_ROOT_DIR}/src/ASCConverters.h
    ${X2T_ROOT_DIR}/src/cextracttools.h
    ${X2T_ROOT_DIR}/src/lib/common.h
    ${X2T_ROOT_DIR}/src/lib/crypt.h
    ${X2T_ROOT_DIR}/src/lib/csv.h
    ${X2T_ROOT_DIR}/src/lib/doc.h
    ${X2T_ROOT_DIR}/src/lib/docx.h
    ${X2T_ROOT_DIR}/src/lib/html.h
    ${X2T_ROOT_DIR}/src/lib/hwp.h
    ${X2T_ROOT_DIR}/src/lib/iwork.h
    ${X2T_ROOT_DIR}/src/lib/odf.h
    ${X2T_ROOT_DIR}/src/lib/pdf_image.h
    ${X2T_ROOT_DIR}/src/lib/pdf_oform.h
    ${X2T_ROOT_DIR}/src/lib/ppt.h
    ${X2T_ROOT_DIR}/src/lib/pptx.h
    ${X2T_ROOT_DIR}/src/lib/rtf.h
    ${X2T_ROOT_DIR}/src/lib/txt.h
    ${X2T_ROOT_DIR}/src/lib/vsdx.h
    ${X2T_ROOT_DIR}/src/lib/xls.h
    ${X2T_ROOT_DIR}/src/lib/xlsx.h
)

set_default_options(x2tlib)

target_compile_definitions(x2tlib PRIVATE
    FILTER_FLATE_DECODE_ENABLED
    AVS_USE_CONVERT_PPTX_TOCUSTOM_VML
    PDFFILE_USE_DYNAMIC_LIBRARY
    XPS_USE_DYNAMIC_LIBRARY
    OFD_USE_DYNAMIC_LIBRARY
    DJVU_USE_DYNAMIC_LIBRARY
    HTMLFILE_USE_DYNAMIC_LIBRARY
    UNICODECONVERTER_USE_DYNAMIC_LIBRARY
    FILE_FORMAT_CHECKER_WITH_MACRO
    KERNEL_USE_DYNAMIC_LIBRARY
    GRAPHICS_USE_DYNAMIC_LIBRARY
    BUILD_X2T_AS_LIBRARY_DYLIB
)

target_include_directories(x2tlib PRIVATE
	${CORE_ROOT_DIR}/OOXML
    ${CORE_ROOT_DIR}/OOXML/PPTXFormat
    ${CORE_ROOT_DIR}/OOXML/Binary/Document
    ${CORE_ROOT_DIR}/OOXML/Binary/Presentation
    ${CORE_ROOT_DIR}/OfficeUtils/src
    ${ICU_INSTALL_DIR}/include
)

# Re-enable exporting symbols
target_compile_options(x2tlib PRIVATE
    -fvisibility=default
    -fvisibility-inlines-hidden
)

# static libs
target_link_libraries(x2tlib PRIVATE
    VbaFormatLib
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
    CompoundFileLib
    CryptoPPLib
)

# dynamic libs
target_link_libraries(x2tlib PUBLIC
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
    ${ICU_INSTALL_DIR_ABS}/lib/libicuuc.so.${ICU_MAJOR_VER}
    ${ICU_INSTALL_DIR_ABS}/lib/libicudata.so.${ICU_MAJOR_VER}
)

set_property(TARGET x2tlib PROPERTY POSITION_INDEPENDENT_CODE ON)

copy_artifacts_to_folder("x2tlib" "${EO_CORE_OUTPUT_DIR}")
copy_icu_libs(x2tlib)