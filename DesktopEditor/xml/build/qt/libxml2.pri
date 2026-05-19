# Copyright (C) Ascensio System SIA, 2009-2026
#
# This program is a free software product. You can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License (AGPL)
# version 3 as published by the Free Software Foundation, together with the
# additional terms provided in the LICENSE file.
#
# This program is distributed WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For
# details, see the GNU AGPL at: https://www.gnu.org/licenses/agpl-3.0.html
#
# You can contact Ascensio System SIA by email at info@onlyoffice.com
# or by postal mail at 20A-6 Ernesta Birznieka-Upisha Street, Riga,
# LV-1050, Latvia, European Union.
#
# The interactive user interfaces in modified versions of the Program
# are required to display Appropriate Legal Notices in accordance with
# Section 5 of the GNU AGPL version 3.
#
# No trademark rights are granted under this License.
#
# All non-code elements of the Product, including illustrations,
# icon sets, and technical writing content, are licensed under the
# Creative Commons Attribution-ShareAlike 4.0 International License:
# https://creativecommons.org/licenses/by-sa/4.0/legalcode
#
# This license applies only to such non-code elements and does not
# modify or replace the licensing terms applicable to the Program's
# source code, which remains licensed under the GNU Affero General
# Public License v3.
#
# SPDX-License-Identifier: AGPL-3.0-only

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
