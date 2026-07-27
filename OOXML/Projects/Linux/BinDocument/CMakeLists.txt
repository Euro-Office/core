cmake_minimum_required(VERSION 3.10)

project(BinDocument)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../..")

include(${CORE_ROOT_DIR}/common.cmake)

set(OOXML_ROOT_DIR "${CORE_ROOT_DIR}/OOXML")
set(BIN_SOURCES_DIR "${OOXML_ROOT_DIR}/Binary")
set(MS_BINARY_FILE_ROOT "${CORE_ROOT_DIR}/MsBinaryFile")
set(DB_ENGINES_DIR "${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseEngines")

# Built as a separate shared library (not compiled directly into BinDocument)
# so that mdbtools' LGPLv2+ source is dynamically linked, not statically bound
# into the AGPL-licensed application. Vendored verbatim from upstream mdbtools
# v1.0.1 (matches the version already recorded in mdbver.h); see
# desktop-apps/package/common/license/3dparty/3DPARTYLICENSE for the notice.
add_library(mdbtools SHARED
    ${DB_ENGINES_DIR}/libmdb/backend.c
    ${DB_ENGINES_DIR}/libmdb/catalog.c
    ${DB_ENGINES_DIR}/libmdb/data.c
    ${DB_ENGINES_DIR}/libmdb/dump.c
    ${DB_ENGINES_DIR}/libmdb/fakeglib.c
    ${DB_ENGINES_DIR}/libmdb/file.c
    ${DB_ENGINES_DIR}/libmdb/iconv.c
    ${DB_ENGINES_DIR}/libmdb/index.c
    ${DB_ENGINES_DIR}/libmdb/like.c
    ${DB_ENGINES_DIR}/libmdb/map.c
    ${DB_ENGINES_DIR}/libmdb/money.c
    ${DB_ENGINES_DIR}/libmdb/options.c
    ${DB_ENGINES_DIR}/libmdb/props.c
    ${DB_ENGINES_DIR}/libmdb/rc4.c
    ${DB_ENGINES_DIR}/libmdb/sargs.c
    ${DB_ENGINES_DIR}/libmdb/stats.c
    ${DB_ENGINES_DIR}/libmdb/table.c
    ${DB_ENGINES_DIR}/libmdb/version.c
    ${DB_ENGINES_DIR}/libmdb/worktable.c
    ${DB_ENGINES_DIR}/libmdb/write.c
)
set_default_options(mdbtools)
target_include_directories(mdbtools PRIVATE ${DB_ENGINES_DIR}/mdbtools)
target_compile_definitions(mdbtools PRIVATE HAVE_ICONV "ICONV_CONST=")
set_property(TARGET mdbtools PROPERTY POSITION_INDEPENDENT_CODE ON)
# set_default_options() applies -fvisibility=hidden project-wide, which would
# hide the mdb_* C API entirely from mdbtools.so's exported symbol table.
# This library's whole purpose is to be linked externally, so override back
# to default visibility.
target_compile_options(mdbtools PRIVATE -fvisibility=default)

