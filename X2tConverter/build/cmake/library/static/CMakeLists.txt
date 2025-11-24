cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(x2tlib)

set(CORE_ROOT_DIR "../../../../..")
set(X2T_ROOT_DIR "../../../..")

file(READ "${CORE_ROOT_DIR}/Common/version.txt" VERSION_TXT_CONTENT)

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

add_library(x2tlib STATIC
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

target_compile_definitions(x2tlib PRIVATE
    UNICODE
	_UNICODE
    FILTER_FLATE_DECODE_ENABLED
    DONT_WRITE_EMBEDDED_FONTS
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
    INTVER=${VERSION_TXT_CONTENT}
)

target_include_directories(x2tlib PRIVATE
	${CORE_ROOT_DIR}/OOXML
    ${CORE_ROOT_DIR}/OOXML/PPTXFormat
    ${CORE_ROOT_DIR}/OOXML/Binary/Document
    ${CORE_ROOT_DIR}/OOXML/Binary/Presentation
    ${CORE_ROOT_DIR}/OfficeUtils/src
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
)

# TODO
# set_target_properties(MyApp PROPERTIES
#     BUILD_RPATH "/path/to/libs"
#     INSTALL_RPATH "/path/to/libs"
# )
