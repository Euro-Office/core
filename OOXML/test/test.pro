TEMPLATE = app
CONFIG   += console
CONFIG   -= app_bundle

DEFINES += BUILD_X2T_AS_LIBRARY_DYLIB

X2T_DIR = $$PWD/../../X2tConverter

include($$X2T_DIR/build/Qt/X2tConverter.pri)
include($$X2T_DIR/../Common/3dParty/googletest/googletest.pri)

HEADERS += $$X2T_DIR/src/dylib/x2t.h
SOURCES += $$X2T_DIR/src/dylib/x2t.cpp

SOURCES += main.cpp\
           activex_groupname.cpp\
           common.cpp\
           checkbox.cpp\
           richvalue.cpp\
           conversion.cpp\
           editorbin_pivots.cpp\
           parameter_type.cpp\
           editorbin_source_file_type.cpp\
           pivot_show_data_as.cpp\
           vml_shape_size.cpp\
           vml_fill_relationship_id.cpp\
           workbook_facts.cpp\
           xlsb_workbook_protection.cpp\
           xlsb_protected_ranges.cpp\

HEADERS += common.h workbook_facts.h

DESTDIR = $$CORE_BUILDS_BINARY_PATH


SOURCES -= $$CORE_GTEST_PATH/src/gtest_main.cc
SOURCES -= ../../../Common/OfficeFileFormatChecker2.cpp
SOURCES -= ../../src/cextracttools.cpp
SOURCES -= ../../src/ASCConverters.cpp
SOURCES += ../../Common/OfficeFileFormatChecker2.cpp
SOURCES += $$X2T_DIR/src/cextracttools.cpp
SOURCES += $$X2T_DIR/src/ASCConverters.cpp