add_library(BinDocument STATIC
    ${BIN_SOURCES_DIR}/Document/DocWrapper/DocxSerializer.cpp
    ${BIN_SOURCES_DIR}/Document/DocWrapper/FontProcessor.cpp
    ${BIN_SOURCES_DIR}/Document/DocWrapper/XlsxSerializer.cpp
    ${BIN_SOURCES_DIR}/Document/DocWrapper/ChartSerializer.cpp
    ${BIN_SOURCES_DIR}/Document/DocWrapper/VsdxSerializer.cpp
    ${BIN_SOURCES_DIR}/Document/BinWriter/BinaryWriterD.cpp
    ${BIN_SOURCES_DIR}/Sheets/Common/Common.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/ChartFromToBinary.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CommonWriter.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CSVReader.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseReader.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseEngines/MdbEngine.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseEngines/SqliteEngine.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseEngines/DuckDbEngine.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseEngines/BerkeleyDbEngine.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/BinaryWriterS.cpp
    ${BIN_SOURCES_DIR}/Sheets/Writer/BinaryReaderS.cpp
    ${BIN_SOURCES_DIR}/Sheets/Writer/CSVWriter.cpp
    ${BIN_SOURCES_DIR}/Draw/BinaryWriterV.cpp
    ${BIN_SOURCES_DIR}/Draw/BinaryReaderV.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/ReaderClasses.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/BinaryReaderD.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/CustomXmlWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/FileWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/ChartWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/CommentsWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/DefaultThemeWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/DocumentRelsWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/DocumentWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/fontTableWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/HeaderFooterWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/MediaWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/NumberingWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/SettingWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/StylesWriter.cpp
    ${BIN_SOURCES_DIR}/Document/BinReader/webSettingsWriter.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLReader.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XML2TableConverter.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XLSXTableController.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/columnNameController.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLConverter2.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLMap.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLReader2.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/CellFormatController.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/DigitReader.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/DateReader.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/LocalInfo.cpp
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/CurrencyReader.cpp
    
    ${CORE_ROOT_DIR}/OfficeCryptReader/source/ECMACryptFile.h
    ${CORE_ROOT_DIR}/OfficeCryptReader/source/CryptTransform.h
    ${BIN_SOURCES_DIR}/Document/DocWrapper/DocxSerializer.h
    ${BIN_SOURCES_DIR}/Document/DocWrapper/FontProcessor.h
    ${BIN_SOURCES_DIR}/Document/DocWrapper/XlsxSerializer.h
    ${BIN_SOURCES_DIR}/Document/DocWrapper/VsdxSerializer.h
    ${BIN_SOURCES_DIR}/Document/BinReader/ChartWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/CommentsWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/DocumentRelsWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/DocumentWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/FileWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/fontTableWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/HeaderFooterWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/MediaWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/NumberingWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/ReaderClasses.h
    ${BIN_SOURCES_DIR}/Document/BinReader/BinaryReaderD.h
    ${BIN_SOURCES_DIR}/Document/BinReader/SettingWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/StylesWriter.h
    ${BIN_SOURCES_DIR}/Document/BinWriter/BinEquationWriter.h
    ${BIN_SOURCES_DIR}/Document/BinWriter/BinReaderWriterDefines.h
    ${BIN_SOURCES_DIR}/Document/BinWriter/BinaryWriterD.h
    ${BIN_SOURCES_DIR}/Sheets/Common/BinReaderWriterDefines.h
    ${BIN_SOURCES_DIR}/Sheets/Common/Common.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/BinaryWriterS.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/ChartFromToBinary.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CommonWriter.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CSVReader.h
    ${BIN_SOURCES_DIR}/Sheets/Writer/BinaryCommonReader.h
    ${BIN_SOURCES_DIR}/Sheets/Writer/BinaryReaderS.h
    ${BIN_SOURCES_DIR}/Sheets/Writer/CSVWriter.h
    ${BIN_SOURCES_DIR}/Draw/BinaryReaderV.h
    ${BIN_SOURCES_DIR}/Draw/BinaryWriterV.h
    ${BIN_SOURCES_DIR}/Draw/BinReaderWriterDefines.h
    ${BIN_SOURCES_DIR}/Document/BinReader/webSettingsWriter.h
        # ../../../../Common/FileDownloader/FileDownloader.h
    ${BIN_SOURCES_DIR}/Document/BinReader/DefaultThemeWriter.h
    # ${BIN_SOURCES_DIR}/Document/DocWrapper/ChartWriter.h
    ${BIN_SOURCES_DIR}/Document/BinReader/CustomXmlWriter.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLReader.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XML2TableConverter.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XLSXTableController.h
    # ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/columnNameController.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLConverter2.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/XMLReader/XMLMap.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/CellFormatController.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/LocalInfo.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/DateReader.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/DigitReader.h
    ${BIN_SOURCES_DIR}/Sheets/Reader/CellFormatController/CurrencyReader.h
)

set_default_options(BinDocument)

if( MSVC )
    target_compile_definitions( BinDocument PRIVATE
        _UNICODE
        UNICODE
    )
endif()

target_compile_definitions(BinDocument PRIVATE
	SOLUTION_ASCOFFICEDOCXFILE2
	#DISABLE_FILE_DOWNLOADER
	AVS_USE_CONVERT_PPTX_TOCUSTOM_VML
	HAVE_ICONV
	"ICONV_CONST="
)

target_precompile_headers(BinDocument PRIVATE
    precompiled.h
)

target_include_directories(BinDocument
    PRIVATE
    ${MS_BINARY_FILE_ROOT}/XlsFile/Format
    ${MS_BINARY_FILE_ROOT}/Common/common_xls
    ${OOXML_ROOT_DIR}/Binary/XlsbFormat
    ${BIN_SOURCES_DIR}/Sheets/Reader/DatabaseEngines/mdbtools
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
target_include_directories(BinDocument PRIVATE ${GLIB_INCLUDE_DIRS})

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(DuckDB CONFIG REQUIRED)
find_package(unofficial-berkeleydb CONFIG REQUIRED)

target_link_libraries(BinDocument PRIVATE
    Boost::system
    Boost::filesystem
    Boost::regex
    Boost::date_time
    unofficial::sqlite3::sqlite3
    $<IF:$<TARGET_EXISTS:duckdb>,duckdb,duckdb_static>
    mdbtools
    unofficial::berkeleydb::libdb
    ${GLIB_LIBRARIES}
)

set_property(TARGET BinDocument PROPERTY POSITION_INDEPENDENT_CODE ON)

declare_victory( BinDocument )
