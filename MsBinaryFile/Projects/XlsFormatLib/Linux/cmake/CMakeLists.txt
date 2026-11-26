cmake_minimum_required(VERSION 3.10)

project(XlsFormatLib)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../..")
include(${CORE_ROOT_DIR}/common.cmake)

set(XLS_FILE_SOURCES_DIR "${CORE_ROOT_DIR}/MsBinaryFile/XlsFile")
set(COMMON_SOURCES_DIR "${CORE_ROOT_DIR}/MsBinaryFile/Common")

add_library(XlsFormatLib STATIC
    ${XLS_FILE_SOURCES_DIR}/Format/Auxiliary/HelpFunc.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFRecord.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFRecordType.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFStream.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFStreamCacheReader.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFStreamCacheWriter.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CompoundFile.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/rtl/cipher.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/rtl/digest.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/BiffDecoder_RCF.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/BinaryCodec_RCF.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/Decryptor.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/RC4Crypt.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/XORCrypt.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Logging/Log.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Logging/Logger.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/ConvertXls2Xlsx.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/external_items.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/mediaitems_utils.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/namespaces.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/oox_content_type.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/oox_package.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/oox_rels.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/XlsConverter.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_conversion_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_drawing_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_drawings.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_hyperlinks.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_output_xml.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_package.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_protection.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_tablecontext.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_textcontext.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_chart_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_pivots_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_sheet_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_external_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_activeX_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_comments.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_comments_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xls_writer.cpp
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/pri/xls_format_logic.cpp

    ${COMMON_SOURCES_DIR}/Utils/OptPtr.h
    ${COMMON_SOURCES_DIR}/Utils/simple_xml_writer.h
    ${XLS_FILE_SOURCES_DIR}/Converter/ConvertXls2Xlsx.h
    ${XLS_FILE_SOURCES_DIR}/Converter/external_items.h
    ${XLS_FILE_SOURCES_DIR}/Converter/mediaitems_utils.h
    ${XLS_FILE_SOURCES_DIR}/Converter/namespaces.h
    ${XLS_FILE_SOURCES_DIR}/Converter/oox_content_type.h
    ${XLS_FILE_SOURCES_DIR}/Converter/oox_package.h
    ${XLS_FILE_SOURCES_DIR}/Converter/oox_rels.h
    ${XLS_FILE_SOURCES_DIR}/Converter/progressCallback.h
    ${XLS_FILE_SOURCES_DIR}/Converter/ShapeType.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xls_writer.h
    ${XLS_FILE_SOURCES_DIR}/Converter/XlsConverter.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_activeX_context.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_chart_context.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_conversion_context.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_drawing_context.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_drawings.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_external_context.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_hyperlinks.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_output_xml.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_package.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_pivots_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_protection.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_sheet_context.cpp
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_tablecontext.h
    ${XLS_FILE_SOURCES_DIR}/Converter/xlsx_textcontext.h
    ${XLS_FILE_SOURCES_DIR}/Format/Auxiliary/HelpFunc.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/BinSmartPointers.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFRecord.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFRecordType.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFStream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFStreamCacheReader.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CFStreamCacheWriter.h
    ${XLS_FILE_SOURCES_DIR}/Format/Binary/CompoundFile.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/BiffDecoder_RCF.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/BinaryCodec_RCF.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/Crypt.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/Decryptor.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/RC4Crypt.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/rtl/cipher.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/rtl/digest.h
    ${XLS_FILE_SOURCES_DIR}/Format/Crypt/XORCrypt.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logging/Log.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logging/Logger.h    
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/BinProcessor.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/ChartSheetSubstream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/CommonSubstream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/CompositeObject.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/EncryptionStream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/GlobalsSubstream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/GlobalWorkbookInfo.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/MacroSheetSubstream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/WorkbookStreamObject.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/WorksheetSubstream.h
    ${XLS_FILE_SOURCES_DIR}/Format/Logic/XlsElementsType.h
)

target_precompile_headers(XlsFormatLib PRIVATE
    ../precompiled.h
)

# The -Werror=return-type guard below must survive EO_NO_WARNINGS. That option
# adds a directory-level -w, and on GCC and the non-Apple clang versions tested
# -w cancels -Werror= promotions no matter the order, so the guard would silently
# do nothing in the CI builds that set it (#421). In that build, strip -w from
# this target and keep the logs quiet another way: -Wreturn-type is on by
# default, so the promotion still fires.
# - GCC: drop the blanket -Wall/-Wextra. A few dozen default-on warnings remain.
# - Clang: -Wall/-Wextra alone are not enough, clang turns hundreds of this
#   target's warnings on by default, so add -Wno-everything. -Werror=return-type
#   comes later on the command line and turns its warning back on as an error.
# (add_compile_options(-w) is copied into the target's COMPILE_OPTIONS when the
# target is created, so it is removed there; -Wall/-Wextra come from
# set_default_options(), so they are dropped from COMMON_CXX_FLAGS before it runs.)
if( EO_NO_WARNINGS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" )
    get_target_property( _xls_compile_options XlsFormatLib COMPILE_OPTIONS )
    if( _xls_compile_options )
        list( REMOVE_ITEM _xls_compile_options -w )
        set_property( TARGET XlsFormatLib PROPERTY COMPILE_OPTIONS "${_xls_compile_options}" )
    endif()
    list( REMOVE_ITEM COMMON_CXX_FLAGS -Wall -Wextra )
    if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
        target_compile_options( XlsFormatLib PRIVATE -Wno-everything )
    endif()
endif()

set_default_options(XlsFormatLib)

if( MSVC )
    target_compile_definitions( XlsFormatLib PRIVATE
        _UNICODE
        UNICODE
    )
endif()

target_include_directories(XlsFormatLib PRIVATE
    ${XLS_FILE_SOURCES_DIR}/Format
    ${XLS_FILE_SOURCES_DIR}/Format/Logic
    ${COMMON_SOURCES_DIR}
    ${CORE_ROOT_DIR}/OOXML/XlsbFormat
)

target_link_libraries(XlsFormatLib PRIVATE 
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
)

if( MSVC )
    target_compile_options( XlsFormatLib PRIVATE /bigobj )
endif()

# A non-void function that runs off its end is undefined behaviour. GCC only warns, and at -O3 it treated the
# missing `return` in PIVOTLI::saveContent as unreachable code: every XLS export of a pivot table crashed.
# See the EO_NO_WARNINGS note above for why -w is stripped from this target so this promotion is not cancelled.
if( CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" )
    target_compile_options( XlsFormatLib PRIVATE -Werror=return-type )
endif()

set_property(TARGET XlsFormatLib PROPERTY POSITION_INDEPENDENT_CODE ON)

declare_victory( XlsFormatLib )
