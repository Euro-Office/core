cmake_minimum_required(VERSION 3.10)

project(libxml)

function(add_libxml TARGET XML_ROOT_DIR ONLY_LIBXML)

    if(NOT ONLY_LIBXML)
        message("Including XML writer!")
        target_sources( ${TARGET} PRIVATE
            ${XML_ROOT_DIR}/src/xmlwriter.cpp
            ${XML_ROOT_DIR}/src/xmllight.cpp
            ${XML_ROOT_DIR}/src/xmldom.cpp

            ${XML_ROOT_DIR}/src/xmllight_private.h
            ${XML_ROOT_DIR}/include/xmlutils.h
            ${XML_ROOT_DIR}/include/xmlwriter.h
        )
    endif()

    target_link_libraries( ${TARGET} PUBLIC LibXml2 )

endfunction()
