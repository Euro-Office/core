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

QT       -= core gui

VERSION = 1.0.0.4
TARGET = DocxRenderer
TEMPLATE = lib

CONFIG += c++11
CONFIG += shared
CONFIG += plugin

CORE_ROOT_DIR = $$PWD/..
PWD_ROOT_DIR = $$PWD
include(../Common/base.pri)

DEFINES += DOCXRENDERER_USE_DYNAMIC_LIBRARY

ADD_DEPENDENCY(UnicodeConverter, kernel, graphics)

# Flag for disable full document creation. Enabled in pdf editor
#CONFIG += disable_full_document_creation

core_windows {
LIBS += \
	-lgdi32 \
	-ladvapi32 \
	-luser32 \
	-lshell32
}

HEADERS += \
	src/logic/elements/BaseItem.h \
	src/logic/elements/ContText.h \
	src/logic/elements/Paragraph.h \
	src/logic/elements/Shape.h \
	src/logic/elements/Table.h \
	src/logic/elements/TextLine.h \
	src/logic/managers/ExternalImageStorage.h \
	src/logic/managers/FontStyleManager.h \
	src/logic/managers/ImageManager.h \
	src/logic/managers/FontManager.h \
	src/logic/managers/ParagraphStyleManager.h \
	src/logic/styles/FontStyle.h \
	src/logic/styles/ParagraphStyle.h \
	src/resources/ColorTable.h \
	src/resources/Constants.h \
	src/resources/ImageInfo.h \
	src/resources/LinesTable.h \
	src/resources/VectorGraphics.h \
	src/resources/resources.h \
	src/resources/utils.h \
	src/logic/Page.h \
	src/logic/Document.h \
	DocxRenderer.h

SOURCES += \
	src/logic/elements/BaseItem.cpp \
	src/logic/elements/ContText.cpp \
	src/logic/elements/Paragraph.cpp \
	src/logic/elements/Shape.cpp \
	src/logic/elements/TextLine.cpp \
	src/logic/managers/FontManager.cpp \
	src/logic/managers/FontStyleManager.cpp \
	src/logic/managers/ImageManager.cpp \
	src/logic/managers/ParagraphStyleManager.cpp \
	src/logic/styles/FontStyle.cpp \
	src/logic/Page.cpp \
	src/logic/Document.cpp \
	src/logic/styles/ParagraphStyle.cpp \
	src/resources/VectorGraphics.cpp \
	DocxRenderer.cpp

disable_full_document_creation {
	DEFINES += DISABLE_FULL_DOCUMENT_CREATION
} else {
	SOURCES += \
		src/resources/resources.cpp
}

DISTFILES += \
	readme.md
