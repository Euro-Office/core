QT       -= core gui

VERSION = 0.0.0.1
TARGET = StarMathConverter
TEMPLATE = lib

CONFIG += shared
CONFIG += plugin

CORE_ROOT_DIR = $$PWD/../../../..
PWD_ROOT_DIR = $$PWD

DEFINES += STARMATH_USE_DYNAMIC_LIBRARY \
           CRYPTOPP_DISABLE_ASM

include($$CORE_ROOT_DIR/Common/base.pri)
include($$CORE_ROOT_DIR/Common/3dParty/icu/icu.pri)
include($$CORE_ROOT_DIR/Common/3dParty/boost/boost.pri)

LIBS += -L$$CORE_BUILDS_LIBRARIES_PATH -lCryptoPPLib
ADD_DEPENDENCY(kernel)

SOURCES +=  $$PWD/cconversionsmtoooxml.cpp \
    $$CORE_ROOT_DIR/OOXML/Base/Unit.cpp \
    $$PWD/conversionmathformula.cpp \
    $$PWD/cstarmathpars.cpp

HEADERS += \
    $$PWD/TextDirection.h \
    $$PWD/cconversionsmtoooxml.h \
    $$CORE_ROOT_DIR/OOXML/Base/Unit.h \
    $$PWD/conversionmathformula.h \
    $$PWD/cstarmathpars.h \
    $$PWD/fontType.h \
    $$PWD/typeConversion.h \
    $$PWD/typeselements.h \
    $$PWD/TFormulaSize.h
