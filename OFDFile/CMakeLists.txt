cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE STRING "C compiler" FORCE)

project(OFDFile)

set(CORE_ROOT_DIR "..")

if(NOT TARGET PdfFile)
    add_subdirectory(${CORE_ROOT_DIR}/PdfFile PdfFile)
endif()

add_library(OFDFile SHARED
    OFDFile.cpp
    src/Annotation.cpp
    src/Base.cpp
    src/Content/Content.cpp
    src/Content/GraphicUnit.cpp
    src/Content/ImageObject.cpp
    src/Content/Layer.cpp
    src/Content/PageBlock.cpp
    src/Content/PathObject.cpp
    src/Content/TextObject.cpp
    src/Document.cpp
    src/OFDFile_Private.cpp
    src/Page.cpp
    src/Res.cpp
    src/Types/Color.cpp
    src/Types/ColorSpace.cpp
    src/Types/CommonData.cpp
    src/Types/CompositeGraphicUnit.cpp
    src/Types/DrawParam.cpp
    src/Types/Font.cpp
    src/Types/MultiMedia.cpp
    src/Types/PageArea.cpp
    src/Types/PenSettings.cpp
    src/Types/Signature.cpp
    src/Types/TemplatePage.cpp
    src/Utils/Types.cpp
    src/Utils/XmlReader.cpp

    OFDFile.h
    src/Annotation.h
    src/Base.h
    src/Content/Content.h
    src/Content/GraphicUnit.h
    src/Content/ImageObject.h
    src/Content/IPageBlock.h
    src/Content/Layer.h
    src/Content/PageBlock.h
    src/Content/PathObject.h
    src/Content/TextObject.h
    src/Document.h
    src/IOFDElement.h
    src/OFDFile_Private.h
    src/Page.h
    src/Res.h
    src/Types/Color.h
    src/Types/ColorSpace.h
    src/Types/CommonData.h
    src/Types/CompositeGraphicUnit.h
    src/Types/DrawParam.h
    src/Types/Font.h
    src/Types/MultiMedia.h
    src/Types/PageArea.h
    src/Types/PenSettings.h
    src/Types/Signature.h
    src/Types/TemplatePage.h
    src/Utils/Types.h
    src/Utils/Utils.h
    src/Utils/XmlReader.h
)

target_include_directories(OFDFile PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CORE_ROOT_DIR}/DesktopEditor/freetype-2.10.4/include
    ${CORE_ROOT_DIR}/DesktopEditor/freetype-2.10.4/include/freetype
)

target_compile_definitions(OFDFile PRIVATE
    OFD_USE_DYNAMIC_LIBRARY
)

# dynamic libs
target_link_libraries(OFDFile PUBLIC
    kernel
    UnicodeConverter
    graphics
    PdfFile

    # gdi32 # Windows
    # advapi32 # Windows
    # user32 # Windows
    # shell32 # Windows
    # Ole32 # Windows
)
