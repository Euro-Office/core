QT -= core
QT -= gui

TARGET = test
TEMPLATE = app

CONFIG += console
CONFIG -= app_bundle

CORE_ROOT_DIR = $$PWD/../..
PWD_ROOT_DIR = $$PWD
include($$CORE_ROOT_DIR/Common/base.pri)
include($$CORE_ROOT_DIR/Common/3dParty/googletest/googletest.pri)
include($$CORE_ROOT_DIR/Common/3dParty/icu/icu.pri)

# test.cpp includes PdfWriter.h, which pulls SrcWriter/Document.h and its
# <ft2build.h>; ADD_DEPENDENCY only adds linker flags.
INCLUDEPATH += \
	$$CORE_ROOT_DIR/DesktopEditor/freetype-2.10.4/include \
	$$CORE_ROOT_DIR/DesktopEditor/freetype-2.10.4/include/freetype

ADD_DEPENDENCY(UnicodeConverter, kernel, graphics, PdfFile, DjVuFile, ooxmlsignature)

SOURCES += test.cpp

DESTDIR = $$PWD/build
