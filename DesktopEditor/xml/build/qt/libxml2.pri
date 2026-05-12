DEFINES += HAVE_VA_COPY

core_static_link_xml_full {
DEFINES += \
    LIBXML_READER_ENABLED \
    LIBXML_PUSH_ENABLED \
    LIBXML_HTML_ENABLED \
    LIBXML_XPATH_ENABLED \
    LIBXML_OUTPUT_ENABLED \
    LIBXML_C14N_ENABLED \
    LIBXML_SAX1_ENABLED \
    LIBXML_TREE_ENABLED \
    LIBXML_XPTR_ENABLED \
    IN_LIBXML \
    LIBXML_STATIC
}

LIBXML2_ROOT_DIR = $$PWD/../../../../Common/3dParty/libxml/libxml2

INCLUDEPATH += \
    $$LIBXML2_ROOT_DIR/include \
    $$LIBXML2_ROOT_DIR/include/libxml \
    $$PWD

core_release {
    SOURCES += $$PWD/libxml2_all.c
    SOURCES += $$PWD/libxml2_all2.c

    DEFINES += XML_ERROR_DISABLE_MODE
}

core_debug {
SOURCES += \
    $$LIBXML2_ROOT_DIR/buf.c \
    $$LIBXML2_ROOT_DIR/c14n.c \
    $$LIBXML2_ROOT_DIR/catalog.c \
    $$LIBXML2_ROOT_DIR/chvalid.c \
    $$LIBXML2_ROOT_DIR/debugXML.c \
    $$LIBXML2_ROOT_DIR/dict.c \
    $$LIBXML2_ROOT_DIR/encoding.c \
    $$LIBXML2_ROOT_DIR/entities.c \
    $$LIBXML2_ROOT_DIR/error.c \
    $$LIBXML2_ROOT_DIR/globals.c \
    $$LIBXML2_ROOT_DIR/hash.c \
    $$LIBXML2_ROOT_DIR/libxml.h \
    $$LIBXML2_ROOT_DIR/list.c \
    $$LIBXML2_ROOT_DIR/nanohttp.c \
    $$LIBXML2_ROOT_DIR/parserInternals.c \
    $$LIBXML2_ROOT_DIR/pattern.c \
    $$LIBXML2_ROOT_DIR/relaxng.c \
    $$LIBXML2_ROOT_DIR/runsuite.c \
    $$LIBXML2_ROOT_DIR/schematron.c \
    $$LIBXML2_ROOT_DIR/shell.c \
    $$LIBXML2_ROOT_DIR/threads.c \
    $$LIBXML2_ROOT_DIR/timsort.h \
    $$LIBXML2_ROOT_DIR/tree.c \
    $$LIBXML2_ROOT_DIR/uri.c \
    $$LIBXML2_ROOT_DIR/xinclude.c \
    $$LIBXML2_ROOT_DIR/xlink.c \
    $$LIBXML2_ROOT_DIR/xmlIO.c \
    $$LIBXML2_ROOT_DIR/xmllint.c \
    $$LIBXML2_ROOT_DIR/xmlmemory.c \
    $$LIBXML2_ROOT_DIR/xmlmodule.c \
    $$LIBXML2_ROOT_DIR/xmlreader.c \
    $$LIBXML2_ROOT_DIR/xmlregexp.c \
    $$LIBXML2_ROOT_DIR/xmlsave.c \
    $$LIBXML2_ROOT_DIR/xmlschemas.c \
    $$LIBXML2_ROOT_DIR/xmlschemastypes.c \
    $$LIBXML2_ROOT_DIR/xmlstring.c \
    $$LIBXML2_ROOT_DIR/xmlwriter.c \
    $$LIBXML2_ROOT_DIR/xpath.c \
    $$LIBXML2_ROOT_DIR/xpointer.c \
    $$LIBXML2_ROOT_DIR/valid.c \
    $$LIBXML2_ROOT_DIR/parser.c
}

!core_only_libxml {
SOURCES +=  \
    $$PWD/../../src/xmlwriter.cpp \
    $$PWD/../../src/xmllight.cpp \
    $$PWD/../../src/xmldom.cpp

HEADERS += \
    $$PWD/../../src/xmllight_private.h

HEADERS += \
    $$PWD/../../include/xmlutils.h \
    $$PWD/../../include/xmlwriter.h
}

core_windows {
	LIBS += -lbcrypt
}
